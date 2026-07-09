/**************************************************************************/
/*  cjava_script.h                                                        */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_handle.h"
#include "cjava_parser.h"

#include "core/object/script_language.h"

class CJavaScript : public Script {
	GDCLASS(CJavaScript, Script);

	String source_code;
	String path;
	bool tool_script = false;
	bool valid = false;
	CJavaParsedScript parsed;
	Ref<CJavaScript> base_script;

protected:
	static void _bind_methods();

public:
	void set_path(const String &p_path);
	String get_path() const { return path; }

	Error reload_from_source();
	const CJavaParsedScript &get_parsed_script() const { return parsed; }

	// Script API.
	virtual bool can_instantiate() const override;
	virtual Ref<Script> get_base_script() const override;
	virtual StringName get_global_name() const override;
	virtual bool inherits_script(const Ref<Script> &p_script) const override;
	virtual StringName get_instance_base_type() const override;
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual bool has_source_code() const override;
	virtual String get_source_code() const override;
	virtual void set_source_code(const String &p_code) override;
	virtual Error reload(bool p_keep_state = false) override;
#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override;
	virtual Vector<DocData::ClassDoc> get_documentation() const override;
	virtual String get_class_icon_path() const override;
#endif
	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override;
	virtual bool is_tool() const override;
	virtual bool is_script_valid() const override;
	virtual bool is_abstract() const override;
	virtual ScriptLanguage *get_language() const override;
	virtual bool has_script_signal(const StringName &p_signal) const override;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override;
	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;
	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;
	virtual const Variant get_rpc_config() const override;

	CJavaScript() = default;
};

class CJavaInstance : public ScriptInstance {
	friend class CJavaRuntime;

	Ref<CJavaScript> script;
	Object *owner = nullptr;
	CJavaOwnershipArena ownership;
	Vector<Variant> field_values;

	Variant get_field_value(int p_index) const;
	bool set_field_value(int p_index, const Variant &p_value, const CJavaFieldDef &p_field);
	void track_symbol(const StringName &p_symbol, const Variant &p_value, CJavaOwnership p_ownership, const CJavaType &p_type);
	bool validate_object_type(Object *p_object, const CJavaType &p_type, String *r_error = nullptr) const;

public:
	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
	virtual void validate_property(PropertyInfo &p_property) const override;
	virtual bool property_can_revert(const StringName &p_name) const override;
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const override;
	virtual Object *get_owner() override;
	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;
	virtual int get_method_argument_count(const StringName &p_method, bool *r_is_valid = nullptr) const override;
	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
	virtual void notification(int p_notification, bool p_reversed = false) override;
	virtual bool refcount_decremented() override;
	virtual Ref<Script> get_script() const override;
	virtual ScriptLanguage *get_language() override;
	virtual const Variant get_rpc_config() const override;

	CJavaInstance(Ref<CJavaScript> p_script, Object *p_owner);
	~CJavaInstance();
};

class CJavaLanguage : public ScriptLanguage {
	static CJavaLanguage *singleton;

protected:
	static void _bind_methods();

public:
	static CJavaLanguage *get_singleton();

	virtual String get_name() const override;
	virtual void init() override;
	virtual String get_type() const override;
	virtual String get_extension() const override;
	virtual void finish() override;
#ifdef TOOLS_ENABLED
	virtual EditorLanguage *get_editor_language() override;
#endif
	virtual Vector<String> get_reserved_words() const override;
	virtual bool is_control_flow_keyword(const String &p_string) const override;
	virtual Vector<String> get_comment_delimiters() const override;
	virtual Vector<String> get_doc_comment_delimiters() const override;
	virtual Vector<String> get_string_delimiters() const override;
	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	virtual Vector<ScriptTemplate> get_built_in_templates(const StringName &p_object) override;
	virtual bool is_using_templates() override;
	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const override;
	virtual bool supports_builtin_mode() const override;
	virtual int find_function(const String &p_function, const String &p_code) const override;
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override;
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override;
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override;
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const override;

	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override;
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override;

	virtual String debug_get_error() const override;
	virtual int debug_get_stack_level_count() const override;
	virtual int debug_get_stack_level_line(int p_level) const override;
	virtual String debug_get_stack_level_function(int p_level) const override;
	virtual String debug_get_stack_level_source(int p_level) const override;
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override;
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) override;

	virtual void reload_all_scripts() override;
	virtual void reload_scripts(const Array &p_scripts, bool p_soft_reload) override;
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override;

	virtual void get_recognized_extensions(List<String> *p_extensions) const override;

	virtual bool handles_global_class_type(const String &p_type) const override;
	virtual String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr, bool *r_is_abstract = nullptr, bool *r_is_tool = nullptr) const override;

	virtual void profiling_start() override {}
	virtual void profiling_stop() override {}
	virtual void profiling_set_save_native_calls(bool p_enable) override {}
	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
	virtual void frame() override {}

	CJavaLanguage() = default;
};
