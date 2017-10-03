/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "PeripheralMouse.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"

#include <sstream>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralMouse::CPeripheralMouse(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus)
{
  // Initialize CPeripheral
  m_features.push_back(FEATURE_JOYSTICK);
}

CPeripheralMouse::~CPeripheralMouse(void)
{
  //m_manager.GetInputManager().UnregisterMouseHandler(this); //! @todo
}

bool CPeripheralMouse::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_MOUSE)
    {
      //m_manager.GetInputManager().RegisterMouseHandler(this); //! @todo
    }
    bSuccess = true;
  }

  return bSuccess;
}

/*
void CPeripheralMouse::RegisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler, bool bPromiscuous)
{
  using namespace KEYBOARD;

  CSingleLock lock(m_mutex);

  if (m_mouseHandlers.find(handler) == m_mouseHandlers.end())
    m_mouseHandlers[handler] = MouseHandle{ new CJoystickEmulation(handler), bPromiscuous };
}

void CPeripheralMouse::UnregisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler)
{
  CSingleLock lock(m_mutex);

  MouseHandlers::iterator it = m_mouseHandlers.find(handler);
  if (it != m_mouseHandlers.end())
  {
    delete it->second.handler;
    m_mouseHandlers.erase(it);
  }
}
*/
