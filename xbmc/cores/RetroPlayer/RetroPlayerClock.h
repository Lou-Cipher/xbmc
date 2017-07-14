/*
 *      Copyright (C) 2017 Team Kodi
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

#include "threads/Thread.h"

namespace KODI
{
namespace RETRO
{
  class IRetroPlayerClockCallback
  {
  public:
    virtual ~IRetroPlayerClockCallback() = default;

    /*!
     * \brief The next frame is being shown
     */
    virtual void FrameEvent() = 0;
  };

  class CRetroPlayerEnvironment : public IRetroPlayerClockCallback
  {
  public:
    ~CRetroPlayerEnvironment() override = default;
  };

  class CRetroPlayerClock : protected CThread
  {
  public:
    CRetroPlayerClock(IRetroPlayerClockCallback *callback, double fps);

    ~CRetroPlayerClock() override;

    void Start();
    void Stop();

    void SetSpeed(double speed);
    double GetSpeed() const { return m_speedFactor; }

  protected:
    // implementation of CThread
    void Process() override;

  private:
    // Timing helper functions
    double FrameTimeMs() const;
    double SleepTimeMs(double nowMs) const;
    double NowMs() const;

    // Construction parameters
    IRetroPlayerClockCallback *const m_callback;
    const double m_fps;

    // Timing parameters
    double m_speedFactor = 0.0;
    double m_lastFrameMs = 0.0;
    CEvent m_sleepEvent;
    CCriticalSection m_mutex;
  };
}
}
