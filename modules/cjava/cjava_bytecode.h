/**************************************************************************/
/*  cjava_bytecode.h                                                      */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_types.h"

#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"

enum CJavaOpcode : uint8_t {
	OP_NOP = 0,
	OP_PUSH_CONST, // u32
	OP_PUSH_LOCAL, // u16
	OP_PUSH_FIELD, // u16
	OP_PUSH_ARG, // u16
	OP_POP,
	OP_STORE_LOCAL, // u16
	OP_STORE_FIELD, // u16
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_NEG,
	OP_NOT,
	OP_CMP_EQ,
	OP_CMP_NE,
	OP_CMP_LT,
	OP_CMP_LE,
	OP_CMP_GT,
	OP_CMP_GE,
	OP_JUMP, // i32
	OP_JUMP_IF_FALSE, // i32
	OP_CALL_PRINT, // argc u8
	OP_CALL_GET_NODE, // pops path string
	OP_CALL_METHOD, // u32 name_const, u8 argc
	OP_CALL_SUPER, // u32 method_name_const, u8 argc
	OP_END,
};

struct CJavaBytecodeChunk {
	int offset = 0;
	int length = 0;
};

struct CJavaBytecodeModule {
	Vector<Variant> constants;
	Vector<StringName> names;
	Vector<uint8_t> code;

	int add_constant(const Variant &p_value);
	int add_name(const StringName &p_name);
	void emit_u8(uint8_t p_value);
	void emit_u16(uint16_t p_value);
	void emit_u32(uint32_t p_value);
	void emit_i32(int32_t p_value);
	void emit_opcode(CJavaOpcode p_op);
	int emit_jump(CJavaOpcode p_jump_op);
	void patch_jump(int p_jump_emit_pos, int p_target_pos);
	CJavaBytecodeChunk end_chunk(int p_start);
};

struct CJavaFieldDef {
	StringName name;
	CJavaType type;
	Variant default_value;
	bool export_to_editor = false;
	CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;
	int field_index = 0;
};

struct CJavaMethodDef {
	StringName name;
	PackedStringArray arg_names;
	Vector<CJavaType> arg_types;
	CJavaBytecodeChunk chunk;
	HashMap<StringName, int> local_slots;
	HashMap<StringName, CJavaLocalDef> local_defs;
	Vector<CJavaLocalDef> local_slot_defs; // Index-aligned metadata for locals.
};

struct CJavaParsedScript {
	bool valid = false;
	String error_message;
	StringName base_class; // Native Godot class at inheritance root.
	StringName global_class_name;
	String import_export_name;
	String base_script_ref; // Another .cjava class/path when extending script.
	String base_script_path; // Resolved absolute path.
	bool base_is_native = true;
	Vector<CJavaFieldDef> fields;
	HashMap<StringName, CJavaMethodDef> methods;
	CJavaBytecodeModule bytecode;
	Vector<String> included_paths;
	Vector<String> import_inside_targets;
};
