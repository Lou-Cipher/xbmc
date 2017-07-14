/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "ITimePlayback.h"

namespace KODI
{
namespace RETRO
{
  class CRealtimePlayback : public ITimePlayback
  {
  public:
    virtual ~CRealtimePlayback() = default;

    // implementation of ITimePlayback
    uint64_t GetTimeMs() const override { return 0; }
    uint64_t GetTotalTimeMs() const override { return 0; }
    uint64_t GetCacheTimeMs() const override { return 0; }
    void SeekTimeMs(uint64_t timeMs) override { }
    void FrameEvent(double speed) override { }
  };
}
}
