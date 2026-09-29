/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Main header file for the TS++ project.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/
#pragma once

#include "always.h"
#include <cstdio>


// #define OUTPUT_MANGLED_NAMES 1


/**
 *  Outputs the address and mangled name to the debug output window.
 */
#ifdef OUTPUT_MANGLED_NAMES
#define OUTPUT_MANGLED_NAME(address, name) __pragma(message("    MakeName(" #address ", " "\"" name "\"" ");"))
#else
#define OUTPUT_MANGLED_NAME(address, name)
#endif


/**
 *  DEFINE_IMPLEMENTATION(prototype, address)
 *
 *  Defines a naked function that jumps to a hardcoded address.
 *
 *  Requirements:
 *  - The function `prototype` must already be declared (e.g., in a class or header).
 *  - Must be used in a `.cpp` source file, outside any class body.
 *
 *  Parameters:
 *  - prototype: The full function prototype including namespace/class (e.g., int Pipe::Flush()).
 *  - address:   The absolute memory address the function should jump to.
 *
 *  Example:
 *      class Pipe { public: int Flush(); };
 *      DEFINE_IMPLEMENTATION(int Pipe::Flush(), 0x005A94E0);
 *
 *  WARNING: If you use this, the definition(s) must be after everything else in the module
 *           otherwise you will get 'unreachable code' warnings!
 *
 */
#define DEFINE_IMPLEMENTATION(prototype, address) \
    /*[[ noreturn ]]*/ __declspec(noinline) __declspec(naked) prototype { \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }

#define DEFINE_IMPLEMENTATION_INLINE(prototype, address, ...) \
    /*[[ noreturn ]]*/ inline __declspec(naked) prototype { \
        OUTPUT_MANGLED_NAME(address, __FUNCDNAME__) \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }

#define DEFINE_IMPLEMENTATION_UNWIND(prototype, address, ...) \
    /*[[ noreturn ]]*/ \
    prototype { \
        OUTPUT_MANGLED_NAME(address, __FUNCDNAME__) \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }

/**
 *    For constructor jumps, we need to adjust the stack before we
 *    make the jump to the address, otherwise the stack will be smashed.
 */
#ifndef NDEBUG
#define CONSTRUCTOR_EPILOG \
    _asm { pop edi } \
    _asm { pop esi } \
    _asm { pop ebx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }
#else
// RelWithDebInfo produces the same epilog as Debug.
#define CONSTRUCTOR_EPILOG \
    _asm { pop edi } \
    _asm { pop esi } \
    _asm { pop ebx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }

/*
#define CONSTRUCTOR_EPILOG \
    _asm { pop ecx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }
*/
#endif

#define DEFINE_IMPLEMENTATION_CONSTRUCTOR(prototype, address, ...) \
    /*[[ noreturn ]]*/ __declspec(noinline) prototype { \
        OUTPUT_MANGLED_NAME(address, __FUNCDNAME__) \
        _asm { mov ecx, this } \
        _asm { mov esp, ebp } \
        _asm { pop ebp } \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }

// For classes with a base class that has no default constructor available.
#define DEFINE_IMPLEMENTATION_CONSTRUCTOR_BASE(prototype, base, address, ...) \
    /*[[ noreturn ]]*/ __declspec(noinline) prototype: \
    base(NoInitClass()) { \
        OUTPUT_MANGLED_NAME(address, __FUNCDNAME__) \
        _asm { mov ecx, this } \
        _asm { mov esp, ebp } \
        _asm { pop ebp } \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }

/**
 *    For destructor jumps, we need to adjust the stack before we
 *    make the jump to the address, otherwise the stack will be smashed.
 */
#ifndef NDEBUG
#define DESTRUCTOR_EPILOG \
    _asm { pop edi } \
    _asm { pop esi } \
    _asm { pop ebx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }
#else
// RelWithDebInfo produces the same epilog as Debug.
#define DESTRUCTOR_EPILOG \
    _asm { pop edi } \
    _asm { pop esi } \
    _asm { pop ebx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }

/*
#define DESTRUCTOR_EPILOG \
    _asm { pop ecx } \
    _asm { mov esp, ebp } \
    _asm { pop ebp }
*/
#endif

#define DEFINE_IMPLEMENTATION_DESTRUCTOR(prototype, address, ...) \
    /*[[ noreturn ]]*/ __declspec(noinline) prototype { \
        __pragma(message(__FUNCDNAME__)) \
                OUTPUT_MANGLED_NAME(address, __FUNCDNAME__) DESTRUCTOR_EPILOG; \
        _asm { mov eax, address } \
        _asm { jmp eax } \
    }


/**
 *  Use Make_Global to access global variables in the original exe once you know
 *  the correct type and address. This should not change from run to run if the
 *  exe loads at a standard base address.
 *
 *  Typical use will be to use define to create a friendly name, e.g:
 *    #define SomeGlobalVar (Make_Global<bool>(0x00FF00FF))
 *
 *  allows you to use SomeGlobalVar as though it was a bool you declared, though
 *  it will reflect the value the original exe sees at address 0x00FF00FF.
 */
template<typename T>
__forceinline T& Make_Global(const uintptr_t address)
{
    static_assert(!std::is_reference_v<T>, "Do not pass a reference type to Make_Global");
    return *reinterpret_cast<T*>(address);
}

template<typename T>
__forceinline T* Make_Pointer(const uintptr_t address)
{
    return reinterpret_cast<T*>(address);
}


/**
 *  Helper macros for defining hooks to various arrays.
 *
 *  @author: OmniBlade, duncanspumpkin
 */
#define ARRAY_MEMBER_DEC(type, var, size)     type(&var)[size];
#define ARRAY_DEC(type, var, size)            extern type(&var)[size];
#define ARRAY_DEF(address, type, var, size)   type(&var)[size] = Make_Global<type[size]>(address);
#define ARRAY2D_DEC(type, var, x, y)          extern type(&var)[x][y];
#define ARRAY2D_DEF(address, type, var, x, y) type(&var)[x][y] = Make_Global<type[x][y]>(address);
