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

#include <string>

namespace KODI
{
namespace HARDWARE
{
  /*!
   * \ingroup hardware
   * \brief Handles events for hardware such as reset buttons on a game console
   */
  class IHardwareInput
  {
  public:
    virtual ~IHardwareInput() = default;
    
    /*!
     * \brief A hardware reset button has been pressed
     *
     * \param portAddress The port belonging to the user who pressed the reset
     *                    button, or "1" (the default port) if unknown
     *
     * @todo Change defaulting mechanism
     */
    virtual void OnResetButton(const std::string &portAddress) = 0;
  };
}
}
