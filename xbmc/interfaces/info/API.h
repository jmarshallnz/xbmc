/*
 *      Copyright (C) 2013 Team XBMC
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
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <string>
#include <vector>

namespace API
{
  class CListItem
  {
    std::string label;
    int         time;
  public:
    CListItem(const std::string &l, int t) : label(l), time(t) {};
    const std::string &GetLabel() const { return label; };
    int GetTime() const { return time; };
  };

  class CPlaylist
  {
    std::vector<CListItem *> items;
  public:
    CPlaylist() {};
    void Add(const CListItem &item)
    {
      items.push_back(new CListItem(item));
    }
    ~CPlaylist()
    {
      for (std::vector<CListItem *>::iterator i = items.begin(); i != items.end(); ++i)
        delete *i;
    }
    CListItem *Get(int num) const
    {
      if (num >= 0 && num < items.size())
        return items[num];
      return NULL;
    }
  };
};
