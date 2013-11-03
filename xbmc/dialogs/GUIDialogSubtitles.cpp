/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "GUIDialogSubtitles.h"
#include "addons/AddonManager.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIImage.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsManager.h"
#include "settings/VideoSettings.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "Util.h"

using namespace ADDON;
using namespace XFILE;

#define CONTROL_NAMELABEL            100
#define CONTROL_NAMELOGO             110
#define CONTROL_SUBLIST              120
#define CONTROL_SUBSEXIST            130
#define CONTROL_SUBSTATUS            140
#define CONTROL_SERVICELIST          150

/*! \brief simple job to retrieve a directory and store a string (language)
 */
class CDirectoryJob: public CJob
{
public:
  CDirectoryJob(const CURL &url, const std::string &language) : m_url(url), m_language(language)
  {
    m_items = new CFileItemList;
  }
  virtual ~CDirectoryJob()
  {
    delete m_items;
  }
  virtual bool DoWork()
  {
    CDirectory::GetDirectory(m_url.Get(), *m_items);
    return true;
  }
  const CFileItemList *GetItems() const { return m_items; }
  const CURL &GetURL() const { return m_url; }
  const std::string &GetLanguage() const { return m_language; }
private:
  CURL           m_url;
  CFileItemList *m_items;
  std::string    m_language;
};

CGUIDialogSubtitles::CGUIDialogSubtitles(void)
    : CGUIDialog(WINDOW_DIALOG_SUBTITLES, "DialogSubtitles.xml")
{
  m_loadType  = KEEP_IN_MEMORY;
  m_subtitles = new CFileItemList;
  m_serviceItems = new CFileItemList;
  m_pausedOnRun = false;
}

CGUIDialogSubtitles::~CGUIDialogSubtitles(void)
{
  delete m_subtitles;
  delete m_serviceItems;
}

bool CGUIDialogSubtitles::OnMessage(CGUIMessage& message)
{
  if  (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_SUBLIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SUBLIST);
      OnMessage(msg);

      int item = msg.GetParam1();
      if (item >= 0 && item < m_subtitles->Size())
        Download(*m_subtitles->Get(item));
      return true;
    }
    else if (iControl == CONTROL_SERVICELIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SERVICELIST);
      OnMessage(msg);

      if (SetService(msg.GetParam1()))
        Search();

      return true;
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    // Resume the video if the user has requested it
    if (g_application.m_pPlayer->IsPaused() && m_pausedOnRun)
      g_application.m_pPlayer->Pause();

    CGUIDialog::OnMessage(message);

    ClearSubtitles();
    ClearServices();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSubtitles::OnInitWindow()
{
  // Pause the video if the user has requested it
  m_pausedOnRun = false;
  if (CSettings::Get().GetBool("subtitles.pauseonsearch") && !g_application.m_pPlayer->IsPaused())
  {
    g_application.m_pPlayer->Pause();
    m_pausedOnRun = true;
  }

  FillServices();
  CGUIWindow::OnInitWindow();
  Search();
}

void CGUIDialogSubtitles::FillServices()
{
  ClearServices();

  VECADDONS addons;
  ADDON::CAddonMgr::Get().GetAddons(ADDON_SUBTITLE_MODULE, addons, true);

  if (addons.empty())
  {
    UpdateStatus(NO_SERVICES);
    return;
  }

  for (VECADDONS::const_iterator addonIt = addons.begin(); addonIt != addons.end(); addonIt++)
  {
    CFileItemPtr item(CAddonsDirectory::FileItemFromAddon(*addonIt, "plugin://", false));
    m_serviceItems->Add(item);
  }

  // Bind our services to the UI
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SERVICELIST, 0, 0, m_serviceItems);
  OnMessage(msg);

  // TODO: Default service support will need to check through the items to find the CFileItem in the loop above.
  SetService(0);
}

bool CGUIDialogSubtitles::SetService(int item)
{
  if (item < 0 || item >= m_serviceItems->Size())
    return false;

  CFileItemPtr selectedItem = m_serviceItems->Get(item);
  if (selectedItem->GetProperty("Addon.ID").asString() != m_currentService)
  {
    m_currentService = selectedItem->GetProperty("Addon.ID").asString();
    CLog::Log(LOGDEBUG, "New Service [%s] ", m_currentService.c_str());

    // highlight this item in the skin
    for (int i = 0; i < m_serviceItems->Size(); i++)
      m_serviceItems->Get(i)->Select(i == item);

    SET_CONTROL_LABEL(CONTROL_NAMELABEL, selectedItem->GetLabel());

    CGUIImage* image = (CGUIImage*)GetControl(CONTROL_NAMELOGO);
    if (image)
      image->SetFileName(selectedItem->GetArt("thumb"));

    if (g_application.m_pPlayer->GetSubtitleCount() == 0)
      SET_CONTROL_HIDDEN(CONTROL_SUBSEXIST);
    else
      SET_CONTROL_VISIBLE(CONTROL_SUBSEXIST);

    return true;
  }
  return false;
}

const CFileItemPtr CGUIDialogSubtitles::GetService() const
{
  for (int i = 0; i < m_serviceItems->Size(); i++)
  {
    if (m_serviceItems->Get(i)->GetProperty("Addon.ID") == m_currentService)
      return m_serviceItems->Get(i);
  }
  return CFileItemPtr();
}

void CGUIDialogSubtitles::Search()
{
  if (m_currentService.empty())
    return; // no services available

  UpdateStatus(SEARCHING);
  ClearSubtitles();

  CURL url("plugin://" + m_currentService + "/");
  url.SetOption("action", "search");

  // TODO: Hook up preferred language settings
//  CStdString strLanguages = StringUtils::Join(CSettings::Get().GetStringList("subtitles.languages"), ",");
//  url.SetOption("languages", strLanguages);
  url.SetOption("languages", "English,Serbian");

  CJobManager::GetInstance().AddJob(new CDirectoryJob(url, ""), this);
}

void CGUIDialogSubtitles::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  const CURL &url             = ((CDirectoryJob *)job)->GetURL();
  const CFileItemList *items  = ((CDirectoryJob *)job)->GetItems();
  const std::string &language = ((CDirectoryJob *)job)->GetLanguage();
  if (url.GetOption("action") == "search")
    OnSearchComplete(items);
  else
    OnDownloadComplete(items, language);
}

void CGUIDialogSubtitles::OnSearchComplete(const CFileItemList *items)
{
  m_subtitles->Assign(*items);

  CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SUBLIST, 0, 0, m_subtitles);
  OnMessage(message);

  UpdateStatus(SEARCH_COMPLETE);
}

void CGUIDialogSubtitles::UpdateStatus(STATUS status)
{
  std::string label;
  switch (status)
  {
    case NO_SERVICES:
      label = g_localizeStrings.Get(24114);
      break;
    case SEARCHING:
      label = g_localizeStrings.Get(24107);
      break;
    case SEARCH_COMPLETE:
      if (!m_subtitles->IsEmpty())
        label = StringUtils::Format(g_localizeStrings.Get(24108).c_str(), m_subtitles->Size());
      else
        label = g_localizeStrings.Get(24109);
      break;
    case DOWNLOADING:
      label = g_localizeStrings.Get(24110);
      break;
    default:
      break;
  }
  SET_CONTROL_LABEL(CONTROL_SUBSTATUS, label);
}

void CGUIDialogSubtitles::Download(const CFileItem &subtitle)
{
  UpdateStatus(DOWNLOADING);

  // subtitle URL should be of the form plugin://<addonid>/?param=foo&param=bar
  // we just append (if not already present) the action=download parameter.
  CURL url(subtitle.GetAsUrl());
  if (url.GetOption("action").empty())
    url.SetOption("action", "download");

  CJobManager::GetInstance().AddJob(new CDirectoryJob(url, subtitle.GetLabel()), this);
}

void CGUIDialogSubtitles::OnDownloadComplete(const CFileItemList *items, const std::string &language)
{
  if (items->IsEmpty())
  {
    CFileItemPtr service = GetService();
    if (service)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, service->GetLabel(), g_localizeStrings.Get(24113));
    UpdateStatus(SEARCH_COMPLETE);
    return;
  }

  CStdString strFileName;
  CStdString strDestPath;
  if (g_application.CurrentFileItem().IsStack())
  {
    for (int i = 0; i < items->Size(); i++)
    {
//    check for all stack items and match to given subs, item [0] == CD1, item [1] == CD2
//    CLog::Log(LOGDEBUG, "Stack Subs [%s} Found", vecItems[i]->GetLabel().c_str());
    }
  }
  else if (StringUtils::StartsWith(g_application.CurrentFile(), "http://"))
  {
    strFileName = "TemporarySubs";
    strDestPath = "special://temp/";
  }
  else
  {
    strFileName = URIUtils::GetFileName(g_application.CurrentFile());
    if (CSettings::Get().GetBool("subtitles.savetomoviefolder"))
    {
      strDestPath = URIUtils::GetDirectory(g_application.CurrentFile());
      if (!CUtil::SupportsWriteFileOperations(strDestPath))
        strDestPath.clear();
    }
    if (strDestPath.empty())
    {
      if (CSpecialProtocol::TranslatePath("special://subtitles").empty())
        strDestPath = "special://temp";
      else
        strDestPath = "special://subtitles";
    }
  }
  // Extract the language and appropriate extension
  CStdString strSubLang;
  g_LangCodeExpander.ConvertToTwoCharCode(strSubLang, language);
  CStdString strUrl = items->Get(0)->GetPath();
  CStdString strSubExt = URIUtils::GetExtension(strUrl);

  // construct subtitle path
  URIUtils::RemoveExtension(strFileName);
  CStdString strSubName;
  strSubName.Format("%s.%s%s", strFileName.c_str(), strSubLang.c_str(), strSubExt.c_str());
  CStdString strSubPath = URIUtils::AddFileToFolder(strDestPath, strSubName);

  // and copy the file across
  CFile::Cache(strUrl, strSubPath);
  SetSubtitles(strSubPath);
  // Close the window
  Close();
}

void CGUIDialogSubtitles::ClearSubtitles()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SUBLIST);
  OnMessage(msg);
  m_subtitles->Clear();
}

void CGUIDialogSubtitles::ClearServices()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SERVICELIST);
  OnMessage(msg);
  m_serviceItems->Clear();
  m_currentService.clear();
}

void CGUIDialogSubtitles::SetSubtitles(const std::string &subtitle)
{
  if (g_application.m_pPlayer)
  {
    int nStream = g_application.m_pPlayer->AddSubtitle(subtitle);
    if(nStream >= 0)
    {
      g_application.m_pPlayer->SetSubtitle(nStream);
      g_application.m_pPlayer->SetSubtitleVisible(true);
      CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay = 0.0f;
      g_application.m_pPlayer->SetSubTitleDelay(0);
    }
  }
}
