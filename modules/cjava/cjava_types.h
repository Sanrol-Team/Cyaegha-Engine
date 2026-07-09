/**************************************************************************/
/*  cjava_types.h                                                         */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "core/string/string_name.h"
#include "core/variant/variant.h"

enum CJavaOwnership : uint8_t {
	CJAVA_OWNERSHIP_NONE = 0,
	CJAVA_OWNERSHIP_OWNED, // Instance owns the handle until scope ends / overwrite.
	CJAVA_OWNERSHIP_BORROWED, // Non-owning reference; must not be stored into @own fields.
};

struct CJavaType {
	Variant::Type builtin = Variant::NIL;
	StringName native_class; // Node, RefCounted, ...
	StringName script_class; // Another .cjava global class name.
	bool is_nullable = true;

	bool is_void() const { return builtin == Variant::NIL && native_class.is_empty() && script_class.is_empty(); }
	bool is_numeric() const { return builtin == Variant::INT || builtin == Variant::FLOAT; }
	bool is_object_like() const { return builtin == Variant::OBJECT || !native_class.is_empty() || !script_class.is_empty(); }

	bool compatible_with(const CJavaType &p_other) const;
	String to_string() const;
};

struct CJavaLocalDef {
	CJavaType type;
	CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;
};
