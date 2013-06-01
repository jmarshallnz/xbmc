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
#include "utils/Variant.h"
#include "Parser.h"
#include "API.h"

namespace INFO_WRAPPER
{
  class Variant : public PARSER::Class
  {
    CVariant m;
  public:
    Variant(const int &i) : m(i) {}
    CVariant Value() const { return m; };
    static PARSER::ClassFunction *Create(const std::string &name, const PARSER::Params &params) { return NULL; }
  };

  class String : public PARSER::Class
  {
    std::string m;
  public:
    String(const std::string &str) : m(str) {}
    CVariant Value() const { return CVariant(m); };

    PARSER::Class *Mid(const PARSER::Params &params) const;
    static PARSER::ClassFunction *Create(const std::string &name, const PARSER::Params &params);
  };

  class ClassItem : public PARSER::Class
  {
    API::CListItem m;
  public:
    ClassItem(const API::CListItem &i) : m(i) {}
    
    PARSER::Class *Label(const PARSER::Params &params) const;
    PARSER::Class *Time(const PARSER::Params &params) const;
    static PARSER::ClassFunction *Create(const std::string &name, const PARSER::Params &params);
  };

  class ClassPlaylist : public PARSER::Class
  {
    API::CPlaylist m;
  public:
    ClassPlaylist(const API::CPlaylist &p) : m(p) {}
    
    PARSER::Class *Item(const PARSER::Params &params) const;
    static PARSER::ClassFunction *Create(const std::string &name, const PARSER::Params &params);
  };

  class ClassGlobal : public PARSER::Class, public PARSER::Function
  {
    API::CPlaylist m1;
    API::CPlaylist m2;
  public:
    ClassGlobal();

    PARSER::Class *Playlist(const PARSER::Params &params) const;
    static PARSER::ClassFunction *Create(const std::string &name, const PARSER::Params &params);
  };
};
