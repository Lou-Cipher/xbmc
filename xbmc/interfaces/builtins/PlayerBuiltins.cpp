/*
 *      Copyright (C) 2005-2015 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlayerBuiltins.h"

#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "SeekHandler.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "view/GUIViewState.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"

#include <math.h>

#ifdef HAS_DVD_DRIVE
#include "Autorun.h"
#endif

/*! \brief Clear current playlist
 *  \param params (ignored)
 */
static int ClearPlaylist(const std::vector<std::string>& params)
{
  CServiceBroker::GetPlaylistPlayer().Clear();

  return 0;
}

/*! \brief Start a playlist from a given offset.
 *  \param params The parameters.
 *  \details params[0] = Position in playlist or playlist type.
 *           params[1] = Position in playlist if params[0] is playlist type (optional).
 */
static int PlayOffset(const std::vector<std::string>& params)
{
  // playlist.playoffset(offset)
  // playlist.playoffset(music|video,offset)
  std::string strPos = params[0];
  std::string paramlow(params[0]);
  StringUtils::ToLower(paramlow);
  if (params.size() > 1)
  {
    // ignore any other parameters if present
    std::string strPlaylist = params[0];
    strPos = params[1];

    int iPlaylist = PLAYLIST_NONE;
    if (paramlow == "music")
      iPlaylist = PLAYLIST_MUSIC;
    else if (paramlow == "video")
      iPlaylist = PLAYLIST_VIDEO;

    // unknown playlist
    if (iPlaylist == PLAYLIST_NONE)
    {
      CLog::Log(LOGERROR,"Playlist.PlayOffset called with unknown playlist: %s", strPlaylist.c_str());
      return false;
    }

    // user wants to play the 'other' playlist
    if (iPlaylist != CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist())
    {
      g_application.StopPlaying();
      CServiceBroker::GetPlaylistPlayer().Reset();
      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlaylist);
    }
  }
  // play the desired offset
  int pos = atol(strPos.c_str());
  // playlist is already playing
  if (g_application.GetAppPlayer().IsPlaying())
    CServiceBroker::GetPlaylistPlayer().PlayNext(pos);
  // we start playing the 'other' playlist so we need to use play to initialize the player state
  else
    CServiceBroker::GetPlaylistPlayer().Play(pos, "");

  return 0;
}

/*! \brief Control player.
 *  \param params The parameters
 *  \details params[0] = Control to execute.
 *           params[1] = "notify" to notify user (optional, certain controls).
 */
static int PlayerControl(const std::vector<std::string>& params)
{
  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();

  std::string paramlow(params[0]);
  StringUtils::ToLower(paramlow);

  if (paramlow ==  "play")
  { // play/pause
    // either resume playing, or pause
    if (g_application.GetAppPlayer().IsPlaying())
    {
      if (g_application.GetAppPlayer().GetPlaySpeed() != 1)
        g_application.GetAppPlayer().SetPlaySpeed(1);
      else
        g_application.GetAppPlayer().Pause();
    }
  }
  else if (paramlow == "stop")
  {
    g_application.StopPlaying();
  }
  else if (StringUtils::StartsWithNoCase(params[0], "frameadvance"))
  {
    std::string strFrames;
    if (params[0].size() == 12)
      CLog::Log(LOGERROR, "PlayerControl(frameadvance(n)) called with no argument");
    else if (params[0].size() < 15) // arg must be at least "(N)"
      CLog::Log(LOGERROR, "PlayerControl(frameadvance(n)) called with invalid argument: \"%s\"", params[0].substr(13).c_str());
    else

    strFrames = params[0].substr(13);
    StringUtils::TrimRight(strFrames, ")");
    float frames = (float) atof(strFrames.c_str());
    g_application.GetAppPlayer().FrameAdvance(frames);
  }
  else if (paramlow =="rewind" || paramlow == "forward")
  {
    if (g_application.GetAppPlayer().IsPlaying() && !g_application.GetAppPlayer().IsPaused())
    {
      float playSpeed = g_application.GetAppPlayer().GetPlaySpeed();

      if (paramlow == "rewind" && playSpeed == 1) // Enables Rewinding
        playSpeed *= -2;
      else if (paramlow == "rewind" && playSpeed > 1) //goes down a notch if you're FFing
        playSpeed /= 2;
      else if (paramlow == "forward" && playSpeed < 1) //goes up a notch if you're RWing
      {
        playSpeed /= 2;
        if (playSpeed == -1)
          playSpeed = 1;
      }
      else
        playSpeed *= 2;

      if (playSpeed > 32 || playSpeed < -32)
        playSpeed = 1;

      g_application.GetAppPlayer().SetPlaySpeed(playSpeed);
    }
  }
  else if (paramlow =="tempoup" || paramlow == "tempodown")
  {
    if (g_application.GetAppPlayer().SupportsTempo() &&
        g_application.GetAppPlayer().IsPlaying() && !g_application.GetAppPlayer().IsPaused())
    {
      float playTempo = g_application.GetAppPlayer().GetPlayTempo();
      if (paramlow == "tempodown")
          playTempo -= 0.1f;
      else if (paramlow == "tempoup")
          playTempo += 0.1f;

      g_application.GetAppPlayer().SetTempo(playTempo);
    }
  }
  else if (paramlow == "next")
  {
    g_application.OnAction(CAction(ACTION_NEXT_ITEM));
  }
  else if (paramlow == "previous")
  {
    g_application.OnAction(CAction(ACTION_PREV_ITEM));
  }
  else if (paramlow == "bigskipbackward")
  {
    if (g_application.GetAppPlayer().IsPlaying())
      g_application.GetAppPlayer().Seek(false, true);
  }
  else if (paramlow == "bigskipforward")
  {
    if (g_application.GetAppPlayer().IsPlaying())
      g_application.GetAppPlayer().Seek(true, true);
  }
  else if (paramlow == "smallskipbackward")
  {
    if (g_application.GetAppPlayer().IsPlaying())
      g_application.GetAppPlayer().Seek(false, false);
  }
  else if (paramlow == "smallskipforward")
  {
    if (g_application.GetAppPlayer().IsPlaying())
      g_application.GetAppPlayer().Seek(true, false);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "seekpercentage"))
  {
    std::string offset;
    if (params[0].size() == 14)
      CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with no argument");
    else if (params[0].size() < 17) // arg must be at least "(N)"
      CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with invalid argument: \"%s\"", params[0].substr(14).c_str());
    else
    {
      // Don't bother checking the argument: an invalid arg will do seek(0)
      offset = params[0].substr(15);
      StringUtils::TrimRight(offset, ")");
      float offsetpercent = (float) atof(offset.c_str());
      if (offsetpercent < 0 || offsetpercent > 100)
        CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) argument, %f, must be 0-100", offsetpercent);
      else if (g_application.GetAppPlayer().IsPlaying())
        g_application.SeekPercentage(offsetpercent);
    }
  }
  else if (paramlow == "showvideomenu")
  {
    if( g_application.GetAppPlayer().IsPlaying() )
      g_application.GetAppPlayer().OnAction(CAction(ACTION_SHOW_VIDEOMENU));
  }
  else if (StringUtils::StartsWithNoCase(params[0], "partymode"))
  {
    std::string strXspPath;
    //empty param=music, "music"=music, "video"=video, else xsp path
    PartyModeContext context = PARTYMODECONTEXT_MUSIC;
    if (params[0].size() > 9)
    {
      if (params[0].size() == 16 && StringUtils::EndsWithNoCase(params[0], "video)"))
        context = PARTYMODECONTEXT_VIDEO;
      else if (params[0].size() != 16 || !StringUtils::EndsWithNoCase(params[0], "music)"))
      {
        strXspPath = params[0].substr(10);
        StringUtils::TrimRight(strXspPath, ")");
        context = PARTYMODECONTEXT_UNKNOWN;
      }
    }
    if (g_partyModeManager.IsEnabled())
      g_partyModeManager.Disable();
    else
      g_partyModeManager.Enable(context, strXspPath);
  }
  else if (paramlow == "random" || paramlow == "randomoff" || paramlow == "randomon")
  {
    // get current playlist
    int iPlaylist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

    // reverse the current setting
    bool shuffled = CServiceBroker::GetPlaylistPlayer().IsShuffled(iPlaylist);
    if ((shuffled && paramlow == "randomon") || (!shuffled && paramlow == "randomoff"))
      return 0;

    // check to see if we should notify the user
    bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
    CServiceBroker::GetPlaylistPlayer().SetShuffle(iPlaylist, !shuffled, notify);

    // save settings for now playing windows
    switch (iPlaylist)
    {
      case PLAYLIST_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistShuffled(CServiceBroker::GetPlaylistPlayer().IsShuffled(iPlaylist));
        CServiceBroker::GetSettings().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistShuffled(CServiceBroker::GetPlaylistPlayer().IsShuffled(iPlaylist));
        CServiceBroker::GetSettings().Save();
      default:
        break;
    }

    // send message
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, CServiceBroker::GetPlaylistPlayer().IsShuffled(iPlaylist));
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "repeat"))
  {
    // get current playlist
    int iPlaylist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    PLAYLIST::REPEAT_STATE previous_state = CServiceBroker::GetPlaylistPlayer().GetRepeat(iPlaylist);

    std::string paramlow(params[0]);
    StringUtils::ToLower(paramlow);

    PLAYLIST::REPEAT_STATE state;
    if (paramlow == "repeatall")
      state = PLAYLIST::REPEAT_ALL;
    else if (paramlow == "repeatone")
      state = PLAYLIST::REPEAT_ONE;
    else if (paramlow == "repeatoff")
      state = PLAYLIST::REPEAT_NONE;
    else if (previous_state == PLAYLIST::REPEAT_NONE)
      state = PLAYLIST::REPEAT_ALL;
    else if (previous_state == PLAYLIST::REPEAT_ALL)
      state = PLAYLIST::REPEAT_ONE;
    else
      state = PLAYLIST::REPEAT_NONE;

    if (state == previous_state)
      return 0;

    // check to see if we should notify the user
    bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
    CServiceBroker::GetPlaylistPlayer().SetRepeat(iPlaylist, state, notify);

    // save settings for now playing windows
    switch (iPlaylist)
    {
      case PLAYLIST_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CServiceBroker::GetSettings().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CServiceBroker::GetSettings().Save();
    }

    // send messages so now playing window can get updated
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "resumelivetv"))
  {
    CFileItem& fileItem(g_application.CurrentFileItem());
    PVR::CPVRChannelPtr channel = fileItem.HasPVRRecordingInfoTag() ? fileItem.GetPVRRecordingInfoTag()->Channel() : PVR::CPVRChannelPtr();

    if (channel)
    {
      CFileItem playItem(channel);
      if (!g_application.PlayMedia(playItem, "", channel->IsRadio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
      {
        CLog::Log(LOGERROR, "ResumeLiveTv could not play channel: %s", channel->ChannelName().c_str());
        return false;
      }
    }
  }
  else if (paramlow == "reset")
  {
    g_application.OnAction(CAction(ACTION_PLAYER_RESET));
  }

  return 0;
}

/*! \brief Play currently inserted DVD.
 *  \param params The parameters.
 *  \details params[0] = "restart" to restart from resume point (optional).
 */
static int PlayDVD(const std::vector<std::string>& params)
{
#ifdef HAS_DVD_DRIVE
  bool restart = false;
  if (!params.empty() && StringUtils::EqualsNoCase(params[0], "restart"))
    restart = true;
  MEDIA_DETECT::CAutorun::PlayDisc(g_mediaManager.GetDiscPath(), true, restart);
#endif

  return 0;
}

/*! \brief Start playback of media.
 *  \param params The parameters.
 *  \details params[0] = URL to media to play (optional).
 *           params[1,...] = "isdir" if media is a directory (optional).
 *           params[1,...] = "1" to start playback in fullscreen (optional).
 *           params[1,...] = "resume" to force resuming (optional).
 *           params[1,...] = "noresume" to force not resuming (optional).
 *           params[1,...] = "playoffset=<offset>" to start playback from a given position in a playlist (optional).
 */
static int PlayMedia(const std::vector<std::string>& params)
{
  CFileItem item(params[0], false);
  if (URIUtils::HasSlashAtEnd(params[0]))
    item.m_bIsFolder = true;

  // restore to previous window if needed
  if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION )
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

  // reset screensaver
  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();

  // ask if we need to check guisettings to resume
  bool askToResume = true;
  int playOffset = 0;
  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    if (StringUtils::EqualsNoCase(params[i], "isdir"))
      item.m_bIsFolder = true;
    else if (params[i] == "1") // set fullscreen or windowed
      CMediaSettings::GetInstance().SetVideoStartWindowed(true);
    else if (StringUtils::EqualsNoCase(params[i], "resume"))
    {
      // force the item to resume (if applicable) (see CApplication::PlayMedia)
      item.m_lStartOffset = STARTOFFSET_RESUME;
      askToResume = false;
    }
    else if (StringUtils::EqualsNoCase(params[i], "noresume"))
    {
      // force the item to start at the beginning (m_lStartOffset is initialized to 0)
      askToResume = false;
    }
    else if (StringUtils::StartsWithNoCase(params[i], "playoffset=")) {
      playOffset = atoi(params[i].substr(11).c_str()) - 1;
      item.SetProperty("playlist_starting_track", playOffset);
    }
  }

  if (!item.m_bIsFolder && item.IsPlugin())
    item.SetProperty("IsPlayable", true);

  if ( askToResume == true )
  {
    if ( CGUIWindowVideoBase::ShowResumeMenu(item) == false )
      return false;
  }
  if (item.m_bIsFolder)
  {
    CFileItemList items;
    std::string extensions = CServiceBroker::GetFileExtensionProvider().GetVideoExtensions() + "|" + CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
    XFILE::CDirectory::GetDirectory(item.GetPath(), items, extensions, XFILE::DIR_FLAG_DEFAULTS);

    bool containsMusic = false, containsVideo = false;
    for (int i = 0; i < items.Size(); i++)
    {
      bool isVideo = items[i]->IsVideo();
      containsMusic |= !isVideo;
      containsVideo |= isVideo;

      if (containsMusic && containsVideo)
        break;
    }

    std::unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(containsVideo ? WINDOW_VIDEO_NAV : WINDOW_MUSIC_NAV, items));
    if (state.get())
      items.Sort(state->GetSortMethod());
    else
      items.Sort(SortByLabel, SortOrderAscending);

    int playlist = containsVideo? PLAYLIST_VIDEO : PLAYLIST_MUSIC;;
    if (containsMusic && containsVideo) //mixed content found in the folder
    {
      for (int i = items.Size() - 1; i >= 0; i--) //remove music entries
      {
        if (!items[i]->IsVideo())
          items.Remove(i);
      }
    }

    CServiceBroker::GetPlaylistPlayer().ClearPlaylist(playlist);
    CServiceBroker::GetPlaylistPlayer().Add(playlist, items);
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlist);
    CServiceBroker::GetPlaylistPlayer().Play(playOffset, "");
  }
  else if (item.IsAudio() || item.IsVideo())
    CServiceBroker::GetPlaylistPlayer().Play(std::make_shared<CFileItem>(item), "");
  else
    g_application.PlayMedia(item, "", PLAYLIST_NONE);

  return 0;
}

/*! \brief Start playback with a given playback core.
 *  \param params The parameters.
 *  \details params[0] = Name of playback core.
 */
static int PlayWith(const std::vector<std::string>& params)
{
  g_application.OnAction(CAction(ACTION_PLAYER_PLAY, params[0]));

  return 0;
}

/*! \brief Seek in currently playing media.
 *  \param params The parameters.
 *  \details params[0] = Number of seconds to seek.
 */
static int Seek(const std::vector<std::string>& params)
{
  if (g_application.GetAppPlayer().IsPlaying())
    g_application.GetAppPlayer().GetSeekHandler().SeekSeconds(atoi(params[0].c_str()));

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_12 Player built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`PlaysDisc(parm)`</b>\n
///     <b>`PlayDVD(param)`</b>(deprecated)
///     ,
///     Plays the inserted disc\, like CD\, DVD or Blu-ray\, in the disc drive.
///     @param[in] param                 "restart" to restart from resume point (optional)
///   }
///   \table_row2_l{
///     <b>`PlayerControl(command[\,param])`</b>
///     ,
///     Allows control of music and videos. The command may be one of Play\, Stop\,
///     Forward\, Rewind\, Next\, Previous\, BigSkipForward\, BigSkipBackward\,
///     SmallSkipForward\, SmallSkipBackward\, Random\, RandomOn\, RandomOff\,
///     Repeat\, RepeatOne\, RepeatAll\, RepeatOff\, Partymode(music) or
///     Partymode(video) or Partymode(path to .xsp file)\, and Record. Play will
///     either pause\, resume\, or stop ffwding or rewinding. Random toggles random
///     playback and Repeat cycles through the repeat modes (these both take an
///     optional second parameter\, Notify\, that notifies the user of the new
///     state). Partymode(music/video) toggles the appropriate partymode\,
///     defaults to music if no parameter is given\, besides the default music or
///     video partymode you can also pass a path to a custom smartplaylist (.xsp)
///     as parameter.
///     @param[in] control               Control to execute.
///     @param[in] param                 "notify" to notify user (optional\, certain controls).
///   }
///   \table_row2_l{
///     <b>`Playlist.Clear`</b>
///     ,
///     Clear the current playlist
///     @param[in]                       (ignored)
///   }
///   \table_row2_l{
///     <b>`Playlist.PlayOffset(positionType[\,position])`</b>
///     ,
///     Start playing from a particular offset in the playlist 
///     @param[in] positionType          Position in playlist or playlist type.
///     @param[in] position              Position in playlist if params[0] is playlist type (optional).
///   }
///   \table_row2_l{
///     <b>`PlayMedia(media[\,isdir][\,1]\,[playoffset=xx])`</b>
///     ,
///     Plays the media. This can be a playlist\, music\, or video file\, directory\,
///     plugin or an Url. The optional parameter "\,isdir" can be used for playing
///     a directory. "\,1" will start a video in a preview window\, instead of
///     fullscreen. If media is a playlist\, you can use playoffset=xx where xx is
///     the position to start playback from.
///     @param[in] media                 URL to media to play (optional).
///     @param[in] isdir                 Set "isdir" if media is a directory (optional).
///     @param[in] fullscreen            Set "1" to start playback in fullscreen (optional).
///     @param[in] resume                Set "resume" to force resuming (optional).
///     @param[in] noresume              Set "noresume" to force not resuming (optional).
///     @param[in] playeroffset          Set "playoffset=<offset>" to start playback from a given position in a playlist (optional).
///   }
///   \table_row2_l{
///     <b>`PlayWith(core)`</b>
///     ,
///     Play the selected item with the specified player core.
///     @param[in] core                  Name of playback core.
///   }
///   \table_row2_l{
///     <b>`Seek(seconds)`</b>
///     ,
///     Seeks to the specified relative amount of seconds within the current
///     playing media. A negative value will seek backward and a positive value forward.
///     @param[in] seconds               Number of seconds to seek.
///   }
/// \table_end
///

CBuiltins::CommandMap CPlayerBuiltins::GetOperations() const
{
  return {
           {"playdisc",            {"Plays the inserted disc, like CD, DVD or Blu-ray, in the disc drive.", 0, PlayDVD}},
           {"playdvd",             {"Plays the inserted disc, like CD, DVD or Blu-ray, in the disc drive.", 0, PlayDVD}},
           {"playlist.clear",      {"Clear the current playlist", 0, ClearPlaylist}},
           {"playlist.playoffset", {"Start playing from a particular offset in the playlist", 1, PlayOffset}},
           {"playercontrol",       {"Control the music or video player", 1, PlayerControl}},
           {"playmedia",           {"Play the specified media file (or playlist)", 1, PlayMedia}},
           {"playwith",            {"Play the selected item with the specified core", 1, PlayWith}},
           {"seek",                {"Performs a seek in seconds on the current playing media file", 1, Seek}}
         };
}
