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

#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  class ITimePlayback
  {
  public:
    virtual ~ITimePlayback() = default;

    virtual uint64_t GetTimeMs() const = 0;
    virtual uint64_t GetTotalTimeMs() const = 0;
    virtual uint64_t GetCacheTimeMs() const = 0;
    virtual void SeekTimeMs(uint64_t timeMs) = 0;

    /*!
     * \brief The next frame is being shown
     *
     * \param speed The current clock speed
     */
    virtual void FrameEvent(double speed) = 0;
  };
}
}
