/**************************************************************************/
/*  cjava_resource_format.cpp                                             */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_resource_format.h"

#include "cjava_script.h"

#include "core/io/file_access.h"

Ref<Resource> ResourceFormatLoaderCJava::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(p_original_path.is_empty() ? p_path : p_original_path, FileAccess::READ, &err);
	if (file.is_null()) {
		if (r_error) {
			*r_error = err;
		}
		return Ref<Resource>();
	}

	Ref<CJavaScript> script;
	script.instantiate();
	script->set_path(p_original_path.is_empty() ? p_path : p_original_path);
	script->set_source_code(file->get_as_utf8_string());
	err = script->reload();
	if (r_error) {
		*r_error = err;
	}
	return script;
}

void ResourceFormatLoaderCJava::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("cjava");
}

bool ResourceFormatLoaderCJava::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "CJavaScript";
}

String ResourceFormatLoaderCJava::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "cjava") {
		return "CJavaScript";
	}
	return "";
}

Error ResourceFormatSaverCJava::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<CJavaScript> script = p_resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot save CJava script '%s'.", p_path));
	file->store_string(script->get_source_code());
	return OK;
}

void ResourceFormatSaverCJava::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<CJavaScript>(p_resource.ptr())) {
		p_extensions->push_back("cjava");
	}
}

bool ResourceFormatSaverCJava::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<CJavaScript>(p_resource.ptr()) != nullptr;
}
