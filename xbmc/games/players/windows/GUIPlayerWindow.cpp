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

#include "GUIPlayerWindow.h"
#include "GUIPlayerWindowDefines.h"
#include "addons/AddonManager.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/controllers/types/ControllerGrid.h"
#include "games/players/guicontrols/GUIControllerPanel.h"
#include "games/players/guicontrols/GUIPlayerPanel.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

CGUIPlayerWindow::CGUIPlayerWindow() :
  CGUIDialog(WINDOW_DIALOG_GAME_PLAYERS, "DialogGamePlayers.xml")
{
  // initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIPlayerWindow::~CGUIPlayerWindow() = default;

void CGUIPlayerWindow::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Fade player focus texture when player is being moved

  CGUIDialog::DoProcess(currentTime, dirtyregions);

  //! @todo Unfade player texture and remove focus
}

bool CGUIPlayerWindow::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      break;
    }
    case GUI_MSG_FOCUSED:
    {
      int controlId = message.GetControlId();

      break;
    }
    case GUI_MSG_SETFOCUS:
    {
      int controlId = message.GetControlId();

      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIPlayerWindow::OnInitWindow()
{
  m_playerPanel = dynamic_cast<CGUIPlayerPanel*>(GetControl(CONTROL_PLAYER_PANEL));
  m_controllerPanel = dynamic_cast<CGUIControllerPanel*>(GetControl(CONTROL_CONTROLLER_PANEL));

  GameClientPtr gameClient = GetGameClient();

  if (gameClient)
  {
    CControllerGrid grid;
    grid.SetControllerTree(gameClient->Input().GetControllerTree());

    const unsigned int maxPlayers = grid.Width(); //! @todo: max width

    // Set max players label
    std::string maxPlayersLabel = g_localizeStrings.Get(35226);
    if (!maxPlayersLabel.empty())
    {
      maxPlayersLabel = StringUtils::Format(maxPlayersLabel, maxPlayers);
      SET_CONTROL_LABEL(CONTROL_MAX_PLAYERS_LABEL, maxPlayersLabel);
    }

    // Set emulator name label
    SET_CONTROL_LABEL(CONTROL_EMULATOR_NANE, gameClient->Name());

    // Configure controller panel
    unsigned int itemSize = 0;
    if (m_controllerPanel != nullptr)
    {
      m_controllerPanel->SetGrid(grid);
      itemSize = m_controllerPanel->GetItemSize();
    }

    // Configure player panel
    if (m_playerPanel != nullptr)
    {
      m_playerPanel->SetPlayerCount(maxPlayers);
      m_playerPanel->SetAvatarSize(itemSize);
    }
  }

  CGUIDialog::OnInitWindow();
}

void CGUIPlayerWindow::OnDeinitWindow(int nextWindowID)
{
  //! @todo Destroy GUI elements

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

GameClientPtr CGUIPlayerWindow::GetGameClient() const
{
  using namespace ADDON;

  std::string gameClientId;

  if (g_application.CurrentFileItem().HasGameInfoTag())
    gameClientId = g_application.CurrentFileItem().GetGameInfoTag()->GetGameClient();

  AddonPtr addon;
  if (!gameClientId.empty())
    CServiceBroker::GetAddonMgr().GetAddon(gameClientId, addon, ADDON::ADDON_GAMEDLL);

  return std::static_pointer_cast<CGameClient>(addon);
}
