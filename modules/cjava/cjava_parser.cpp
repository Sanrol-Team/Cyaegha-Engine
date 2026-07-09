/**************************************************************************/
/*  cjava_parser.cpp                                                      */
/**************************************************************************/
/*                         Cyaegha Engine                                 */
/**************************************************************************/

#include "cjava_parser.h"

#include "cjava_compiler.h"

CJavaParsedScript CJavaParser::parse(const String &p_source, const String &p_path) {
	return CJavaCompiler::compile(p_source, p_path);
}
