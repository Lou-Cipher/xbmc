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

#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"

#include <string>

struct game_input_device;
struct game_input_port;

namespace KODI
{
namespace GAME
{
  class CControllerPort;

  /*!
   * \ingroup games
   * \brief Represents a port that devices can connect to
   */
  class CGameClientPort
  {
  public:
    /*!
     * \brief Construct a hardware port
     *
     * \param port The hardware port Game API struct
     */
    CGameClientPort(const game_input_port &port);

    /*!
     * \brief Construct a hardware port that accepts the given controllers
     *
     * \param controllers List of accepted controller profiles
     *
     * The port is given the ID specified by DEFAULT_PORT_ID. Eventually, we
     * need to implement a better defaulting mechanism.
     */
    CGameClientPort(const ControllerVector &controllers);

    /*!
     * \brief Construct a controller port
     *
     * \param logicalPort The logical port Game API struct
     * \param physicalPort The physical port definition
     *
     * @todo Document logical vs. physical topology here
     */
    CGameClientPort(const game_input_port &logicalPort, const CControllerPort &physicalPort);

    /*!
     * \brief Destructor
     */
    ~CGameClientPort();

    /*!
     * \brief Get the port type
     *
     * The port type identifies if this port is for a keyboard, mouse, or
     * controller.
     */
    PORT_TYPE PortType() const { return m_type; }

    /*!
     * \brief Get the ID of the port
     *
     * The ID is used when creating a toplogical address for the port.
     */
    const std::string &ID() const { return m_portId; }

    /*!
     * \brief Get the list of devices accepted by this port
     */
    const GameClientDeviceVec &Devices() const { return m_acceptedDevices; }

    /*!
     * \brief Check if the port with the specified address is connected to a
     *        controller
     *
     * \param address The port's topological address
     *
     * \return True if the port has a controller attached, false otherwise
     */
    bool HasController(const std::string &address) const;

  private:
    PORT_TYPE m_type;
    std::string m_portId;
    GameClientDeviceVec m_acceptedDevices;
  };
}
}
