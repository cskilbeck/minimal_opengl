#pragma once

void init_gl_functions();

#undef GL_FUNCTION
#define GL_FUNCTION(fn_type, fn_name) extern fn_type fn_name
#include "gl_functions.inc"
#undef GL_FUNCTION
