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
namespace GAME
{
  class CGameClient;
}

namespace RETRO
{
  class CRetroPlayerAutoSave : protected CThread
  {
  public:
    CRetroPlayerAutoSave(GAME::CGameClient &gameClient);

    ~CRetroPlayerAutoSave() override;

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    // Construction parameter
    GAME::CGameClient &m_gameClient;
  };
}
}
