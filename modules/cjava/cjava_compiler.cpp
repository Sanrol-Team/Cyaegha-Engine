/**************************************************************************/
/*  cjava_compiler.cpp                                                    */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_compiler.h"

#include "cjava_types.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "core/string/char_utils.h"
#include "scene/main/node.h"

namespace {

enum TokenType {
	TK_EOF,
	TK_IDENT,
	TK_NUMBER,
	TK_STRING,
	TK_OP,
	TK_PUNCT,
};

struct Token {
	TokenType type = TK_EOF;
	String text;
	double number = 0.0;
	int line = 1;
};

struct Lexer {
	String source;
	int pos = 0;
	int line = 1;

	explicit Lexer(const String &p_source) :
			source(p_source) {}

	bool is_eof() const { return pos >= source.length(); }
	char32_t peek(int p_ofs = 0) const {
		const int i = pos + p_ofs;
		return i < source.length() ? source[i] : 0;
	}
	char32_t next() {
		if (is_eof()) {
			return 0;
		}
		char32_t c = source[pos++];
		if (c == '\n') {
			line++;
		}
		return c;
	}
	void skip_ws() {
		while (!is_eof()) {
			const char32_t c = peek();
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				next();
			} else if (c == '/' && peek(1) == '/') {
				next();
				next();
				while (!is_eof() && peek() != '\n') {
					next();
				}
			} else if (c == '/' && peek(1) == '*') {
				next();
				next();
				while (!is_eof() && !(peek() == '*' && peek(1) == '/')) {
					next();
				}
				if (!is_eof()) {
					next();
					next();
				}
			} else {
				break;
			}
		}
	}
	Token lex() {
		skip_ws();
		Token tk;
		tk.line = line;
		if (is_eof()) {
			return tk;
		}
		char32_t c = peek();
		if (is_digit(c) || (c == '.' && is_digit(peek(1)))) {
			tk.type = TK_NUMBER;
			String num;
			while (is_digit(peek()) || peek() == '.') {
				num += next();
			}
			tk.number = num.to_float();
			tk.text = num;
			return tk;
		}
		if (c == '"') {
			next();
			tk.type = TK_STRING;
			while (!is_eof() && peek() != '"') {
				if (peek() == '\\') {
					next();
					tk.text += next();
				} else {
					tk.text += next();
				}
			}
			if (!is_eof()) {
				next();
			}
			return tk;
		}
		if (is_ascii_alphabet_char(c) || c == '_' || c == '@') {
			tk.type = TK_IDENT;
			while (is_ascii_alphanumeric_char(peek()) || peek() == '_' || peek() == '@') {
				tk.text += next();
			}
			return tk;
		}
		if (String("=<>!+-*/;,.(){}[]").find_char(c) != -1) {
			tk.type = (c == ';' || c == ',' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']') ? TK_PUNCT : TK_OP;
			tk.text = String::chr(next());
			const char32_t c2 = peek();
			if ((tk.text == "=" && c2 == '=') || (tk.text == "!" && c2 == '=') || (tk.text == "<" && c2 == '=') || (tk.text == ">" && c2 == '=')) {
				tk.text += next();
			}
			return tk;
		}
		next();
		tk.type = TK_OP;
		tk.text = String::chr(c);
		return tk;
	}
};

static HashMap<String, CJavaParsedScript> g_cjava_export_registry;
static HashMap<String, CJavaParsedScript> g_cjava_class_registry;

static CJavaType _type_from_name(const String &p_type_name, const String &p_script_class = String()) {
	CJavaType type;
	if (p_type_name == "int") {
		type.builtin = Variant::INT;
	} else if (p_type_name == "double" || p_type_name == "float") {
		type.builtin = Variant::FLOAT;
	} else if (p_type_name == "boolean" || p_type_name == "bool") {
		type.builtin = Variant::BOOL;
	} else if (p_type_name == "String") {
		type.builtin = Variant::STRING;
	} else if (p_type_name == "void") {
		type.builtin = Variant::NIL;
	} else if (ClassDB::class_exists(StringName(p_type_name))) {
		type.builtin = Variant::OBJECT;
		type.native_class = StringName(p_type_name);
	} else if (g_cjava_class_registry.has(p_type_name)) {
		type.builtin = Variant::OBJECT;
		type.script_class = StringName(p_type_name);
	} else if (!p_script_class.is_empty() && p_type_name == p_script_class) {
		type.builtin = Variant::OBJECT;
		type.script_class = StringName(p_script_class);
	} else if (p_type_name == "Node") {
		type.builtin = Variant::OBJECT;
		type.native_class = Node::get_class_static();
	} else {
		type.builtin = Variant::OBJECT;
		type.native_class = StringName(p_type_name);
	}
	return type;
}

static void _remap_field_indices(CJavaBytecodeModule &p_bc, int p_offset, int p_start = 0, int p_end = -1) {
	if (p_offset == 0) {
		return;
	}
	if (p_end < 0) {
		p_end = p_bc.code.size();
	}
	for (int i = p_start; i < p_end; i++) {
		const CJavaOpcode op = (CJavaOpcode)p_bc.code[i];
		if (op == OP_PUSH_FIELD || op == OP_STORE_FIELD) {
			if (i + 2 < p_end) {
				uint16_t idx = p_bc.code[i + 1] | (p_bc.code[i + 2] << 8);
				idx += p_offset;
				p_bc.code.write[i + 1] = idx & 0xFF;
				p_bc.code.write[i + 2] = (idx >> 8) & 0xFF;
			}
			i += 2;
		}
	}
}

static void _inherit_script(CJavaParsedScript &r_child, const CJavaParsedScript &p_base) {
	const int field_offset = p_base.fields.size();
	for (CJavaFieldDef &field : r_child.fields) {
		field.field_index += field_offset;
	}
	for (KeyValue<StringName, CJavaMethodDef> &E : r_child.methods) {
		_remap_field_indices(r_child.bytecode, field_offset, E.value.chunk.offset, E.value.chunk.offset + E.value.chunk.length);
	}
	Vector<CJavaFieldDef> merged_fields = p_base.fields;
	merged_fields.append_array(r_child.fields);
	r_child.fields = merged_fields;
	for (int i = 0; i < r_child.fields.size(); i++) {
		r_child.fields.write[i].field_index = i;
	}
	HashMap<StringName, CJavaMethodDef> merged_methods;
	for (const KeyValue<StringName, CJavaMethodDef> &E : p_base.methods) {
		merged_methods.insert(E.key, E.value);
	}
	for (const KeyValue<StringName, CJavaMethodDef> &E : r_child.methods) {
		merged_methods.insert(E.key, E.value);
	}
	r_child.methods = merged_methods;
	r_child.base_class = p_base.base_class;
	r_child.base_is_native = p_base.base_is_native;
	if (!p_base.base_script_path.is_empty()) {
		r_child.base_script_path = p_base.base_script_path;
		r_child.base_script_ref = p_base.base_script_ref;
	}
}

static CJavaParsedScript _compile_file(const String &p_source, const String &p_path, bool p_require_lifecycle);
static CJavaParsedScript _resolve_script_ref(const String &p_ref, const String &p_path);

static String _resolve_path(const String &p_base_path, const String &p_include_path) {
	if (p_include_path.is_absolute_path()) {
		return p_include_path;
	}
	if (!p_base_path.is_empty()) {
		return p_base_path.get_base_dir().path_join(p_include_path);
	}
	return p_include_path;
}

static String _preprocess_source(const String &p_source, const String &p_path, HashSet<String> &r_visited, Vector<String> &r_includes, String &r_error) {
	String out;
	int line_start = 0;
	const int len = p_source.length();
	for (int i = 0; i <= len; i++) {
		if (i == len || p_source[i] == '\n') {
			String line = p_source.substr(line_start, i - line_start).strip_edges();
			if (line.begins_with("@include")) {
				const int q1 = line.find_char('"');
				const int q2 = line.find_char('"', q1 + 1);
				if (q1 == -1 || q2 == -1) {
					r_error = "Invalid @include syntax";
					return String();
				}
				const String inc = line.substr(q1 + 1, q2 - q1 - 1);
				const String abs = _resolve_path(p_path, inc);
				if (r_visited.has(abs)) {
					r_error = vformat("Circular @include: %s", abs);
					return String();
				}
				r_visited.insert(abs);
				r_includes.push_back(abs);
				Error err = OK;
				const String inc_src = FileAccess::get_file_as_string(abs, &err);
				if (err != OK) {
					r_error = vformat("Failed to @include '%s'", abs);
					return String();
				}
				out += _preprocess_source(inc_src, abs, r_visited, r_includes, r_error);
				if (!r_error.is_empty()) {
					return String();
				}
			} else if (line.begins_with("@importinside") || line.begins_with("@importout") || line.begins_with("@export") || line.begins_with("@own") || line.begins_with("@borrow")) {
				out += line + "\n";
			} else {
				out += p_source.substr(line_start, i - line_start + (i < len ? 1 : 0));
			}
			line_start = i + 1;
		}
	}
	return out;
}

class Compiler {
	Vector<Token> tokens;
	int token_pos = 0;
	CJavaParsedScript *result = nullptr;
	CJavaBytecodeModule *bc = nullptr;
	CJavaMethodDef *method = nullptr;
	String error;
	CJavaType last_expr_type;
	CJavaOwnership last_expr_ownership = CJAVA_OWNERSHIP_NONE;

	Token &cur() { return tokens.write[token_pos]; }
	const Token &cur() const { return tokens[token_pos]; }
	void advance() {
		if (token_pos < tokens.size()) {
			token_pos++;
		}
	}
	bool match(const String &p_text) {
		if (cur().type != TK_IDENT && cur().type != TK_OP && cur().type != TK_PUNCT) {
			return false;
		}
		if (cur().text != p_text) {
			return false;
		}
		advance();
		return true;
	}
	bool expect(const String &p_text) {
		if (!match(p_text)) {
			error = vformat("Expected '%s' near line %d", p_text, cur().line);
			return false;
		}
		return true;
	}

	void fail(const String &p_msg) { error = p_msg; }

	bool check_type(const CJavaType &p_expected, const CJavaType &p_actual, const String &p_context) {
		if (p_expected.is_void() || p_actual.is_void()) {
			return true;
		}
		if (!p_expected.compatible_with(p_actual)) {
			fail(vformat("Type mismatch in %s: expected %s, got %s near line %d", p_context, p_expected.to_string(), p_actual.to_string(), cur().line));
			return false;
		}
		return true;
	}

	bool check_ownership_assign(CJavaOwnership p_target, CJavaOwnership p_source, const String &p_context) {
		if (p_target == CJAVA_OWNERSHIP_OWNED && p_source == CJAVA_OWNERSHIP_BORROWED) {
			fail(vformat("Cannot assign @borrow reference into @own target in %s near line %d", p_context, cur().line));
			return false;
		}
		return true;
	}

	CJavaType type_for_field(int p_index) const {
		if (p_index >= 0 && p_index < result->fields.size()) {
			return result->fields[p_index].type;
		}
		return CJavaType();
	}

	CJavaOwnership ownership_for_field(int p_index) const {
		if (p_index >= 0 && p_index < result->fields.size()) {
			return result->fields[p_index].ownership;
		}
		return CJAVA_OWNERSHIP_NONE;
	}

	CJavaType type_for_name(const StringName &p_name) const {
		const int fi = field_index(p_name);
		if (fi != -1) {
			return type_for_field(fi);
		}
		if (method && method->local_defs.has(p_name)) {
			return method->local_defs[p_name].type;
		}
		const int ai = arg_index(p_name);
		if (ai != -1 && ai < method->arg_types.size()) {
			return method->arg_types[ai];
		}
		return CJavaType();
	}

	CJavaOwnership ownership_for_name(const StringName &p_name) const {
		const int fi = field_index(p_name);
		if (fi != -1) {
			return ownership_for_field(fi);
		}
		if (method && method->local_defs.has(p_name)) {
			return method->local_defs[p_name].ownership;
		}
		return CJAVA_OWNERSHIP_NONE;
	}

	Variant default_for_type(const CJavaType &p_type) {
		switch (p_type.builtin) {
			case Variant::INT:
				return 0;
			case Variant::FLOAT:
				return 0.0;
			case Variant::BOOL:
				return false;
			case Variant::STRING:
				return String();
			default:
				return Variant();
		}
	}

	bool parse_modifiers(bool &r_export_field, CJavaOwnership &r_ownership) {
		while (cur().type == TK_IDENT && cur().text.begins_with("@")) {
			if (cur().text == "@export") {
				r_export_field = true;
				advance();
			} else if (cur().text == "@own") {
				r_ownership = CJAVA_OWNERSHIP_OWNED;
				advance();
			} else if (cur().text == "@borrow") {
				r_ownership = CJAVA_OWNERSHIP_BORROWED;
				advance();
			} else {
				break;
			}
		}
		return true;
	}

	Variant::Type parse_type_name(const String &p_type) {
		if (p_type == "int") {
			return Variant::INT;
		}
		if (p_type == "double" || p_type == "float") {
			return Variant::FLOAT;
		}
		if (p_type == "boolean" || p_type == "bool") {
			return Variant::BOOL;
		}
		if (p_type == "String") {
			return Variant::STRING;
		}
		return Variant::OBJECT;
	}

	Variant default_for_type(Variant::Type p_type) {
		switch (p_type) {
			case Variant::INT:
				return 0;
			case Variant::FLOAT:
				return 0.0;
			case Variant::BOOL:
				return false;
			case Variant::STRING:
				return String();
			default:
				return Variant();
		}
	}

	int ensure_local(const StringName &p_name) {
		if (method->local_slots.has(p_name)) {
			return method->local_slots[p_name];
		}
		const int idx = method->local_slots.size();
		method->local_slots.insert(p_name, idx);
		method->local_slot_defs.resize(idx + 1);
		return idx;
	}

	int field_index(const StringName &p_name) const {
		for (int i = 0; i < result->fields.size(); i++) {
			if (result->fields[i].name == p_name) {
				return i;
			}
		}
		return -1;
	}

	int arg_index(const StringName &p_name) const {
		for (int i = 0; i < method->arg_names.size(); i++) {
			if (method->arg_names[i] == p_name) {
				return i;
			}
		}
		return -1;
	}

	bool push_name_ref(const StringName &p_name) {
		const int fi = field_index(p_name);
		if (fi != -1) {
			bc->emit_opcode(OP_PUSH_FIELD);
			bc->emit_u16(fi);
			last_expr_type = type_for_field(fi);
			last_expr_ownership = ownership_for_field(fi);
			return true;
		}
		if (method->local_slots.has(p_name)) {
			bc->emit_opcode(OP_PUSH_LOCAL);
			bc->emit_u16(method->local_slots[p_name]);
			last_expr_type = method->local_defs.has(p_name) ? method->local_defs[p_name].type : CJavaType();
			last_expr_ownership = method->local_defs.has(p_name) ? method->local_defs[p_name].ownership : CJAVA_OWNERSHIP_NONE;
			return true;
		}
		const int ai = arg_index(p_name);
		if (ai != -1) {
			bc->emit_opcode(OP_PUSH_ARG);
			bc->emit_u16(ai);
			last_expr_type = ai < method->arg_types.size() ? method->arg_types[ai] : CJavaType();
			last_expr_ownership = CJAVA_OWNERSHIP_BORROWED;
			return true;
		}
		fail(vformat("Unknown identifier '%s'", String(p_name)));
		last_expr_type = CJavaType();
		return false;
	}

	void compile_expr(int p_precedence = 0) { compile_cmp(); }

	void compile_primary() {
		if (cur().type == TK_NUMBER) {
			const int idx = bc->add_constant(cur().number);
			bc->emit_opcode(OP_PUSH_CONST);
			bc->emit_u32(idx);
			last_expr_type = _type_from_name(cur().text.contains(".") ? "double" : "int");
			last_expr_ownership = CJAVA_OWNERSHIP_NONE;
			advance();
			return;
		}
		if (cur().type == TK_STRING) {
			const int idx = bc->add_constant(cur().text);
			bc->emit_opcode(OP_PUSH_CONST);
			bc->emit_u32(idx);
			last_expr_type = _type_from_name("String");
			last_expr_ownership = CJAVA_OWNERSHIP_NONE;
			advance();
			return;
		}
		if (cur().type == TK_IDENT) {
			const String name = cur().text;
			if (name == "true" || name == "false") {
				const int idx = bc->add_constant(name == "true");
				bc->emit_opcode(OP_PUSH_CONST);
				bc->emit_u32(idx);
				last_expr_type = _type_from_name("boolean");
				last_expr_ownership = CJAVA_OWNERSHIP_NONE;
				advance();
				return;
			}
			if (name == "super") {
				advance();
				expect(".");
				const StringName method_name = StringName(cur().text);
				advance();
				expect("(");
				uint8_t argc = 0;
				if (!match(")")) {
					do {
						compile_expr();
						argc++;
					} while (match(","));
					expect(")");
				}
				const int name_idx = bc->add_constant(method_name);
				bc->emit_opcode(OP_CALL_SUPER);
				bc->emit_u32(name_idx);
				bc->emit_u8(argc);
				last_expr_type = CJavaType();
				return;
			}
			advance();
			if (match("(")) {
				Vector<int> args;
				if (!match(")")) {
					do {
						compile_expr();
						args.push_back(1);
					} while (match(","));
					expect(")");
				}
				if (name == "print") {
					bc->emit_opcode(OP_CALL_PRINT);
					bc->emit_u8(args.size());
				} else if (name == "get_node" || name == "getNode") {
					if (args.size() != 1) {
						fail("get_node expects 1 argument");
						return;
					}
					bc->emit_opcode(OP_CALL_GET_NODE);
					last_expr_type = _type_from_name("Node");
					last_expr_ownership = CJAVA_OWNERSHIP_OWNED;
				} else {
					fail(vformat("Unknown builtin '%s'", name));
				}
				return;
			}
			if (match(".")) {
				const StringName method_name = StringName(cur().text);
				advance();
				expect("(");
				uint8_t argc = 0;
				if (!match(")")) {
					do {
						compile_expr();
						argc++;
					} while (match(","));
					expect(")");
				}
				if (!push_name_ref(StringName(name))) {
					return;
				}
				const int name_idx = bc->add_constant(method_name);
				bc->emit_opcode(OP_CALL_METHOD);
				bc->emit_u32(name_idx);
				bc->emit_u8(argc);
				return;
			}
			push_name_ref(StringName(name));
			return;
		}
		if (match("(")) {
			compile_expr();
			expect(")");
			return;
		}
		fail(vformat("Unexpected token '%s'", cur().text));
	}

	void compile_unary() {
		if (match("-")) {
			compile_unary();
			bc->emit_opcode(OP_NEG);
			return;
		}
		if (match("!")) {
			compile_unary();
			bc->emit_opcode(OP_NOT);
			return;
		}
		compile_primary();
	}

	void compile_mul() {
		compile_unary();
		while (cur().text == "*" || cur().text == "/") {
			const String op = cur().text;
			advance();
			compile_unary();
			bc->emit_opcode(op == "*" ? OP_MUL : OP_DIV);
		}
	}

	void compile_add() {
		compile_mul();
		while (cur().text == "+" || cur().text == "-") {
			const String op = cur().text;
			advance();
			compile_mul();
			bc->emit_opcode(op == "+" ? OP_ADD : OP_SUB);
		}
	}

	void compile_cmp() {
		compile_add();
		while (true) {
			CJavaOpcode op = OP_NOP;
			if (match("==")) {
				op = OP_CMP_EQ;
			} else if (match("!=")) {
				op = OP_CMP_NE;
			} else if (match("<=")) {
				op = OP_CMP_LE;
			} else if (match(">=")) {
				op = OP_CMP_GE;
			} else if (match("<")) {
				op = OP_CMP_LT;
			} else if (match(">")) {
				op = OP_CMP_GT;
			} else {
				break;
			}
			compile_add();
			bc->emit_opcode(op);
		}
	}

	void compile_stmt() {
		if (cur().type == TK_IDENT && (cur().text == "int" || cur().text == "double" || cur().text == "float" || cur().text == "boolean" || cur().text == "bool" || cur().text == "String" || cur().text == "Node" || ClassDB::class_exists(StringName(cur().text)) || g_cjava_class_registry.has(cur().text))) {
			CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;
			bool ignored_export = false;
			parse_modifiers(ignored_export, ownership);
			const String type_name = cur().text;
			const CJavaType declared_type = _type_from_name(type_name, result->global_class_name);
			advance();
			const StringName var_name = StringName(cur().text);
			advance();
			if (ownership == CJAVA_OWNERSHIP_NONE && declared_type.is_object_like()) {
				ownership = CJAVA_OWNERSHIP_OWNED;
			}
			if (match("=")) {
				compile_expr();
				if (!check_type(declared_type, last_expr_type, vformat("local '%s'", var_name))) {
					return;
				}
				const int local_idx = ensure_local(var_name);
				CJavaLocalDef def;
				def.type = declared_type;
				def.ownership = ownership;
				method->local_defs[var_name] = def;
				method->local_slot_defs.write[local_idx] = def;
				bc->emit_opcode(OP_STORE_LOCAL);
				bc->emit_u16(local_idx);
			} else {
				CJavaLocalDef def;
				def.type = declared_type;
				def.ownership = ownership;
				const int local_idx = ensure_local(var_name);
				method->local_defs[var_name] = def;
				method->local_slot_defs.write[local_idx] = def;
			}
			expect(";");
			return;
		}
		if (cur().type == TK_IDENT && cur().text == "if") {
			advance();
			expect("(");
			compile_expr();
			expect(")");
			const int jmp_false = bc->emit_jump(OP_JUMP_IF_FALSE);
			compile_block();
			int jmp_end = -1;
			if (match("else")) {
				jmp_end = bc->emit_jump(OP_JUMP);
				bc->patch_jump(jmp_false, bc->code.size());
				compile_block();
			} else {
				bc->patch_jump(jmp_false, bc->code.size());
			}
			if (jmp_end != -1) {
				bc->patch_jump(jmp_end, bc->code.size());
			}
			return;
		}
		if (cur().type == TK_IDENT && cur().text == "for") {
			advance();
			expect("(");
			compile_stmt(); // init
			const int loop_start = bc->code.size();
			compile_expr(); // condition
			const int jmp_exit = bc->emit_jump(OP_JUMP_IF_FALSE);
			expect(";");
			compile_stmt(); // increment
			expect(")");
			compile_block(); // body
			bc->emit_opcode(OP_JUMP);
			bc->emit_i32(loop_start - bc->code.size());
			bc->patch_jump(jmp_exit, bc->code.size());
			return;
		}
		if (cur().type == TK_IDENT) {
			const StringName lhs = StringName(cur().text);
			advance();
			if (match("=")) {
				compile_expr();
				const int fi = field_index(lhs);
				if (fi != -1) {
					if (!check_type(type_for_field(fi), last_expr_type, vformat("field '%s'", lhs))) {
						return;
					}
					if (!check_ownership_assign(ownership_for_field(fi), last_expr_ownership, vformat("field '%s'", lhs))) {
						return;
					}
					bc->emit_opcode(OP_STORE_FIELD);
					bc->emit_u16(fi);
				} else {
					const CJavaType lhs_type = type_for_name(lhs);
					if (!lhs_type.is_void() && !check_type(lhs_type, last_expr_type, vformat("local '%s'", lhs))) {
						return;
					}
					CJavaOwnership target_ownership = method->local_defs.has(lhs) ? method->local_defs[lhs].ownership : CJAVA_OWNERSHIP_NONE;
					if (!check_ownership_assign(target_ownership, last_expr_ownership, vformat("local '%s'", lhs))) {
						return;
					}
					if (!method->local_defs.has(lhs)) {
						CJavaLocalDef def;
						def.type = lhs_type;
						def.ownership = last_expr_ownership == CJAVA_OWNERSHIP_NONE ? CJAVA_OWNERSHIP_NONE : last_expr_ownership;
						const int li = ensure_local(lhs);
						method->local_defs[lhs] = def;
						method->local_slot_defs.write[li] = def;
					}
					const int li = method->local_slots[lhs];
					bc->emit_opcode(OP_STORE_LOCAL);
					bc->emit_u16(li);
				}
				expect(";");
				return;
			}
			if (match("(")) {
				token_pos--;
				cur();
				compile_expr();
				bc->emit_opcode(OP_POP);
				expect(";");
				return;
			}
		}
		if (match("{")) {
			token_pos--;
			compile_block();
			return;
		}
		compile_expr();
		bc->emit_opcode(OP_POP);
		expect(";");
	}

	void compile_block() {
		expect("{");
		while (cur().type != TK_EOF && cur().text != "}") {
			compile_stmt();
			if (!error.is_empty()) {
				return;
			}
		}
		expect("}");
	}

	bool parse_directives() {
		while (cur().type == TK_IDENT && cur().text.begins_with("@")) {
			const String directive = cur().text;
			advance();
			if (directive == "@importout") {
				expect("(");
				if (cur().type != TK_STRING) {
					fail("@importout requires string name");
					return false;
				}
				result->import_export_name = StringName(cur().text);
				advance();
				expect(")");
				expect(";");
			} else if (directive == "@importinside") {
				expect("(");
				if (cur().type != TK_STRING) {
					fail("@importinside requires string");
					return false;
				}
				const String target = cur().text;
				advance();
				expect(")");
				expect(";");
				result->import_inside_targets.push_back(target);
			} else {
				token_pos--;
				break;
			}
		}
		return true;
	}

	bool parse_extends() {
		if (!(cur().type == TK_IDENT && cur().text == "extends")) {
			if (cur().type == TK_IDENT && cur().text == "class") {
				advance();
				result->global_class_name = StringName(cur().text);
				advance();
				expect("extends");
			} else {
				fail("Expected extends");
				return false;
			}
		} else {
			advance();
		}

		if (cur().type == TK_STRING) {
			result->base_script_ref = cur().text;
			result->base_is_native = false;
			advance();
		} else if (cur().type == TK_IDENT) {
			const String base_name = cur().text;
			advance();
			if (ClassDB::class_exists(StringName(base_name))) {
				result->base_class = StringName(base_name);
				result->base_is_native = true;
			} else {
				result->base_script_ref = base_name;
				result->base_is_native = false;
			}
		} else {
			fail("Expected base class name or script path");
			return false;
		}
		match(";");
		return true;
	}

	bool parse_field() {
		bool export_field = false;
		CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;
		if (cur().type == TK_IDENT && cur().text.begins_with("@")) {
			parse_modifiers(export_field, ownership);
		}
		if (cur().type != TK_IDENT) {
			return false;
		}
		const String type_name = cur().text;
		if (type_name == "void" || type_name == "extends" || type_name == "class" || type_name.begins_with("@")) {
			return false;
		}
		const CJavaType field_type = _type_from_name(type_name, result->global_class_name);
		if (field_type.is_void()) {
			return false;
		}
		advance();
		CJavaFieldDef field;
		field.name = StringName(cur().text);
		field.type = field_type;
		field.export_to_editor = export_field;
		if (ownership == CJAVA_OWNERSHIP_NONE && field_type.is_object_like()) {
			ownership = CJAVA_OWNERSHIP_OWNED;
		}
		field.ownership = ownership;
		field.field_index = result->fields.size();
		advance();
		field.default_value = default_for_type(field.type);
		if (match("=")) {
			if (cur().type == TK_NUMBER) {
				field.default_value = cur().number;
				last_expr_type = _type_from_name(cur().text.contains(".") ? "double" : "int");
				advance();
			} else if (cur().type == TK_STRING) {
				field.default_value = cur().text;
				last_expr_type = _type_from_name("String");
				advance();
			} else if (cur().text == "true" || cur().text == "false") {
				field.default_value = cur().text == "true";
				last_expr_type = _type_from_name("boolean");
				advance();
			}
			if (!check_type(field.type, last_expr_type, vformat("field '%s' default", field.name))) {
				return false;
			}
		}
		expect(";");
		result->fields.push_back(field);
		return true;
	}

	bool parse_method() {
		if (!(cur().type == TK_IDENT && cur().text == "void")) {
			return false;
		}
		advance();
		CJavaMethodDef mdef;
		mdef.name = StringName(cur().text);
		advance();
		expect("(");
		if (!match(")")) {
			do {
				CJavaType arg_type;
				if (cur().type == TK_IDENT) {
					arg_type = _type_from_name(cur().text, result->global_class_name);
					advance();
				}
				if (cur().type == TK_IDENT) {
					mdef.arg_names.push_back(cur().text);
					mdef.arg_types.push_back(arg_type);
					advance();
				}
			} while (match(","));
			expect(")");
		}
		method = &mdef;
		bc = &result->bytecode;
		const int start = bc->code.size();
		compile_block();
		bc->emit_opcode(OP_END);
		mdef.chunk = bc->end_chunk(start);
		method = nullptr;
		if (!error.is_empty()) {
			return false;
		}
		result->methods[mdef.name] = mdef;
		return true;
	}

	bool require_lifecycle = true;

public:
	bool run(const String &p_source, CJavaParsedScript &r_result, bool p_require_lifecycle = true) {
		require_lifecycle = p_require_lifecycle;
		result = &r_result;
		Lexer lex(p_source);
		while (true) {
			Token t = lex.lex();
			if (t.type == TK_EOF) {
				tokens.push_back(t);
				break;
			}
			tokens.push_back(t);
		}
		token_pos = 0;
		bc = &result->bytecode;
		if (!parse_directives() || !parse_extends()) {
			return false;
		}
		while (cur().type != TK_EOF) {
			if (parse_field()) {
				continue;
			}
			if (parse_method()) {
				continue;
			}
			fail(vformat("Unexpected token '%s' at line %d", cur().text, cur().line));
			return false;
		}
		if (require_lifecycle && !result->methods.has("_ready") && !result->methods.has("_process")) {
			fail("Define at least _ready() or _process()");
			return false;
		}
		return error.is_empty();
	}

	String get_error() const { return error; }
};

static void _merge_import(CJavaParsedScript &r_dst, const CJavaParsedScript &p_src) {
	const int const_base = r_dst.bytecode.constants.size();
	const int code_base = r_dst.bytecode.code.size();
	for (const Variant &c : p_src.bytecode.constants) {
		r_dst.bytecode.constants.push_back(c);
	}
	for (uint8_t b : p_src.bytecode.code) {
		r_dst.bytecode.code.push_back(b);
	}
	for (int i = 0; i < r_dst.bytecode.code.size(); i++) {
		if (i < code_base) {
			if (r_dst.bytecode.code[i] == (uint8_t)OP_PUSH_CONST && i + 4 < r_dst.bytecode.code.size()) {
				uint32_t idx = r_dst.bytecode.code[i + 1] | (r_dst.bytecode.code[i + 2] << 8) | (r_dst.bytecode.code[i + 3] << 16) | (r_dst.bytecode.code[i + 4] << 24);
				if (idx < (uint32_t)const_base) {
					continue;
				}
			}
		}
	}
	for (const CJavaFieldDef &f : p_src.fields) {
		CJavaFieldDef nf = f;
		nf.field_index = r_dst.fields.size();
		r_dst.fields.push_back(nf);
	}
	for (const KeyValue<StringName, CJavaMethodDef> &E : p_src.methods) {
		if (r_dst.methods.has(E.key)) {
			continue;
		}
		CJavaMethodDef m = E.value;
		m.chunk.offset += code_base;
		r_dst.methods.insert(E.key, m);
	}
}

static CJavaParsedScript _compile_file(const String &p_source, const String &p_path, bool p_require_lifecycle) {
	CJavaParsedScript result;
	HashSet<String> visited;
	Vector<String> includes;
	String preprocess_error;
	const String expanded = _preprocess_source(p_source, p_path, visited, includes, preprocess_error);
	if (!preprocess_error.is_empty()) {
		result.error_message = preprocess_error;
		return result;
	}
	result.included_paths = includes;
	Compiler compiler;
	if (!compiler.run(expanded, result, p_require_lifecycle)) {
		result.error_message = compiler.get_error();
		return result;
	}
	if (result.base_is_native) {
		if (!result.base_class.is_empty() && !ClassDB::class_exists(result.base_class)) {
			result.error_message = vformat("Unknown native base class '%s'", result.base_class);
			return result;
		}
	} else if (!result.base_script_ref.is_empty()) {
		const CJavaParsedScript base = _resolve_script_ref(result.base_script_ref, p_path);
		if (!base.valid) {
			result.error_message = vformat("Failed to resolve base script '%s': %s", result.base_script_ref, base.error_message);
			return result;
		}
		result.base_script_path = base.base_script_path.is_empty() ? _resolve_path(p_path, result.base_script_ref.ends_with(".cjava") ? result.base_script_ref : result.base_script_ref + ".cjava") : base.base_script_path;
		_inherit_script(result, base);
		if (result.base_class.is_empty()) {
			result.base_class = base.base_class;
		}
	}
	result.valid = true;
	return result;
}

static CJavaParsedScript _resolve_script_ref(const String &p_ref, const String &p_path) {
	if (g_cjava_class_registry.has(p_ref)) {
		return g_cjava_class_registry[p_ref];
	}
	String candidate = p_ref;
	if (!candidate.ends_with(".cjava")) {
		candidate = p_ref + ".cjava";
	}
	const String abs = _resolve_path(p_path, candidate);
	if (FileAccess::exists(abs)) {
		Error err = OK;
		const String src = FileAccess::get_file_as_string(abs, &err);
		if (err == OK) {
			CJavaParsedScript parsed = _compile_file(src, abs, false);
			parsed.base_script_path = abs;
			return parsed;
		}
	}
	CJavaParsedScript invalid;
	invalid.error_message = vformat("Unknown script class or path '%s'", p_ref);
	return invalid;
}

} // namespace

CJavaParsedScript CJavaCompiler::compile(const String &p_source, const String &p_path) {
	CJavaParsedScript result = _compile_file(p_source, p_path, true);
	if (!result.valid) {
		return result;
	}
	for (const String &target : result.import_inside_targets) {
		if (target.ends_with(".cjava")) {
			const String abs = _resolve_path(p_path, target);
			Error err = OK;
			const String src = FileAccess::get_file_as_string(abs, &err);
			if (err != OK) {
				result.valid = false;
				result.error_message = vformat("Failed to @importinside '%s'", abs);
				return result;
			}
			const CJavaParsedScript imported = _compile_file(src, abs, false);
			if (!imported.valid) {
				result.valid = false;
				result.error_message = vformat("@importinside '%s': %s", abs, imported.error_message);
				return result;
			}
			_merge_import(result, imported);
		} else if (g_cjava_export_registry.has(target)) {
			_merge_import(result, g_cjava_export_registry[target]);
		} else {
			result.valid = false;
			result.error_message = vformat("Unknown @importinside target '%s'", target);
			return result;
		}
	}
	if (!result.import_export_name.is_empty()) {
		g_cjava_export_registry[result.import_export_name] = result;
	}
	if (!result.global_class_name.is_empty()) {
		g_cjava_class_registry[String(result.global_class_name)] = result;
	}
	result.valid = true;
	return result;
}
