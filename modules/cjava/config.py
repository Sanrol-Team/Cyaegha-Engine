def can_build(env, platform):
    return True


def configure(env):
    env.add_module_version_string("cjava")


def get_doc_classes():
    return [
        "CJavaScript",
    ]


def get_doc_path():
    return "doc_classes"
