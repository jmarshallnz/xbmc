/*!
\file GUIControlGroup.h
\brief
*/

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

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief group of controls, useful for remembering last control + animating/hiding together
 */
class CGUIControlGroup : public CGUIControl
{
public:
  CGUIControlGroup();
  CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIControlGroup(const CGUIControlGroup &from);
  virtual ~CGUIControlGroup(void);
  virtual CGUIControlGroup *Clone() const { return new CGUIControlGroup(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render(const CRect *bounds, CGUIControl const **start);

  /*! \brief Find the last opaque control that covers the given rectangle.
   Used for determining where we should start rendering - if we have a control that covers
   the given render rect and is opaque, then no need to render anything underneath that control.
   \param bounds the rectangle the control must cover.
   \return the last opaque control that covers the rectangle, NULL if no such control exists.
   */
  const CGUIControl *GetLastOpaqueControl(const CRect &bounds) const;

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool SendControlMessage(CGUIMessage& message);
  virtual bool HasFocus() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool CanFocus() const;

  virtual EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void SetInitialVisibility();

  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual bool HasAnimation(ANIMATION_TYPE anim);
  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimations();

  virtual bool HasID(int id) const;
  virtual bool HasVisibleID(int id) const;

  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  const CGUIControl *GetControl(int id) const;
  virtual CGUIControl *GetFirstFocusableControl(int id);
  void GetContainers(std::vector<CGUIControl *> &containers) const;

  virtual void AddControl(CGUIControl *control, int position = -1);
  bool InsertControl(CGUIControl *control, const CGUIControl *insertPoint);
  virtual bool RemoveControl(const CGUIControl *control);
  virtual void ClearAll();
  void SetDefaultControl(int id, bool always) { m_defaultControl = id; m_defaultAlways = always; };
  void SetRenderFocusedLast(bool renderLast) { m_renderFocusedLast = renderLast; };

  virtual void SaveStates(std::vector<CControlState> &states);

  virtual bool IsGroup() const { return true; };

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  /*!
   \brief Check whether a given control is valid
   Runs through controls and returns whether this control is valid.  Only functional
   for controls with non-zero id.
   \param control to check
   \return true if the control is valid, false otherwise.
   */
  bool IsValidControl(const CGUIControl *control) const;

  /*! \brief Get the render order of the child controls
   This gives visible children optionally within the given bounds in the correct render order
   depending on m_renderFocusedLast.
   \param renderList [out] the resulting controls to render
   \param bounds the bounds to render within
   */
  void GetRenderOrder(std::vector<CGUIControl *> &renderList, const CRect *bounds = NULL) const;

  // sub controls
  std::vector<CGUIControl *> m_children;
  typedef std::vector<CGUIControl *>::iterator iControls;
  typedef std::vector<CGUIControl *>::const_iterator ciControls;
  typedef std::vector<CGUIControl *>::reverse_iterator rControls;
  typedef std::vector<CGUIControl *>::const_reverse_iterator crControls;

  // fast lookup by id
  typedef std::multimap<int, CGUIControl *> LookupMap;
  void AddLookup(CGUIControl *control);
  void RemoveLookup(CGUIControl *control);
  const LookupMap &GetLookup() { return m_lookup; };
  LookupMap m_lookup;

  int  m_defaultControl;
  bool m_defaultAlways;
  int m_focusedControl;
  bool m_renderFocusedLast;
};

