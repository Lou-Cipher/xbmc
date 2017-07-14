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
  class IFramePlayback
  {
  public:
    virtual ~IFramePlayback() = default;

    /*!
     * \brief Past frame count
     */
    virtual uint64_t PastFramesAvailable() const = 0;

    /*!
     * \brief Future frame count
     */
    virtual uint64_t FutureFramesAvailable() const = 0;

    virtual bool HasCurrentFrame() const = 0;

    virtual uint64_t MaxFrameCount() const = 0;

    virtual void SeekFrame(int64_t offsetFrames) = 0;

    virtual void SetMaxFrameCount(uint64_t frameCount) = 0;

    virtual void FrameEvent() = 0;
  };
}
}
