/**************************************************************************/
/*  cjava_types.cpp                                                       */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_types.h"

#include "core/object/class_db.h"

bool CJavaType::compatible_with(const CJavaType &p_other) const {
	if (p_other.is_void()) {
		return true;
	}
	if (is_void()) {
		return false;
	}
	if (builtin != Variant::NIL && p_other.builtin != Variant::NIL) {
		if (builtin == p_other.builtin) {
			return true;
		}
		if (is_numeric() && p_other.is_numeric()) {
			return true;
		}
		return false;
	}
	if (!script_class.is_empty() || !p_other.script_class.is_empty()) {
		if (!script_class.is_empty() && !p_other.script_class.is_empty()) {
			return script_class == p_other.script_class;
		}
		return false;
	}
	if (!native_class.is_empty() && !p_other.native_class.is_empty()) {
		return ClassDB::is_parent_class(p_other.native_class, native_class) || ClassDB::is_parent_class(native_class, p_other.native_class) || native_class == p_other.native_class;
	}
	if (builtin == Variant::OBJECT && p_other.is_object_like()) {
		return true;
	}
	return false;
}

String CJavaType::to_string() const {
	if (!script_class.is_empty()) {
		return String(script_class);
	}
	if (!native_class.is_empty()) {
		return String(native_class);
	}
	switch (builtin) {
		case Variant::INT:
			return "int";
		case Variant::FLOAT:
			return "double";
		case Variant::BOOL:
			return "boolean";
		case Variant::STRING:
			return "String";
		case Variant::OBJECT:
			return "Object";
		default:
			return "void";
	}
}
