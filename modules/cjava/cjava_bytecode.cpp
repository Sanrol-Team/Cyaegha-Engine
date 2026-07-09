/**************************************************************************/
/*  cjava_bytecode.cpp                                                    */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_bytecode.h"

int CJavaBytecodeModule::add_constant(const Variant &p_value) {
	constants.push_back(p_value);
	return constants.size() - 1;
}

int CJavaBytecodeModule::add_name(const StringName &p_name) {
	names.push_back(p_name);
	return names.size() - 1;
}

void CJavaBytecodeModule::emit_u8(uint8_t p_value) {
	code.push_back(p_value);
}

void CJavaBytecodeModule::emit_u16(uint16_t p_value) {
	code.push_back(p_value & 0xFF);
	code.push_back((p_value >> 8) & 0xFF);
}

void CJavaBytecodeModule::emit_u32(uint32_t p_value) {
	code.push_back(p_value & 0xFF);
	code.push_back((p_value >> 8) & 0xFF);
	code.push_back((p_value >> 16) & 0xFF);
	code.push_back((p_value >> 24) & 0xFF);
}

void CJavaBytecodeModule::emit_i32(int32_t p_value) {
	emit_u32((uint32_t)p_value);
}

void CJavaBytecodeModule::emit_opcode(CJavaOpcode p_op) {
	emit_u8((uint8_t)p_op);
}

int CJavaBytecodeModule::emit_jump(CJavaOpcode p_jump_op) {
	emit_opcode(p_jump_op);
	const int pos = code.size();
	emit_i32(0);
	return pos;
}

void CJavaBytecodeModule::patch_jump(int p_jump_emit_pos, int p_target_pos) {
	const int32_t rel = p_target_pos - (p_jump_emit_pos + 4);
	code.write[p_jump_emit_pos] = rel & 0xFF;
	code.write[p_jump_emit_pos + 1] = (rel >> 8) & 0xFF;
	code.write[p_jump_emit_pos + 2] = (rel >> 16) & 0xFF;
	code.write[p_jump_emit_pos + 3] = (rel >> 24) & 0xFF;
}

CJavaBytecodeChunk CJavaBytecodeModule::end_chunk(int p_start) {
	CJavaBytecodeChunk chunk;
	chunk.offset = p_start;
	chunk.length = code.size() - p_start;
	return chunk;
}
