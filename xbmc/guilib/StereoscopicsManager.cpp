/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#include <stdlib.h>
#include "StereoscopicsManager.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/Key.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/ISettingCallback.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "rendering/RenderSystem.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "windowing/WindowingFactory.h"


struct StereoModeMap
{
  const char*          name;
  RENDER_STEREO_MODE   mode;
};

static const struct StereoModeMap VideoModeToGuiModeMap[] =
{
  { "mono",                     RENDER_STEREO_MODE_OFF },
  { "left_right",               RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "right_left",               RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "top_bottom",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "bottom_top",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "checkerboard_rl",          RENDER_STEREO_MODE_OFF }, // unsupported
  { "checkerboard_lr",          RENDER_STEREO_MODE_OFF }, // unsupported
  { "row_interleaved_rl",       RENDER_STEREO_MODE_INTERLACED },
  { "row_interleaved_lr",       RENDER_STEREO_MODE_INTERLACED },
  { "col_interleaved_rl",       RENDER_STEREO_MODE_OFF }, // unsupported
  { "col_interleaved_lr",       RENDER_STEREO_MODE_OFF }, // unsupported
  { "anaglyph_cyan_red",        RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN },
  { "anaglyph_green_magenta",   RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA },
  { "block_lr",                 RENDER_STEREO_MODE_OFF }, // unsupported
  { "block_rl",                 RENDER_STEREO_MODE_OFF }, // unsupported
  {}
};

static const struct StereoModeMap StringToGuiModeMap[] =
{
  { "off",                      RENDER_STEREO_MODE_OFF },
  { "split_vertical",           RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "side_by_side",             RENDER_STEREO_MODE_SPLIT_VERTICAL }, // alias
  { "sbs",                      RENDER_STEREO_MODE_SPLIT_VERTICAL }, // alias
  { "split_horizontal",         RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "over_under",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL }, // alias
  { "tab",                      RENDER_STEREO_MODE_SPLIT_HORIZONTAL }, // alias
  { "row_interleaved",          RENDER_STEREO_MODE_INTERLACED },
  { "interlaced",               RENDER_STEREO_MODE_INTERLACED }, // alias
  { "anaglyph_cyan_red",        RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN },
  { "anaglyph_green_magenta",   RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA },
  { "hardware_based",           RENDER_STEREO_MODE_HARDWAREBASED },
  { "monoscopic",               RENDER_STEREO_MODE_MONO },
  {}
};


CStereoscopicsManager::CStereoscopicsManager(void)
{
  m_lastStereoMode = RENDER_STEREO_MODE_OFF;
}

CStereoscopicsManager::~CStereoscopicsManager(void)
{
}

CStereoscopicsManager& CStereoscopicsManager::Get(void)
{
  static CStereoscopicsManager sStereoscopicsManager;
  return sStereoscopicsManager;
}

void CStereoscopicsManager::Initialize(void)
{
  m_lastStereoMode = GetStereoMode();
  // turn off stereo mode on XBMC startup
  SetStereoMode(RENDER_STEREO_MODE_OFF);
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoMode(void)
{
  return (RENDER_STEREO_MODE) CSettings::Get().GetInt("videoscreen.stereoscopicmode");
}

void CStereoscopicsManager::SetStereoMode(const RENDER_STEREO_MODE &mode)
{
  RENDER_STEREO_MODE currentMode = GetStereoMode();
  if (mode != currentMode && mode >= RENDER_STEREO_MODE_OFF)
  {
    if(!g_Windowing.SupportsStereo(mode))
      return;

    m_lastStereoMode = currentMode;
    CSettings::Get().SetInt("videoscreen.stereoscopicmode", mode);
  }
}

RENDER_STEREO_MODE CStereoscopicsManager::GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step)
{
  RENDER_STEREO_MODE mode = currentMode;
  do {
    mode = (RENDER_STEREO_MODE) ((mode + step) % RENDER_STEREO_MODE_COUNT);
    if(g_Windowing.SupportsStereo(mode))
      break;
   } while (mode != currentMode);
  return mode;
}

std::string CStereoscopicsManager::DetectStereoModeByString(const std::string &needle)
{
  std::string stereoMode;
  CStdString searchString(needle);
  CStdStringArray tags;
  StringUtils::ToUpper(searchString);

  CStdString tag( g_advancedSettings.m_stereoscopicflags_sbs );
  if (stereoMode.empty() && !tag.IsEmpty())
  {
    StringUtils::ToUpper(tag);
    StringUtils::SplitString(tag, "|", tags);
    if (StringUtils::ContainsKeyword(searchString, tags))
      stereoMode = "left_right";
  }

  tag = g_advancedSettings.m_stereoscopicflags_tab;
  if (stereoMode.empty() && !tag.IsEmpty())
  {
    StringUtils::ToUpper(tag);
    StringUtils::SplitString(tag, "|", tags);
    if (StringUtils::ContainsKeyword(searchString, tags))
      stereoMode = "top_bottom";
  }

  if (stereoMode.empty())
    stereoMode = "mono";

  CLog::Log(LOGDEBUG, "StereoscopicsManager: Detected stereo mode in string '%s' is '%s'", CURL::GetRedacted(needle).c_str(), stereoMode.c_str());
  return stereoMode;
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeByUserChoice(const CStdString &heading)
{
  RENDER_STEREO_MODE mode = GetStereoMode();
  // if no stereo mode is set already, suggest mode of current video by preselecting it
  if (mode == RENDER_STEREO_MODE_OFF && g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
    mode = GetStereoModeOfPlayingVideo();

  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDlgSelect->Reset();
  if (heading.IsEmpty())
    pDlgSelect->SetHeading(g_localizeStrings.Get(36528).c_str());
  else
    pDlgSelect->SetHeading(heading.c_str());

  // prepare selectable stereo modes
  std::vector<RENDER_STEREO_MODE> selectableModes;
  for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE selectableMode = (RENDER_STEREO_MODE) i;
    if (g_Windowing.SupportsStereo(selectableMode))
    {
      selectableModes.push_back(selectableMode);
      CStdString label = g_localizeStrings.Get(36502+i);
      pDlgSelect->Add( label );
      if (mode == selectableMode)
        pDlgSelect->SetSelected( label );
    }
  }

  pDlgSelect->DoModal();

  int iItem = pDlgSelect->GetSelectedLabel();
  if (iItem > -1 && pDlgSelect->IsConfirmed())
    mode = (RENDER_STEREO_MODE) selectableModes[iItem];
  else
    mode = GetStereoMode();

  return mode;
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeOfPlayingVideo(void)
{
  RENDER_STEREO_MODE mode = RENDER_STEREO_MODE_OFF;

  CStdString playerMode = g_infoManager.GetLabel(VIDEOPLAYER_STEREOSCOPIC_MODE);
  if (!playerMode.IsEmpty())
  {
    int convertedMode = ConvertVideoToGuiStereoMode(playerMode);
    if (convertedMode > -1)
      mode = (RENDER_STEREO_MODE) convertedMode;
  }

  CLog::Log(LOGDEBUG, "StereoscopicsManager: autodetected GUI stereo mode for movie mode %s is: %s", playerMode.c_str(), GetLabelForStereoMode(mode).c_str());
  return mode;
}

CStdString CStereoscopicsManager::GetLabelForStereoMode(const RENDER_STEREO_MODE &mode)
{
  return g_localizeStrings.Get(36502 + mode);
}

RENDER_STEREO_MODE CStereoscopicsManager::GetPreferredPlaybackMode(void)
{
  RENDER_STEREO_MODE playbackMode = m_lastStereoMode;
  int preferredMode = CSettings::Get().GetInt("videoscreen.preferedstereoscopicmode");
  if (preferredMode == RENDER_STEREO_MODE_AUTO) // automatic mode, detect by movie
  {
    if (g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
      playbackMode = GetStereoModeOfPlayingVideo();
    else if (playbackMode == RENDER_STEREO_MODE_OFF)
      playbackMode = GetNextSupportedStereoMode(RENDER_STEREO_MODE_OFF);
  }
  else // predefined mode
  {
    playbackMode = (RENDER_STEREO_MODE) preferredMode;
  }
  return playbackMode;
}

int CStereoscopicsManager::ConvertVideoToGuiStereoMode(const std::string &mode)
{
  size_t i = 0;
  while (VideoModeToGuiModeMap[i].name)
  {
    if (mode == VideoModeToGuiModeMap[i].name)
      return VideoModeToGuiModeMap[i].mode;
    i++;
  }
  return -1;
}

int CStereoscopicsManager::ConvertStringToGuiStereoMode(const std::string &mode)
{
  size_t i = 0;
  while (StringToGuiModeMap[i].name)
  {
    if (mode == StringToGuiModeMap[i].name)
      return StringToGuiModeMap[i].mode;
    i++;
  }
  return ConvertVideoToGuiStereoMode(mode);
}

const char* CStereoscopicsManager::ConvertGuiStereoModeToString(const RENDER_STEREO_MODE &mode)
{
  size_t i = 0;
  while (StringToGuiModeMap[i].name)
  {
    if (StringToGuiModeMap[i].mode == mode)
      return StringToGuiModeMap[i].name;
    i++;
  }
  return "";
}

std::string CStereoscopicsManager::NormalizeStereoMode(const std::string &mode)
{
  // normalize stereo mode
  if (!mode.empty() && mode != "mono")
  {
    int guiMode = ConvertStringToGuiStereoMode(mode);
    if (guiMode > -1)
      return ConvertGuiStereoModeToString((RENDER_STEREO_MODE) guiMode);
    else
      return mode;
  }
  return "mono";
}

CAction CStereoscopicsManager::ConvertActionCommandToAction(const std::string &command, const std::string &parameter)
{
  if (command == "SetStereoMode")
  {
    int actionId = -1;
    if (parameter == "next")
      actionId = ACTION_STEREOMODE_NEXT;
    else if (parameter == "previous")
      actionId = ACTION_STEREOMODE_PREVIOUS;
    else if (parameter == "toggle")
      actionId = ACTION_STEREOMODE_TOGGLE;
    else if (parameter == "select")
      actionId = ACTION_STEREOMODE_SELECT;
    else if (parameter == "tomono")
      actionId = ACTION_STEREOMODE_TOMONO;

    // already have a valid actionID return it
    if (actionId > -1)
      return CAction(actionId);

    // still no valid action ID, check if parameter is a supported stereomode
    if (ConvertStringToGuiStereoMode(parameter) > -1)
      return CAction(ACTION_STEREOMODE_SET, parameter);
  }
  return CAction(ACTION_NONE);
}

void CStereoscopicsManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == "videoscreen.stereoscopicmode")
  {
    RENDER_STEREO_MODE mode = GetStereoMode();
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode setting changed to %s", GetLabelForStereoMode(mode).c_str());
    ApplyStereoMode(mode);
  }
}

bool CStereoscopicsManager::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_PLAYBACK_STARTED:
    OnPlaybackStarted();
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    OnPlaybackStopped();
    break;
  }

  return false;
}

bool CStereoscopicsManager::OnAction(const CAction &action)
{
  RENDER_STEREO_MODE mode = GetStereoMode();

  if (action.GetID() == ACTION_STEREOMODE_NEXT)
  {
    SetStereoMode(GetNextSupportedStereoMode(mode));
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_PREVIOUS)
  {
    SetStereoMode(GetNextSupportedStereoMode(mode, RENDER_STEREO_MODE_COUNT - 1));
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_TOGGLE)
  {
    if (mode == RENDER_STEREO_MODE_OFF)
    {
      RENDER_STEREO_MODE targetMode = m_lastStereoMode;
      if (targetMode == RENDER_STEREO_MODE_OFF)
        targetMode = GetPreferredPlaybackMode();
      SetStereoMode(targetMode);
    }
    else
    {
      SetStereoMode(RENDER_STEREO_MODE_OFF);
    }
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_SELECT)
  {
    SetStereoMode(GetStereoModeByUserChoice());
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_TOMONO)
  {
    if (mode == RENDER_STEREO_MODE_MONO)
    {
      RENDER_STEREO_MODE targetMode = m_lastStereoMode;
      if (targetMode == RENDER_STEREO_MODE_OFF)
        targetMode = GetPreferredPlaybackMode();
      SetStereoMode(targetMode);
    }
    else
    {
      SetStereoMode(RENDER_STEREO_MODE_MONO);
    }
  }
  else if (action.GetID() == ACTION_STEREOMODE_SET)
  {
    int stereoMode = ConvertStringToGuiStereoMode(action.GetName());
    if (stereoMode > -1)
      SetStereoMode( (RENDER_STEREO_MODE) stereoMode);
  }

  return false;
}

void CStereoscopicsManager::ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify)
{
  RENDER_STEREO_MODE currentMode = g_graphicsContext.GetStereoMode();
  CLog::Log(LOGDEBUG, "StereoscopicsManager::ApplyStereoMode: trying to apply stereo mode. Current: %s | Target: %s", GetLabelForStereoMode(currentMode).c_str(), GetLabelForStereoMode(mode).c_str());
  if (currentMode != mode)
  {
    g_graphicsContext.SetStereoMode(mode);
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode changed to %s", GetLabelForStereoMode(mode).c_str());
    if (notify)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36501), GetLabelForStereoMode(mode));
  }
}

void CStereoscopicsManager::OnPlaybackStarted(void)
{
  if (!g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
    return;

  // only change stereo mode if not yet in stereo mode
  RENDER_STEREO_MODE mode = GetStereoMode();
  if (mode != RENDER_STEREO_MODE_OFF)
    return;

  int playbackMode = CSettings::Get().GetInt("videoplayer.stereoscopicplaybackmode");
  switch (playbackMode)
  {
  case 0: // Ask
    {
      CApplicationMessenger::Get().MediaPause();

      CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
      pDlgSelect->Reset();
      pDlgSelect->SetHeading(g_localizeStrings.Get(36527).c_str());

      RENDER_STEREO_MODE preferred = GetPreferredPlaybackMode();
      RENDER_STEREO_MODE playing   = GetStereoModeOfPlayingVideo();

      int idx_playing   = -1
        , idx_mono      = -1;
        

      // add choices
      int idx_preferred = pDlgSelect->Add( g_localizeStrings.Get(36530)
                                     + " ("
                                     + GetLabelForStereoMode(preferred)
                                     + ")");

      if(preferred != RENDER_STEREO_MODE_MONO)
        idx_mono = pDlgSelect->Add( g_localizeStrings.Get(36529) ); // mono / 2d


      if(playing != RENDER_STEREO_MODE_OFF && g_Windowing.SupportsStereo(playing))
        idx_playing = pDlgSelect->Add( g_localizeStrings.Get(36532)
                                    + " ("
                                    + GetLabelForStereoMode(playing)
                                    + ")");

      int idx_select = pDlgSelect->Add( g_localizeStrings.Get(36531) ); // other / select

      pDlgSelect->DoModal();

      if(pDlgSelect->IsConfirmed())
      {
        int iItem = pDlgSelect->GetSelectedLabel();
        if      (iItem == idx_preferred) mode = preferred;
        else if (iItem == idx_mono)      mode = RENDER_STEREO_MODE_MONO;
        else if (iItem == idx_playing)   mode = playing;
        else if (iItem == idx_select)    mode = GetStereoModeByUserChoice();

        SetStereoMode(mode);
      }

      CApplicationMessenger::Get().MediaUnPause();
    }
    break;
  case 1: // Stereoscopic
    SetStereoMode( GetPreferredPlaybackMode() );
    break;
  default:
    break;
  }
}

void CStereoscopicsManager::OnPlaybackStopped(void)
{
  RENDER_STEREO_MODE mode = GetStereoMode();
  if (CSettings::Get().GetBool("videoplayer.quitstereomodeonstop") == true && mode != RENDER_STEREO_MODE_OFF)
  {
    SetStereoMode(RENDER_STEREO_MODE_OFF);
  }
}
