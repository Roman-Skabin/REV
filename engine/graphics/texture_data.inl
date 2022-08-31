// Copyright (c) 2020-2022, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

namespace REV
{
    REV_INLINE bool TextureData::Empty() const
    {
        return m_Header ? !m_Header->surfaces_count : true;
    }

    REV_INLINE u16 TextureData::PlanesCount() const
    {
        return m_Header ? m_Header->planes_count : 0;
    }

    REV_INLINE u16 TextureData::SurfacesCount() const
    {
        return m_Header ? m_Header->surfaces_count : 0;
    }
}
