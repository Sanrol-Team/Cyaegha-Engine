/**************************************************************************/
/*  cjava_compiler.h                                                      */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_bytecode.h"

class CJavaCompiler {
public:
	static CJavaParsedScript compile(const String &p_source, const String &p_path = "");
};
