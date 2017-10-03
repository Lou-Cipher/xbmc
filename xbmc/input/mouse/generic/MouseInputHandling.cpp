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

#include "MouseInputHandling.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"

using namespace KODI;
using namespace MOUSE;

CMouseInputHandling::CMouseInputHandling(IMouseInputHandler* handler, JOYSTICK::IButtonMap* buttonMap) :
  m_handler(handler),
  m_buttonMap(buttonMap),
  m_x(0),
  m_y(0)
{
}

bool CMouseInputHandling::OnPosition(int x, int y)
{
  int dx = x - m_x;
  int dy = y - m_y;

  bool bHandled = false;

  /*! @todo
  std::string featureName;
  if (m_buttonMap->GetRelativePointer(featureName))
    bHandled = m_handler->OnMotion(featureName, dx, dy);
  */

  m_x = x;
  m_y = y;

  return bHandled;
}

bool CMouseInputHandling::OnButtonPress(unsigned int button)
{
  bool bHandled = false;

  JOYSTICK::CDriverPrimitive source(JOYSTICK::PRIMITIVE_TYPE::BUTTON, button);

  std::string buttonName;
  if (m_buttonMap->GetFeature(source, buttonName))
    bHandled = m_handler->OnButtonPress(buttonName);

  return bHandled;
}

void CMouseInputHandling::OnButtonRelease(unsigned int button)
{
  JOYSTICK::CDriverPrimitive source(JOYSTICK::PRIMITIVE_TYPE::BUTTON, button);

  std::string buttonName;
  if (m_buttonMap->GetFeature(source, buttonName))
    m_handler->OnButtonRelease(buttonName);
}
