/**************************************************************************/
/*  cjava_parser.h                                                        */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#pragma once

#include "cjava_bytecode.h"

class CJavaParser {
public:
	static CJavaParsedScript parse(const String &p_source, const String &p_path = "");
};
