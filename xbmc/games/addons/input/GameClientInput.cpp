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

#include "GameClientInput.h"
#include "GameClientHardware.h"
#include "GameClientJoystick.h"
#include "GameClientKeyboard.h"
#include "GameClientMouse.h"
#include "GameClientPort.h"
#include "GameClientTopology.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"
#include "games/GameServices.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "peripherals/PeripheralTypes.h" //! @todo
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient &gameClient,
                                   AddonInstance_Game &addonStruct,
                                   CCriticalSection &clientAccess) :
  CGameClientSubsystem(gameClient, addonStruct, clientAccess)
{
}

CGameClientInput::~CGameClientInput()
{
  Deinitialize();
}

void CGameClientInput::Initialize()
{
  LoadTopology();

  // Ensure hardware is open to receive events
  m_hardware.reset(new CGameClientHardware(m_gameClient));
}

void CGameClientInput::Deinitialize()
{
  m_hardware.reset();

  CloseKeyboard();

  CloseMouse();
}

bool CGameClientInput::AcceptsInput() const
{
  return g_windowManager.GetActiveWindowID() == WINDOW_FULLSCREEN_GAME;
}

void CGameClientInput::LoadTopology()
{
  game_input_topology *topologyStruct = nullptr;

  if (m_gameClient.Initialized())
  {
    try { topologyStruct = m_struct.toAddon.GetTopology(); }
    catch (...) { m_gameClient.LogException("GetTopology()"); }
  }

  GameClientPortVec hardwarePorts;

  if (topologyStruct != nullptr)
  {
    //! @todo Guard against infinite loops provided by the game client

    game_input_port *ports = topologyStruct->ports;
    if (ports != nullptr)
    {
      for (unsigned int i = 0; i < topologyStruct->port_count; i++)
        hardwarePorts.emplace_back(new CGameClientPort(ports[i]));
    }

    m_playerLimit = topologyStruct->player_limit;

    try { m_struct.toAddon.FreeTopology(topologyStruct); }
    catch (...) { m_gameClient.LogException("FreeTopology()"); }
  }

  // If no topology is available, create a default one with a single port that
  // accepts all controllers imported by addon.xml
  if (hardwarePorts.empty())
    hardwarePorts.emplace_back(new CGameClientPort(GetControllers(m_gameClient)));

  CGameClientTopology topology(std::move(hardwarePorts));
  m_controllers = topology.GetControllerTree();
}

void CGameClientInput::OpenJoystick(const std::string &portAddress)
{
  using namespace JOYSTICK;

  auto &joystick = m_joysticks[portAddress];

  const CControllerPortNode &port = m_controllers.GetPort(portAddress);
  if (!port.IsControllerAccepted(portAddress, joystick->ControllerID()))
  {

  }

  ControllerPtr controller = port.ActiveController().Controller();

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open port \"%s\"", portAddress.c_str());
    return;
  }

  /*! @todo
  if (!IsControllerAccepted(portAddress, controller))
  {
    CLog::Log(LOGERROR, "Failed to open port: Invalid controller %s on port \"%s\"",
              controller->ID().c_str(),
              portAddress.c_str());
    return;
  }
  */
}

void CGameClientInput::CloseJoystick(const std::string &portAddress)
{
  // Can't close port if it doesn't exist
  if (m_joysticks.find(portAddress) == m_joysticks.end())
    return;

  //! @todo
  //CServiceBroker::GetGameServices().PortManager().ClosePort(m_joysticks[port].get());

  m_joysticks.erase(portAddress);

  UpdatePort(portAddress, CController::EmptyPtr);
}

bool CGameClientInput::ReceiveInputEvent(const game_input_event& event)
{
  bool bHandled = false;

  switch (event.type)
  {
    case GAME_INPUT_EVENT_MOTOR:
      if (event.port_address != nullptr && event.feature_name != nullptr)
        bHandled = SetRumble(event.port_address, event.feature_name, event.motor.magnitude);
      break;
    default:
      break;
  }

  return bHandled;
}

void CGameClientInput::UpdatePort(const std::string &portAddress, const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  CSingleLock lock(m_clientAccess);

  if (m_gameClient.Initialized())
  {
    if (controller != CController::EmptyPtr)
    {
      std::string strId = controller->ID();

      game_controller controllerStruct = { };

      controllerStruct.controller_id        = strId.c_str();
      controllerStruct.provides_input       = controller->Topology().ProvidesInput();
      controllerStruct.digital_button_count = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
      controllerStruct.analog_button_count  = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
      controllerStruct.analog_stick_count   = controller->FeatureCount(FEATURE_TYPE::ANALOG_STICK);
      controllerStruct.accelerometer_count  = controller->FeatureCount(FEATURE_TYPE::ACCELEROMETER);
      controllerStruct.key_count            = controller->FeatureCount(FEATURE_TYPE::KEY);
      controllerStruct.rel_pointer_count    = controller->FeatureCount(FEATURE_TYPE::RELPOINTER);
      controllerStruct.abs_pointer_count    = controller->FeatureCount(FEATURE_TYPE::ABSPOINTER);
      controllerStruct.motor_count          = controller->FeatureCount(FEATURE_TYPE::MOTOR);

      try { m_struct.toAddon.ConnectController(true, portAddress.c_str(), &controllerStruct); }
      catch (...) { m_gameClient.LogException("ConnectController()"); }
    }
    else
    {
      try { m_struct.toAddon.ConnectController(false, portAddress.c_str(), nullptr); }
      catch (...) { m_gameClient.LogException("ConnectController()"); }
    }
  }
}

/*
void CGameClientInput::OpenKeyboard()
{
  using namespace PERIPHERALS;

  PeripheralVector keyboards; //! @todo Move to player manager
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(keyboards, FEATURE_KEYBOARD);

  if (!keyboards.empty())
    m_keyboard.reset(new CGameClientKeyboard(m_gameClient, m_struct.toAddon, keyboards[0].get()));
}

void CGameClientInput::CloseKeyboard()
{
  m_keyboard.reset();
}

void CGameClientInput::OpenMouse()
{
  m_mouse.reset(new CGameClientMouse(m_gameClient, m_struct.toAddon, &CServiceBroker::GetInputManager()));

  CSingleLock lock(m_clientAccess);

  if (m_gameClient.Initialized())
  {
    std::string strId = m_mouse->ControllerID();

    game_controller controllerStruct = { strId.c_str() };

    try { m_struct.toAddon.UpdatePort(GAME_INPUT_PORT_MOUSE, true, &controllerStruct); }
    catch (...) { m_gameClient.LogException("UpdatePort()"); }
  }
}

void CGameClientInput::CloseMouse()
{
  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try { m_struct.toAddon.UpdatePort(GAME_INPUT_PORT_MOUSE, false, nullptr); }
      catch (...) { m_gameClient.LogException("UpdatePort()"); }
    }
  }

  m_mouse.reset();
}
*/

bool CGameClientInput::SetRumble(const std::string &portAddress, const std::string& feature, float magnitude)
{
  bool bHandled = false;

  auto it = m_joysticks.find(portAddress);
  if (it != m_joysticks.end())
    bHandled = it->second->SetRumble(feature, magnitude);

  return bHandled;
}

ControllerVector CGameClientInput::GetControllers(const CGameClient &gameClient)
{
  using namespace ADDON;

  ControllerVector controllers;

  CGameServices& gameServices = CServiceBroker::GetGameServices();

  const ADDONDEPS& dependencies = gameClient.GetDeps();
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    ControllerPtr controller = gameServices.GetController(it->first);
    if (controller)
      controllers.push_back(controller);
  }

  if (controllers.empty())
  {
    // Use the default controller
    ControllerPtr controller = gameServices.GetDefaultController();
    if (controller)
      controllers.push_back(controller);
  }

  return controllers;
}
