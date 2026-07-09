/**************************************************************************/
/*  cjava_handle.cpp                                                      */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_handle.h"

#include "core/object/object.h"
#include "core/string/print_string.h"

CJavaHandle CJavaHandle::from_object(Object *p_object, CJavaOwnership p_ownership) {
	CJavaHandle handle;
	if (p_object) {
		handle.kind = OBJECT;
		handle.object_id = p_object->get_instance_id();
		handle.ownership = p_ownership;
	}
	return handle;
}

Object *CJavaHandle::resolve() const {
	if (kind != OBJECT) {
		return nullptr;
	}
	return ObjectDB::get_instance(object_id);
}

bool CJavaHandle::is_valid() const {
	return kind == OBJECT && ObjectDB::get_instance(object_id) != nullptr;
}

int CJavaOwnershipArena::track(const StringName &p_symbol, const CJavaHandle &p_handle, CJavaOwnership p_ownership) {
	CJavaHandle stored = p_handle;
	stored.ownership = p_ownership;
	const int index = handles.push_back(stored);
	CJavaOwnershipBinding binding;
	binding.symbol = p_symbol;
	binding.ownership = p_ownership;
	binding.handle_index = index;
	bindings[p_symbol] = binding;
	return index;
}

void CJavaOwnershipArena::release_symbol(const StringName &p_symbol) {
	bindings.erase(p_symbol);
}

bool CJavaOwnershipArena::has_binding(const StringName &p_symbol) const {
	return bindings.has(p_symbol);
}

CJavaOwnership CJavaOwnershipArena::get_ownership(const StringName &p_symbol) const {
	if (bindings.has(p_symbol)) {
		return bindings[p_symbol].ownership;
	}
	return CJAVA_OWNERSHIP_NONE;
}

const CJavaHandle *CJavaOwnershipArena::get_handle_for_symbol(const StringName &p_symbol) const {
	if (!bindings.has(p_symbol)) {
		return nullptr;
	}
	const int index = bindings[p_symbol].handle_index;
	if (index < 0 || index >= handles.size()) {
		return nullptr;
	}
	return &handles[index];
}

void CJavaOwnershipArena::clear() {
	handles.clear();
	bindings.clear();
}
