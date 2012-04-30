#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogBoxBase.h"
#include "Utils/Variant.h"

/* Callback declaration
 This would be better done using something like boost::function() so we can bind member functions
 (using boost::bind, though I think there's uglier stdlib tricks as well) directly instead of
 having a hacky static that calls the class we want.  This will also allow the owner param to
 be dropped (essentially boost::function holds the callback class + function).
 */
typedef void (*DialogCallback) (bool confirmed, void *owner, const CVariant &data);

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogYesNo(int overrideId = -1);
  virtual ~CGUIDialogYesNo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);

  void ShowModal(DialogCallback callback, void *owner, const CVariant &data);
  virtual void OnDeinitWindow(int nextWindowID);

  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel=-1, int iYesLabel=-1);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, bool& bCanceled);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel, bool& bCanceled, unsigned int autoCloseTime = 0);
  static bool ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, const CStdString& noLabel="", const CStdString& yesLabel="");
  static bool ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, bool &bCanceled, const CStdString& noLabel="", const CStdString& yesLabel="");
protected:
  bool m_bCanceled;
  DialogCallback  m_callback;
  void *          m_callbackOwner; ///< this shouldn't be needed once something like boost::function is used
  CVariant        m_callbackData;  ///< using a CVariant as there's almost certainly going to be callbacks that require a heap of info
};
