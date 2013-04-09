/*
 *      Copyright (C) 2012 Team XBMC
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

#include "Parser.h"
#include <sstream>

namespace PARSER
{
  class CTokenStream : public TokenStream
  {
  public:
    CTokenStream(const std::string &str);

    virtual void Next() { m_tokenNum++; };
    virtual Token Peek() const;
    virtual Token Get();

  private:
    void ScanQuote(std::istringstream &i);
    void ScanNumber(std::istringstream &i);
    void ScanLabel(std::istringstream &i);
    
    unsigned int m_tokenNum;
    std::vector<Token> m_tokens;
  };
};
