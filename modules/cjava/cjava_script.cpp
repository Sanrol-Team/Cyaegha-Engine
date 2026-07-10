/**************************************************************************/
/*  cjava_script.cpp                                                      */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_script.h"

#include "cjava_runtime.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/object/script_language.h"
#include "scene/main/node.h"

CJavaLanguage *CJavaLanguage::singleton = nullptr;

static uint32_t _cjava_export_usage() {
	return PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_EDITOR;
}

void CJavaScript::_bind_methods() {
}

void CJavaLanguage::_bind_methods() {}

CJavaLanguage *CJavaLanguage::get_singleton() {
	return singleton;
}

void CJavaScript::set_path(const String &p_path) {
	path = p_path;
}

Error CJavaScript::reload_from_source() {
	parsed = CJavaParser::parse(source_code, path);
	valid = parsed.valid;
	base_script.unref();
	if (valid && !parsed.base_is_native && !parsed.base_script_path.is_empty()) {
		Error load_err = OK;
		Ref<CJavaScript> loaded = ResourceLoader::load(parsed.base_script_path, "CJavaScript", ResourceLoader::CACHE_MODE_REUSE, &load_err);
		if (load_err == OK && loaded.is_valid()) {
			base_script = loaded;
		} else {
			valid = false;
			ERR_PRINT(vformat("CJava: failed to load base script '%s'", parsed.base_script_path));
			return ERR_FILE_CANT_OPEN;
		}
	}
	if (!valid) {
		ERR_PRINT(vformat("CJava: %s", parsed.error_message));
		return ERR_PARSE_ERROR;
	}
	return OK;
}

bool CJavaScript::can_instantiate() const {
	return valid && ClassDB::class_exists(parsed.base_class);
}

Ref<Script> CJavaScript::get_base_script() const {
	return base_script;
}

StringName CJavaScript::get_global_name() const {
	return parsed.global_class_name;
}

bool CJavaScript::inherits_script(const Ref<Script> &p_script) const {
	Ref<CJavaScript> cursor = Ref<CJavaScript>(const_cast<CJavaScript *>(this));
	while (cursor.is_valid()) {
		if (cursor == p_script) {
			return true;
		}
		cursor = cursor->base_script;
	}
	return false;
}

StringName CJavaScript::get_instance_base_type() const {
	return parsed.base_class;
}

ScriptInstance *CJavaScript::instance_create(Object *p_this) {
	if (!valid) {
		return nullptr;
	}
	return memnew(CJavaInstance(Ref<CJavaScript>(this), p_this));
}

bool CJavaScript::has_source_code() const {
	return !source_code.is_empty();
}

String CJavaScript::get_source_code() const {
	return source_code;
}

void CJavaScript::set_source_code(const String &p_code) {
	if (source_code != p_code) {
		source_code = p_code;
		reload_from_source();
	}
}

Error CJavaScript::reload(bool p_keep_state) {
	return reload_from_source();
}

#ifdef TOOLS_ENABLED
StringName CJavaScript::get_doc_class_name() const {
	return get_global_name();
}

Vector<DocData::ClassDoc> CJavaScript::get_documentation() const {
	return Vector<DocData::ClassDoc>();
}

String CJavaScript::get_class_icon_path() const {
	return String();
}
#endif

bool CJavaScript::has_method(const StringName &p_method) const {
	if (parsed.methods.has(p_method)) {
		return true;
	}
	if (base_script.is_valid()) {
		return base_script->has_method(p_method);
	}
	return false;
}

MethodInfo CJavaScript::get_method_info(const StringName &p_method) const {
	if (parsed.methods.has(p_method)) {
		MethodInfo info;
		info.name = p_method;
		const CJavaMethodDef &method = parsed.methods[p_method];
		for (int i = 0; i < method.arg_names.size(); i++) {
			Variant::Type arg_type = i < method.arg_types.size() ? method.arg_types[i].builtin : Variant::NIL;
			info.arguments.push_back(PropertyInfo(arg_type, method.arg_names[i]));
		}
		return info;
	}
	if (base_script.is_valid()) {
		return base_script->get_method_info(p_method);
	}
	MethodInfo info;
	info.name = p_method;
	return info;
}

bool CJavaScript::is_tool() const {
	return tool_script;
}

bool CJavaScript::is_script_valid() const {
	return valid;
}

bool CJavaScript::is_abstract() const {
	return false;
}

ScriptLanguage *CJavaScript::get_language() const {
	return CJavaLanguage::get_singleton();
}

bool CJavaScript::has_script_signal(const StringName &p_signal) const {
	return false;
}

void CJavaScript::get_script_signal_list(List<MethodInfo> *r_signals) const {}

bool CJavaScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	for (const CJavaFieldDef &field : parsed.fields) {
		if (field.name == p_property && field.export_to_editor) {
			r_value = field.default_value;
			return true;
		}
	}
	return false;
}

void CJavaScript::get_script_method_list(List<MethodInfo> *p_list) const {
	if (base_script.is_valid()) {
		base_script->get_script_method_list(p_list);
	}
	for (const KeyValue<StringName, CJavaMethodDef> &E : parsed.methods) {
		bool exists = false;
		for (const MethodInfo &existing : *p_list) {
			if (existing.name == E.key) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			p_list->push_back(get_method_info(E.key));
		}
	}
}

void CJavaScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	if (base_script.is_valid()) {
		base_script->get_script_property_list(p_list);
	}
	for (const CJavaFieldDef &field : parsed.fields) {
		if (!field.export_to_editor) {
			continue;
		}
		bool exists = false;
		for (const PropertyInfo &existing : *p_list) {
			if (existing.name == field.name) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			PropertyInfo info(field.type.builtin, field.name, PROPERTY_HINT_NONE, field.type.native_class, _cjava_export_usage());
			p_list->push_back(info);
		}
	}
}

const Variant CJavaScript::get_rpc_config() const {
	return Variant();
}

Variant CJavaInstance::get_field_value(int p_index) const {
	if (p_index >= 0 && p_index < field_values.size()) {
		return field_values[p_index];
	}
	return Variant();
}

bool CJavaInstance::set_field_value(int p_index, const Variant &p_value, const CJavaFieldDef &p_field) {
	if (p_index < 0) {
		return false;
	}
	if (p_value.get_type() == Variant::OBJECT) {
		Object *obj = p_value.get_validated_object();
		String type_error;
		if (!validate_object_type(obj, p_field.type, &type_error)) {
			ERR_PRINT(vformat("CJava: %s", type_error));
			return false;
		}
		if (p_field.ownership != CJAVA_OWNERSHIP_NONE) {
			track_symbol(p_field.name, p_value, p_field.ownership, p_field.type);
		}
	}
	if (p_index >= field_values.size()) {
		field_values.resize(p_index + 1);
	}
	field_values.write[p_index] = p_value;
	return true;
}

void CJavaInstance::track_symbol(const StringName &p_symbol, const Variant &p_value, CJavaOwnership p_ownership, const CJavaType &p_type) {
	Object *obj = p_value.get_validated_object();
	if (!obj || p_ownership == CJAVA_OWNERSHIP_NONE) {
		return;
	}
	String type_error;
	if (!validate_object_type(obj, p_type, &type_error)) {
		ERR_PRINT(vformat("CJava: %s", type_error));
		return;
	}
	ownership.track(p_symbol, CJavaHandle::from_object(obj, p_ownership), p_ownership);
}

bool CJavaInstance::validate_object_type(Object *p_object, const CJavaType &p_type, String *r_error) const {
	if (!p_object) {
		return true;
	}
	if (!p_type.native_class.is_empty()) {
		if (!ClassDB::is_parent_class(p_object->get_class(), p_type.native_class)) {
			if (r_error) {
				*r_error = vformat("Expected object of type %s, got %s", p_type.native_class, p_object->get_class());
			}
			return false;
		}
	}
	if (!p_type.script_class.is_empty()) {
		Ref<Script> obj_script = p_object->get_script();
		Ref<CJavaScript> cj = obj_script;
		if (cj.is_null() || cj->get_global_name() != p_type.script_class) {
			if (r_error) {
				*r_error = vformat("Expected script class %s", p_type.script_class);
			}
			return false;
		}
	}
	return true;
}

CJavaInstance::CJavaInstance(Ref<CJavaScript> p_script, Object *p_owner) {
	script = p_script;
	owner = p_owner;
	if (script.is_valid()) {
		const CJavaParsedScript &parsed = script->get_parsed_script();
		field_values.resize(parsed.fields.size());
		for (int i = 0; i < parsed.fields.size(); i++) {
			field_values.write[i] = parsed.fields[i].default_value;
		}
	}
}

CJavaInstance::~CJavaInstance() {
	ownership.clear();
}

bool CJavaInstance::set(const StringName &p_name, const Variant &p_value) {
	if (!script.is_valid()) {
		return false;
	}
	const CJavaParsedScript &parsed = script->get_parsed_script();
	for (int i = 0; i < parsed.fields.size(); i++) {
		const CJavaFieldDef &field = parsed.fields[i];
		if (field.name == p_name && field.export_to_editor) {
			set_field_value(i, p_value, field);
			return true;
		}
	}
	return false;
}

bool CJavaInstance::get(const StringName &p_name, Variant &r_ret) const {
	if (!script.is_valid()) {
		return false;
	}
	const CJavaParsedScript &parsed = script->get_parsed_script();
	for (int i = 0; i < parsed.fields.size(); i++) {
		const CJavaFieldDef &field = parsed.fields[i];
		if (field.name == p_name && field.export_to_editor) {
			r_ret = get_field_value(i);
			return true;
		}
	}
	if (ownership.has_binding(p_name)) {
		const CJavaHandle *handle = ownership.get_handle_for_symbol(p_name);
		if (handle && handle->is_valid()) {
			r_ret = handle->resolve();
			return true;
		}
	}
	return false;
}

void CJavaInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (!script.is_valid()) {
		return;
	}
	for (const CJavaFieldDef &field : script->get_parsed_script().fields) {
		if (!field.export_to_editor) {
			continue;
		}
		PropertyInfo info(field.type.builtin, field.name, PROPERTY_HINT_NONE, field.type.native_class, _cjava_export_usage());
		p_properties->push_back(info);
	}
}

Variant::Type CJavaInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (!script.is_valid()) {
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return Variant::NIL;
	}
	for (const CJavaFieldDef &field : script->get_parsed_script().fields) {
		if (field.name == p_name && field.export_to_editor) {
			if (r_is_valid) {
				*r_is_valid = true;
			}
			return field.type.builtin;
		}
	}
	if (r_is_valid) {
		*r_is_valid = false;
	}
	return Variant::NIL;
}

void CJavaInstance::validate_property(PropertyInfo &p_property) const {}

bool CJavaInstance::property_can_revert(const StringName &p_name) const {
	if (!script.is_valid()) {
		return false;
	}
	for (const CJavaFieldDef &field : script->get_parsed_script().fields) {
		if (field.name == p_name && field.export_to_editor) {
			return true;
		}
	}
	return false;
}

bool CJavaInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
	if (!script.is_valid()) {
		return false;
	}
	for (const CJavaFieldDef &field : script->get_parsed_script().fields) {
		if (field.name == p_name && field.export_to_editor) {
			r_ret = field.default_value;
			return true;
		}
	}
	return false;
}

Object *CJavaInstance::get_owner() {
	return owner;
}

void CJavaInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool CJavaInstance::has_method(const StringName &p_method) const {
	return script.is_valid() && script->has_method(p_method);
}

int CJavaInstance::get_method_argument_count(const StringName &p_method, bool *r_is_valid) const {
	if (script.is_valid() && script->get_parsed_script().methods.has(p_method)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return script->get_parsed_script().methods[p_method].arg_names.size();
	}
	if (r_is_valid) {
		*r_is_valid = false;
	}
	return 0;
}

Variant CJavaInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (!script.is_valid() || !script->get_parsed_script().methods.has(p_method)) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	const CJavaMethodDef &method = script->get_parsed_script().methods[p_method];
	const Variant ret = CJavaRuntime::execute(this, method, p_args, p_argcount);
	r_error.error = Callable::CallError::CALL_OK;
	return ret;
}

void CJavaInstance::notification(int p_notification, bool p_reversed) {
	if (!script.is_valid()) {
		return;
	}

	Node *node = Object::cast_to<Node>(owner);

	if (p_notification == Node::NOTIFICATION_ENTER_TREE && !p_reversed && node) {
		if (script->has_method("_process")) {
			node->set_process(true);
		}
	}

	if (p_notification == Node::NOTIFICATION_READY && !p_reversed && script->has_method("_ready")) {
		Callable::CallError err;
		callp(StringName("_ready"), nullptr, 0, err);
	}

	if (p_notification == Node::NOTIFICATION_PROCESS && !p_reversed && script->has_method("_process") && node) {
		const Variant delta = node->get_process_delta_time();
		const Variant *args[1] = { &delta };
		Callable::CallError err;
		callp(StringName("_process"), args, 1, err);
	}

	if (p_notification == Object::NOTIFICATION_PREDELETE) {
		ownership.clear();
	}
}

bool CJavaInstance::refcount_decremented() {
	return true;
}

Ref<Script> CJavaInstance::get_script() const {
	return script;
}

ScriptLanguage *CJavaInstance::get_language() {
	return CJavaLanguage::get_singleton();
}

const Variant CJavaInstance::get_rpc_config() const {
	return script.is_valid() ? script->get_rpc_config() : Variant();
}

String CJavaLanguage::get_name() const {
	return "CJava";
}

void CJavaLanguage::init() {
	singleton = this;
}

String CJavaLanguage::get_type() const {
	return "CJavaScript";
}

String CJavaLanguage::get_extension() const {
	return "cjava";
}

void CJavaLanguage::finish() {
	singleton = nullptr;
}

#ifdef TOOLS_ENABLED
EditorLanguage *CJavaLanguage::get_editor_language() {
	return nullptr;
}
#endif

Vector<String> CJavaLanguage::get_reserved_words() const {
	Vector<String> words;
	words.push_back("extends");
	words.push_back("class");
	words.push_back("void");
	words.push_back("double");
	words.push_back("float");
	words.push_back("int");
	words.push_back("boolean");
	words.push_back("bool");
	words.push_back("String");
	words.push_back("Node");
	words.push_back("if");
	words.push_back("else");
	words.push_back("@own");
	words.push_back("@borrow");
	words.push_back("super");
	words.push_back("true");
	words.push_back("false");
	return words;
}

bool CJavaLanguage::is_control_flow_keyword(const String &p_string) const {
	return p_string == "if" || p_string == "else" || p_string == "for";
}

Vector<String> CJavaLanguage::get_comment_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("//");
	delimiters.push_back("/* */");
	return delimiters;
}

Vector<String> CJavaLanguage::get_doc_comment_delimiters() const {
	return get_comment_delimiters();
}

Vector<String> CJavaLanguage::get_string_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("' '");
	delimiters.push_back("\" \"");
	return delimiters;
}

Ref<Script> CJavaLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<CJavaScript> script;
	script.instantiate();
	const String code = vformat(
			"extends %s;\n\n"
			"@export int speed = 10;\n\n"
			"void _ready() {\n"
			"    Node player = get_node(\"Player\");\n"
			"    if (speed > 5) {\n"
			"        print(\"fast\");\n"
			"    }\n"
			"}\n\n"
			"void _process(double delta) {\n"
			"    speed = speed + 1;\n"
			"}\n",
			p_base_class_name);
	script->set_source_code(code);
	script->reload();
	return script;
}

Vector<ScriptLanguage::ScriptTemplate> CJavaLanguage::get_built_in_templates(const StringName &p_object) {
	Vector<ScriptTemplate> templates;
	if (ClassDB::is_parent_class(p_object, Node::get_class_static())) {
		ScriptTemplate tpl;
		tpl.inherit = p_object;
		tpl.name = "Node CJava";
		tpl.description = "CJava script with bytecode VM, @export fields, and Unity-style lifecycle.";
		tpl.content =
				"extends %CLASS%;\n\n"
				"@export int speed = 10;\n\n"
				"void _ready() {\n"
				"    Node player = get_node(\"Player\");\n"
				"    if (speed > 5) {\n"
				"        print(\"fast\");\n"
				"    }\n"
				"    for (int i = 0; i < 3; i = i + 1) {\n"
				"        print(i);\n"
				"    }\n"
				"}\n\n"
				"void _process(double delta) {\n"
				"    speed = speed + 1;\n"
				"}\n";
		templates.push_back(tpl);
	}
	return templates;
}

bool CJavaLanguage::is_using_templates() {
	return true;
}

bool CJavaLanguage::validate(const String &p_script, const String &p_path, List<String> *r_functions, List<ScriptError> *r_errors, List<Warning> *r_warnings, HashSet<int> *r_safe_lines) const {
	const CJavaParsedScript parsed = CJavaParser::parse(p_script, p_path);
	if (!parsed.valid) {
		if (r_errors) {
			ScriptError error;
			error.path = p_path;
			error.line = 1;
			error.message = parsed.error_message;
			r_errors->push_back(error);
		}
		return false;
	}
	if (r_functions) {
		for (const KeyValue<StringName, CJavaMethodDef> &E : parsed.methods) {
			r_functions->push_back(E.key);
		}
	}
	return true;
}

bool CJavaLanguage::supports_builtin_mode() const {
	return false;
}

int CJavaLanguage::find_function(const String &p_function, const String &p_code) const {
	const int pos = p_code.findn("void " + p_function);
	return pos;
}

String CJavaLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const {
	String args;
	for (int i = 0; i < p_args.size(); i++) {
		if (i > 0) {
			args += ", ";
		}
		args += "double " + p_args[i];
	}
	return vformat("void %s(%s) {\n}\n", p_name, args);
}

void CJavaLanguage::get_public_functions(List<MethodInfo> *p_functions) const {}

void CJavaLanguage::get_public_constants(List<Pair<String, Variant>> *p_constants) const {}

void CJavaLanguage::get_public_annotations(List<MethodInfo> *p_annotations) const {}

void CJavaLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {}

void CJavaLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {}

String CJavaLanguage::debug_get_error() const {
	return String();
}

int CJavaLanguage::debug_get_stack_level_count() const {
	return 0;
}

int CJavaLanguage::debug_get_stack_level_line(int p_level) const {
	return 0;
}

String CJavaLanguage::debug_get_stack_level_function(int p_level) const {
	return String();
}

String CJavaLanguage::debug_get_stack_level_source(int p_level) const {
	return String();
}

void CJavaLanguage::debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}

void CJavaLanguage::debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}

void CJavaLanguage::debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {}

String CJavaLanguage::debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) {
	return String();
}

void CJavaLanguage::reload_all_scripts() {}

void CJavaLanguage::reload_scripts(const Array &p_scripts, bool p_soft_reload) {}

void CJavaLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
	if (p_script.is_valid()) {
		p_script->reload(p_soft_reload);
	}
}

void CJavaLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	if (p_extensions) {
		p_extensions->push_back("cjava");
	}
}

bool CJavaLanguage::handles_global_class_type(const String &p_type) const {
	return p_type == "CJavaScript";
}

String CJavaLanguage::get_global_class_name(const String &p_path, String *r_base_type, String *r_icon_path, bool *r_is_abstract, bool *r_is_tool) const {
	if (r_is_abstract) {
		*r_is_abstract = false;
	}
	if (r_is_tool) {
		*r_is_tool = false;
	}
	if (r_icon_path) {
		*r_icon_path = String();
	}
	Error err = OK;
	Ref<CJavaScript> script = ResourceLoader::load(p_path, "CJavaScript", ResourceLoader::CACHE_MODE_REUSE, &err);
	if (err != OK || script.is_null()) {
		return String();
	}
	if (r_base_type) {
		*r_base_type = script->get_instance_base_type();
	}
	return script->get_global_name();
}
