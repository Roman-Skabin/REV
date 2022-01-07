// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/render_pass.h"

namespace REV
{

RenderPassBase::RenderPassBase(Allocator *allocator, RENDER_PASS_KIND kind)
    : m_Shaders(allocator),
      m_Kind(kind)
{
}

RenderPassBase::~RenderPassBase()
{
}

void RenderPassBase::UploadResources()
{
}

void RenderPassBase::ReadBackResources()
{
}

}
