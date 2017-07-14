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

#include "ReversiblePlayback.h"
#include "IFramePlayback.h"
#include "games/GameSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <math.h>

using namespace KODI;
using namespace RETRO;

#define REWIND_FACTOR  0.25  // Rewind at 25% of gameplay speed

CReversiblePlayback::CReversiblePlayback(IFramePlayback* framePlayback, double fps) :
  m_framePlayback(framePlayback),
  m_fps(fps)
{
  ResizeFramePlayback();

  GAME::CGameSettings::GetInstance().RegisterObserver(this);
}

CReversiblePlayback::~CReversiblePlayback()
{
  GAME::CGameSettings::GetInstance().UnregisterObserver(this);
}

void CReversiblePlayback::SeekTimeMs(uint64_t timeMs)
{
  CSingleLock lock(m_mutex);

  const int64_t offsetTimeMs = timeMs - GetTimeMs();
  const int64_t offsetFrames = llrint(offsetTimeMs / 1000.0 * m_fps);

  if (offsetFrames != 0)
  {
    m_framePlayback->SeekFrame(offsetFrames);
    UpdatePlaybackStats();
  }
}

void CReversiblePlayback::FrameEvent(double speed)
{
  CSingleLock lock(m_mutex);

  if (speed < 0.0)
    m_framePlayback->SeekFrame(-1);

  if (speed != 0.0)
    m_framePlayback->FrameEvent();

  UpdatePlaybackStats();
}

void CReversiblePlayback::UpdatePlaybackStats()
{
  const uint64_t played = m_framePlayback->PastFramesAvailable() + m_framePlayback->HasCurrentFrame() ? 1 : 0;
  const uint64_t total = m_framePlayback->MaxFrameCount();
  const uint64_t cached = m_framePlayback->FutureFramesAvailable();

  m_playTimeMs = llrint(1000.0 * played / m_fps);
  m_totalTimeMs = llrint(1000.0 * total / m_fps);
  m_cacheTimeMs = llrint(1000.0 * cached / m_fps);
}

void CReversiblePlayback::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
  case ObservableMessageSettingsChanged:
    ResizeFramePlayback();
    break;
  default:
    break;
  }
}

void CReversiblePlayback::ResizeFramePlayback()
{
  uint64_t frameCount = 0;

  const bool bRewindEnabled = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_GAMES_ENABLEREWIND);
  if (bRewindEnabled)
  {
    int rewindBufferSec = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_GAMES_REWINDTIME);
    if (rewindBufferSec < 10)
      rewindBufferSec = 10; // Sanity check

    frameCount = llrint(rewindBufferSec * m_fps);
  }

  CSingleLock lock(m_mutex);

  m_framePlayback->SetMaxFrameCount(frameCount);
  UpdatePlaybackStats();
}
