#pragma once

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

#include <string>
#include "guilib/GUIDialog.h"
#include "utils/Job.h"

class CFileItem;
class CFileItemList;

class CGUIDialogSubtitles : public CGUIDialog, IJobCallback
{
public:
  CGUIDialogSubtitles(void);
  virtual ~CGUIDialogSubtitles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnInitWindow();

protected:
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  bool SetService(int item);
  const CFileItemPtr GetService() const;
  void FillServices();
  void ClearServices();
  void ClearSubtitles();

  enum STATUS { NO_SERVICES = 0, SEARCHING, SEARCH_COMPLETE, DOWNLOADING };
  void UpdateStatus(STATUS status);

  void Search();
  void OnSearchComplete(const CFileItemList *items);

  void Download(const CFileItem &subtitle);
  void OnDownloadComplete(const CFileItemList *items, const std::string &language);

  void SetSubtitles(const std::string &subtitle);

  CFileItemList* m_subtitles;
  CFileItemList* m_serviceItems;
  std::string    m_currentService;
  bool m_pausedOnRun;
};
