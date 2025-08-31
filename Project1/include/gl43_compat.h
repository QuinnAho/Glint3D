#pragma once

#include "gl_platform.h"

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

// Minimal prototypes for GL 4.2/4.3 functions we use
typedef void (APIENTRYP PFNGLBINDIMAGETEXTUREPROC)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void (APIENTRYP PFNGLDISPATCHCOMPUTEPROC)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (APIENTRYP PFNGLMEMORYBARRIERPROC)(GLbitfield barriers);

extern PFNGLBINDIMAGETEXTUREPROC pglBindImageTexture;
extern PFNGLDISPATCHCOMPUTEPROC  pglDispatchCompute;
extern PFNGLMEMORYBARRIERPROC    pglMemoryBarrier;

// Load the above pointers using wglGetProcAddress / opengl32.dll
bool LoadGL43ComputeFunctions();
