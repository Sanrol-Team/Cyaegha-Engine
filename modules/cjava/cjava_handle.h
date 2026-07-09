/**************************************************************************/
/*  cjava_handle.h                                                        */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_types.h"

#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"

// Script-side object reference without GC: stores ObjectID only.
struct CJavaHandle {
	enum Kind : uint8_t {
		NONE,
		OBJECT,
	};

	Kind kind = NONE;
	ObjectID object_id;
	CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;

	static CJavaHandle from_object(Object *p_object, CJavaOwnership p_ownership = CJAVA_OWNERSHIP_OWNED);
	Object *resolve() const;
	bool is_valid() const;
};

struct CJavaOwnershipBinding {
	StringName symbol;
	CJavaOwnership ownership = CJAVA_OWNERSHIP_NONE;
	int handle_index = -1;
};

// Per-script-instance ownership arena (freed with the instance, no GC).
class CJavaOwnershipArena {
	Vector<CJavaHandle> handles;
	HashMap<StringName, CJavaOwnershipBinding> bindings;

public:
	int track(const StringName &p_symbol, const CJavaHandle &p_handle, CJavaOwnership p_ownership);
	void release_symbol(const StringName &p_symbol);
	bool has_binding(const StringName &p_symbol) const;
	CJavaOwnership get_ownership(const StringName &p_symbol) const;
	const CJavaHandle *get_handle_for_symbol(const StringName &p_symbol) const;
	void clear();
	int get_handle_count() const { return handles.size(); }
	const CJavaHandle &get_handle(int p_index) const { return handles[p_index]; }
};
