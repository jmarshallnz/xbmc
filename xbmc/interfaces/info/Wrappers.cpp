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

#include "Wrappers.h"

using namespace std;
using namespace API;
using namespace PARSER;

namespace INFO_WRAPPER
{
  Class *String::Mid(const Params &params) const
  {
    return new String(m.substr(params[0].asUnsignedInteger(), params.size() == 2 ? params[1].asUnsignedInteger() : string::npos));
  }

  ClassFunction *String::Create(const string &name, const Params &params)
  {
    if (name == "Mid" && (params.size() == 1 || params.size() == 2))
      return new MemberFunction<String, String>(&String::Mid);
    return NULL;
  }

  Class *ClassItem::Label(const Params &params) const
  {
    return new String(m.GetLabel());
  }

  Class *ClassItem::Time(const Params &params) const
  {
    return new Variant(m.GetTime());
  }

  ClassFunction *ClassItem::Create(const string &name, const Params &params)
  {
    if (name == "Label" && params.size() == 0)
      return new MemberFunction<ClassItem, String>(&ClassItem::Label);
    if (name == "Time" && params.size() == 0)
      return new MemberFunction<ClassItem, Variant>(&ClassItem::Time);
    return NULL;
  }

  Class *ClassPlaylist::Item(const Params &params) const
  {
    CListItem *item = m.Get(params[0].asInteger());
    if (item)
      return new ClassItem(*item);
    return NULL;
  }

  ClassFunction *ClassPlaylist::Create(const string &name, const Params &params)
  {
    if (name == "Item" && params.size() == 1)
      return new MemberFunction<ClassPlaylist, ClassItem>(&ClassPlaylist::Item);
    return NULL;
  }

  ClassGlobal::ClassGlobal() : PARSER::Function("global", vector<PARSER::Atom*>(), new MemberFunction<ClassGlobal, ClassGlobal>())
  {
    m1.Add(CListItem("foo", 1));
    m1.Add(CListItem("bar", 2));
    m2.Add(CListItem("foo2", 1));
    m2.Add(CListItem("bar2", 2));
  }

  Class *ClassGlobal::Playlist(const Params &params) const
  {
    return new ClassPlaylist(params[0] == 1 ? m1 : m2);
  }

  ClassFunction *ClassGlobal::Create(const string &name, const Params &params)
  {
    if (name == "Playlist" && params.size() == 1)
      return new MemberFunction<ClassGlobal, ClassPlaylist>(&ClassGlobal::Playlist);
    return NULL;
  }
}
