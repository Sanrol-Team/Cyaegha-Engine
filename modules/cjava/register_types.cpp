/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "register_types.h"

#include "cjava_resource_format.h"
#include "cjava_script.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"

static CJavaLanguage *script_language_cjava = nullptr;
static Ref<ResourceFormatLoaderCJava> resource_loader_cjava;
static Ref<ResourceFormatSaverCJava> resource_saver_cjava;

void initialize_cjava_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(CJavaScript);

		script_language_cjava = memnew(CJavaLanguage);
		ScriptServer::register_language(script_language_cjava);

		resource_loader_cjava.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_cjava);

		resource_saver_cjava.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_cjava);
	}
}

void uninitialize_cjava_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		ScriptServer::unregister_language(script_language_cjava);

		ResourceLoader::remove_resource_format_loader(resource_loader_cjava);
		resource_loader_cjava.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_cjava);
		resource_saver_cjava.unref();

		if (script_language_cjava) {
			memdelete(script_language_cjava);
			script_language_cjava = nullptr;
		}
	}
}
