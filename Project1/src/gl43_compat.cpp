#include "gl43_compat.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

PFNGLBINDIMAGETEXTUREPROC pglBindImageTexture = nullptr;
PFNGLDISPATCHCOMPUTEPROC  pglDispatchCompute  = nullptr;
PFNGLMEMORYBARRIERPROC    pglMemoryBarrier    = nullptr;

#ifdef _WIN32
static void* GetAnyGLFuncAddress(const char* name)
{
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 || p == (void*)-1)
    {
        HMODULE module = GetModuleHandleA("opengl32.dll");
        p = (void*)GetProcAddress(module, name);
    }
    return p;
}
#endif

bool LoadGL43ComputeFunctions()
{
#ifdef _WIN32
    pglBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)GetAnyGLFuncAddress("glBindImageTexture");
    pglDispatchCompute  = (PFNGLDISPATCHCOMPUTEPROC) GetAnyGLFuncAddress("glDispatchCompute");
    pglMemoryBarrier    = (PFNGLMEMORYBARRIERPROC)   GetAnyGLFuncAddress("glMemoryBarrier");
    return pglBindImageTexture && pglDispatchCompute && pglMemoryBarrier;
#else
    return false;
#endif
}

