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

#include "games/addons/GameClientSubsystem.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/controllers/ControllerTypes.h"

#include <map>
#include <memory>
#include <string>

class CCriticalSection;
struct game_input_event;

namespace KODI
{
namespace GAME
{
  class CGameClient;
  class CGameClientHardware;
  class CGameClientJoystick;
  class CGameClientKeyboard;
  class CGameClientMouse;

  class CGameClientInput : protected CGameClientSubsystem
  {
  public:
    CGameClientInput(CGameClient &gameClient,
                     AddonInstance_Game &addonStruct,
                     CCriticalSection &clientAccess);
    ~CGameClientInput() override;

    void Initialize();
    void Deinitialize();

    // Input functions
    bool AcceptsInput() const;
    const CControllerTree &GetControllerTree() const { return m_controllers; }
    //bool IsControllerAccepted(const std::string &portAddress, const ControllerPtr &controller) const; //! @todo
    void OpenJoystick(const std::string &portAddress);
    void CloseJoystick(const std::string &portAddress);

    // Input callbacks
    bool ReceiveInputEvent(const game_input_event& eventStruct);

  private:
    // Private input helpers
    void LoadTopology();
    bool SupportsKeyboard();
    bool SupportsMouse();
    void UpdatePort(const std::string &portAddress, const ControllerPtr& controller);
    void OpenKeyboard();
    void CloseKeyboard();
    void OpenMouse();
    void CloseMouse();

    // Private callback helpers
    bool SetRumble(const std::string &portAddress, const std::string& feature, float magnitude);

    // Helper functions
    static ControllerVector GetControllers(const CGameClient &gameClient);

    // Input properties
    CControllerTree m_controllers;
    using PortAddress = std::string;
    std::map<PortAddress, std::unique_ptr<CGameClientJoystick>> m_joysticks;
    std::unique_ptr<CGameClientKeyboard> m_keyboard;
    std::unique_ptr<CGameClientMouse> m_mouse;
    std::unique_ptr<CGameClientHardware> m_hardware;
    int m_playerLimit = -1;
  };
} // namespace GAME
} // namespace KODI
