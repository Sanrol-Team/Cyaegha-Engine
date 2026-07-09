/**************************************************************************/
/*  cjava_runtime.h                                                       */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_bytecode.h"

class CJavaInstance;

class CJavaRuntime {
public:
	static Variant execute(CJavaInstance *p_instance, const CJavaMethodDef &p_method, const Variant **p_args, int p_argcount);
	static Variant execute_super(CJavaInstance *p_instance, const StringName &p_method, const Variant **p_args, int p_argcount);
};
