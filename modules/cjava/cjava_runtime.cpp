/**************************************************************************/
/*  cjava_runtime.cpp                                                     */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_runtime.h"

#include "cjava_script.h"

#include "core/string/print_string.h"
#include "scene/main/node.h"

static uint32_t _read_u32(const Vector<uint8_t> &p_code, int &r_ip) {
	uint32_t v = p_code[r_ip] | (p_code[r_ip + 1] << 8) | (p_code[r_ip + 2] << 16) | (p_code[r_ip + 3] << 24);
	r_ip += 4;
	return v;
}

static uint16_t _read_u16(const Vector<uint8_t> &p_code, int &r_ip) {
	uint16_t v = p_code[r_ip] | (p_code[r_ip + 1] << 8);
	r_ip += 2;
	return v;
}

Variant CJavaRuntime::execute(CJavaInstance *p_instance, const CJavaMethodDef &p_method, const Variant **p_args, int p_argcount) {
	ERR_FAIL_NULL_V(p_instance, Variant());
	const Ref<CJavaScript> script = p_instance->get_script();
	ERR_FAIL_COND_V(script.is_null(), Variant());
	const CJavaBytecodeModule &bc = script->get_parsed_script().bytecode;
	const CJavaBytecodeChunk &chunk = p_method.chunk;

	Vector<Variant> stack;
	Vector<Variant> locals;
	locals.resize(p_method.local_slots.size());

	int ip = chunk.offset;
	const int end = chunk.offset + chunk.length;

	auto pop = [&]() -> Variant {
		ERR_FAIL_COND_V(stack.is_empty(), Variant());
		return stack[stack.size() - 1];
	};
	auto pop_discard = [&]() {
		if (!stack.is_empty()) {
			stack.remove_at(stack.size() - 1);
		}
	};

	Node *owner_node = Object::cast_to<Node>(p_instance->get_owner());

	while (ip < end) {
		const CJavaOpcode op = (CJavaOpcode)bc.code[ip++];

		switch (op) {
			case OP_NOP:
				break;
			case OP_PUSH_CONST: {
				const uint32_t idx = _read_u32(bc.code, ip);
				stack.push_back(bc.constants[idx]);
			} break;
			case OP_PUSH_LOCAL: {
				const uint16_t idx = _read_u16(bc.code, ip);
				Variant value;
				if (idx < locals.size()) {
					value = locals[idx];
				}
				if (value.get_type() == Variant::OBJECT && idx < p_method.local_slot_defs.size()) {
					const CJavaLocalDef &def = p_method.local_slot_defs[idx];
					if (def.ownership == CJAVA_OWNERSHIP_BORROWED) {
						Object *obj = value.get_validated_object();
						if (!obj) {
							ERR_PRINT(vformat("CJava: @borrow local slot %d references a freed object", (int)idx));
							value = Variant();
						}
					}
				}
				stack.push_back(value);
			} break;
			case OP_PUSH_FIELD: {
				const uint16_t idx = _read_u16(bc.code, ip);
				const CJavaParsedScript &parsed = script->get_parsed_script();
				Variant value = p_instance->get_field_value(idx);
				if (value.get_type() == Variant::OBJECT && idx < parsed.fields.size()) {
					const CJavaFieldDef &field = parsed.fields[idx];
					if (field.ownership == CJAVA_OWNERSHIP_BORROWED) {
						Object *obj = value.get_validated_object();
						if (!obj) {
							ERR_PRINT(vformat("CJava: @borrow field '%s' references a freed object", field.name));
							value = Variant();
						}
					}
				}
				stack.push_back(value);
			} break;
			case OP_PUSH_ARG: {
				const uint16_t idx = _read_u16(bc.code, ip);
				if (idx < p_argcount) {
					stack.push_back(*p_args[idx]);
				} else {
					stack.push_back(Variant());
				}
			} break;
			case OP_POP:
				pop_discard();
				break;
			case OP_STORE_LOCAL: {
				const uint16_t idx = _read_u16(bc.code, ip);
				const Variant v = pop();
				if (idx >= locals.size()) {
					locals.resize(idx + 1);
				}
				locals.write[idx] = v;
				if (v.get_type() == Variant::OBJECT && idx < p_method.local_slot_defs.size()) {
					const CJavaLocalDef &def = p_method.local_slot_defs[idx];
					const StringName symbol = StringName(vformat("__local_%d", (int)idx));
					p_instance->track_symbol(symbol, v, def.ownership, def.type);
				}
			} break;
			case OP_STORE_FIELD: {
				const uint16_t idx = _read_u16(bc.code, ip);
				const Variant v = pop();
				const CJavaParsedScript &parsed = script->get_parsed_script();
				if (idx < parsed.fields.size()) {
					p_instance->set_field_value(idx, v, parsed.fields[idx]);
				}
			} break;
			case OP_ADD:
			case OP_SUB:
			case OP_MUL:
			case OP_DIV: {
				const Variant b = pop();
				const Variant a = pop();
				Variant::Operator variant_op = Variant::OP_MAX;
				switch (op) {
					case OP_ADD:
						variant_op = Variant::OP_ADD;
						break;
					case OP_SUB:
						variant_op = Variant::OP_SUBTRACT;
						break;
					case OP_MUL:
						variant_op = Variant::OP_MULTIPLY;
						break;
					case OP_DIV:
						variant_op = Variant::OP_DIVIDE;
						break;
					default:
						break;
				}
				bool valid = false;
				Variant result;
				Variant::evaluate(variant_op, a, b, result, valid);
				stack.push_back(valid ? result : Variant());
			} break;
			case OP_NEG: {
				const Variant a = pop();
				bool valid = false;
				Variant result;
				Variant::evaluate(Variant::OP_NEGATE, a, Variant(), result, valid);
				stack.push_back(valid ? result : Variant());
			} break;
			case OP_NOT: {
				Variant a = pop();
				stack.push_back(!a.booleanize());
			} break;
			case OP_CMP_EQ:
			case OP_CMP_NE:
			case OP_CMP_LT:
			case OP_CMP_LE:
			case OP_CMP_GT:
			case OP_CMP_GE: {
				const Variant b = pop();
				const Variant a = pop();
				bool res = false;
				switch (op) {
					case OP_CMP_EQ:
						res = a == b;
						break;
					case OP_CMP_NE:
						res = a != b;
						break;
					case OP_CMP_LT:
						res = a < b;
						break;
					case OP_CMP_LE:
						res = a <= b;
						break;
					case OP_CMP_GT:
						res = a > b;
						break;
					case OP_CMP_GE:
						res = a >= b;
						break;
					default:
						break;
				}
				stack.push_back(res);
			} break;
			case OP_JUMP: {
				const int32_t rel = (int32_t)_read_u32(bc.code, ip);
				ip += rel;
			} break;
			case OP_JUMP_IF_FALSE: {
				const int32_t rel = (int32_t)_read_u32(bc.code, ip);
				const Variant cond = pop();
				if (!cond.booleanize()) {
					ip += rel;
				}
			} break;
			case OP_CALL_PRINT: {
				const uint8_t argc = bc.code[ip++];
				String line;
				for (int i = argc - 1; i >= 0; i--) {
					if (!line.is_empty()) {
						line += " ";
					}
					line += String(pop());
				}
				print_line(line);
			} break;
			case OP_CALL_GET_NODE: {
				const Variant path_var = pop();
				ERR_FAIL_COND_V(owner_node == nullptr, Variant());
				Node *node = owner_node->get_node(NodePath(path_var));
				stack.push_back(node);
			} break;
			case OP_CALL_METHOD: {
				const uint32_t name_idx = _read_u32(bc.code, ip);
				const uint8_t argc = bc.code[ip++];
				Vector<Variant> args;
				args.resize(argc);
				for (int i = argc - 1; i >= 0; i--) {
					args.write[i] = pop();
				}
				const Variant obj_var = pop();
				Object *obj = obj_var.get_validated_object();
				if (obj) {
					const StringName method = bc.constants[name_idx];
					Callable::CallError ce;
					const Variant ret = obj->callp(method, (const Variant **)args.ptr(), argc, ce);
					stack.push_back(ret);
				} else {
					stack.push_back(Variant());
				}
			} break;
			case OP_CALL_SUPER: {
				const uint32_t name_idx = _read_u32(bc.code, ip);
				const uint8_t argc = bc.code[ip++];
				Vector<Variant> args;
				args.resize(argc);
				for (int i = argc - 1; i >= 0; i--) {
					args.write[i] = pop();
				}
				const StringName method_name = bc.constants[name_idx];
				const Variant ret = CJavaRuntime::execute_super(p_instance, method_name, (const Variant **)args.ptr(), argc);
				stack.push_back(ret);
			} break;
			case OP_END:
				return Variant();
			default:
				ERR_PRINT(vformat("CJava VM: unknown opcode %d", (int)op));
				return Variant();
		}
	}
	return Variant();
}

Variant CJavaRuntime::execute_super(CJavaInstance *p_instance, const StringName &p_method, const Variant **p_args, int p_argcount) {
	ERR_FAIL_NULL_V(p_instance, Variant());
	const Ref<CJavaScript> script = p_instance->get_script();
	ERR_FAIL_COND_V(script.is_null(), Variant());
	const Ref<Script> base_ref = script->get_base_script();
	const Ref<CJavaScript> base = base_ref;
	ERR_FAIL_COND_V(base.is_null(), Variant());
	ERR_FAIL_COND_V(!base->get_parsed_script().methods.has(p_method), Variant());
	return execute(p_instance, base->get_parsed_script().methods[p_method], p_args, p_argcount);
}
