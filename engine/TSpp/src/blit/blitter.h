/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Base class for the templated blitters.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/
#pragma once

#include "always.h"


class Blitter
{
public:
    Blitter() {}
    virtual ~Blitter() {}

    /**
     *  Blits from source to dest (starts at first pixel). The preferred, ~99%-of-the-time path.
     */
    virtual void BlitForward(void* dest, void const* source, int length, int z_min = 0, void* z_buff = nullptr, void* a_buff = nullptr, int alpha_level = 1000, int warp_offset = 0) const = 0;

    /**
     *  Copies the pixels in reverse order. Only required when the source and dest regions
     *  overlap in a certain way; rarely called.
     */
    virtual void BlitBackward(void* dest, void const* source, int length, int z_min = 0, void* z_buff = nullptr, void* a_buff = nullptr, int alpha_level = 1000) const = 0;
};
