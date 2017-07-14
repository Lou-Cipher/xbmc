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
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  class IFramePlayback;

  class CReversiblePlayback : public ITimePlayback,
                              public Observer
  {
  public:
    CReversiblePlayback(IFramePlayback* framePlayback, double fps);

    virtual ~CReversiblePlayback();

    // implementation of ITimePlayback
    uint64_t GetTimeMs() const override      { return m_playTimeMs; }
    uint64_t GetTotalTimeMs() const override { return m_totalTimeMs; }
    uint64_t GetCacheTimeMs() const override { return m_cacheTimeMs; }
    void SeekTimeMs(uint64_t timeMs) override;
    void FrameEvent(double speed) override;

    // implementation of Observer
    void Notify(const Observable &obs, const ObservableMessage msg) override;

  private:
    void UpdatePlaybackStats();
    void ResizeFramePlayback();

    // Construction parameter
    IFramePlayback *const m_framePlayback;
    const double m_fps;

    // Playback stats
    uint64_t m_playTimeMs = 0;
    uint64_t m_totalTimeMs = 0;
    uint64_t m_cacheTimeMs = 0;

    CCriticalSection m_mutex;
  };
}
}
