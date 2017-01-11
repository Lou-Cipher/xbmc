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

#include "Environment.h"
#include "Action.h"
#include "Goal.h"
#include "Reward.h"
#include "State.h"
#include "threads/SystemClock.h"

#include <cmath>

using namespace RETROPLAYER;

#define FOREVER_MS   (7 * 24 * 60 * 60 * 1000) // 1 week is large enough

CEnvironment::CEnvironment() :
  CThread("RetroPlayer Environment"),
  m_state(new CState(this)),
  m_reward(new CReward(this)),
  m_goal(new CGoal(this)),
  m_action(new CAction(this)),
  m_speedFactor(0.0),
  m_lastFrameMs(0.0)
{
}

bool CEnvironment::Create()
{
  if (!IsRunning())
  {
    CThread::Create(false);
    return true;
  }
  return false;
}

void CEnvironment::Destroy()
{
  StopThread(true);
}

void CEnvironment::SetSpeed(double speedFactor)
{
  {
    CSingleLock lock(m_mutex);
    m_speedFactor = speedFactor;
  }
  m_sleepEvent.Set();
}

void CEnvironment::Process()
{
  // Environment variables
  uint64_t t = 0;
  CState& state = *m_state;
  CReward& reward = *m_reward;
  CGoal& goal = *m_goal;
  CAction& action = *m_action;

  // Timing variables
  double nextFrameMs = NowMs();

  while (!m_bStop)
  {
    UpdateState(t, state, reward, goal, action);
    if (m_bStop)
      break;

    UpdateReward(t, state, reward, goal, action);
    if (m_bStop)
      break;

    UpdateGoal(t, state, reward, goal, action);
    if (m_bStop)
      break;

    //WaitForAction();
    if (m_bStop)
      break;

    UpdateAction(t, state, reward, goal, action);

    //! @todo Seek

    t++;
  }
}

// --- Environment functions ---------------------------------------------------

void CEnvironment::UpdateState(uint64_t t, CState& state, const CReward& reward, const CGoal& goal, const CAction& action)
{
  state.Update(t, reward, goal, action);
}

void CEnvironment::UpdateReward(uint64_t t, const CState& state, CReward& reward, const CGoal& goal, const CAction& action)
{
  reward.Update(t, state, goal, action);
}

void CEnvironment::UpdateGoal(uint64_t t, const CState& state, const CReward& reward, CGoal& goal, const CAction& action)
{
  goal.Update(t, state, reward, action);
}

void CEnvironment::UpdateAction(uint64_t t, const CState& state, const CReward& reward, const CGoal& goal, CAction& action)
{
  action.Update(t, state, reward, goal);
}

// --- Timing functions --------------------------------------------------------

double CEnvironment::FrameTimeMs() const
{
  const double fps = 60.0; //! @todo

  if (m_speedFactor != 0.0)
    return 1000.0 / fps / std::fabs(m_speedFactor);
  else
    return FOREVER_MS;
}

double CEnvironment::SleepTimeMs(double nowMs) const
{
  // Calculate next frame time
  const double nextFrameMs = m_lastFrameMs + FrameTimeMs();

  // Calculate sleep time
  const double sleepTimeMs = nextFrameMs - nowMs;

  return sleepTimeMs;
}

double CEnvironment::NowMs() const
{
  return static_cast<double>(XbmcThreads::SystemClockMillis());
}
