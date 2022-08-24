#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "defines.h"

#include <stdio.h>

const char* src_str(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API: return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
	case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
	case GL_DEBUG_SOURCE_OTHER: return "OTHER";
	}
	return "";
}

const char* type_str(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: return "ERROR";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
	case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
	case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
	case GL_DEBUG_TYPE_MARKER: return "MARKER";
	case GL_DEBUG_TYPE_OTHER: return "OTHER";
	}
	return "";
}

const char* severity_str(GLenum severity)
{
	switch (severity) {
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
	case GL_DEBUG_SEVERITY_LOW: return "LOW";
	case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
	case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
	}
	return "";
}

void message_callback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param )
{

	printf("%s, %s, %s, %d: %s\n\n", src_str, type_str, severity_str, id, message);
}

void debug_init()
{
	glDebugMessageCallback( message_callback, NULL );
	glEnable( GL_DEBUG_OUTPUT );
	glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
	glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE );
};

#endif