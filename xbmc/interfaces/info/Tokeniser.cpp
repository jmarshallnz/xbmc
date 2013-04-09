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

#include "Tokeniser.h"

using namespace std;

namespace PARSER
{
  CTokenStream::CTokenStream(const string &str)
  {
    m_tokenNum = 0;
    istringstream is(str);
    while (true)
    {
      char ch = is.peek();
      if (!is.good())
        break;
      if (ch == '"')
        ScanQuote(is);
      else if (isdigit(ch))
        ScanNumber(is);
      else if (isalpha(ch))
        ScanLabel(is);
      else if (isspace(ch))
        is.get();
      else if (ch == '!' || ch == '=')
      { // support != and ==
        is.get();
        if (is.peek() == '=')
          m_tokens.push_back(Token(ch == '!' ? NOTEQUALS : EQUALS)), is.get();
        else
          m_tokens.push_back(Token(ch == '!' ? NOT : EQUALS));
      }
      else if (ch == '.')
        m_tokens.push_back(Token(PERIOD)), is.get();
      else if (ch == ',')
        m_tokens.push_back(Token(COMMA)), is.get();
      else if (ch == '(')
        m_tokens.push_back(Token(LBRACKET)), is.get();
      else if (ch == ')')
        m_tokens.push_back(Token(RBRACKET)), is.get();
      else if (ch == '+')
        m_tokens.push_back(Token(PLUS)), is.get();
      else if (ch == '-')
        m_tokens.push_back(Token(MINUS)), is.get();
      else if (ch == '*')
        m_tokens.push_back(Token(TIMES)), is.get();
      else if (ch == '/')
        m_tokens.push_back(Token(DIVIDE)), is.get();
      else if (ch == '|')
        m_tokens.push_back(Token(OR)), is.get();
      else
      { // invalid input
        throw "invalid input";
      }
    }
  }

  void CTokenStream::ScanQuote(istringstream &i)
  {
    if (i.get() == '"')
    {
      string token;
      bool escaped = false;
      while (i.good())
      {
        char ch = i.get();
        if (ch == EOF)
          throw "expected \"";
        if (escaped)
        {
          if (ch == '"' || ch == '\\')
            token.push_back(ch);
          else
            token.push_back('\\');
        }
        else if (ch == '"')
          break;
        if (ch != '\\' && ch != '"')
          token.push_back(ch);
        escaped = escaped ? false : (ch == '\\'); // escape only every second slash
      }
      i.get(); // consume the "
      m_tokens.push_back(Token(Token::STRING, token));
    }
  }

  void CTokenStream::ScanNumber(istringstream &i)
  {
    double num;
    i >> num;
    m_tokens.push_back(Token(num));
  }

  void CTokenStream::ScanLabel(istringstream &i)
  {
    string token;
    if (isalpha(i.peek()))
    {
      while (isalpha(i.peek()) || isdigit(i.peek()))
        token.push_back(i.get());
      if (token == "true")
        m_tokens.push_back(Token(true));
      else if (token == "false")
        m_tokens.push_back(Token(false));
      else
        m_tokens.push_back(Token(Token::LABEL, token));
    }
  }

  Token CTokenStream::Peek() const
  {
    if (m_tokenNum < m_tokens.size())
      return m_tokens[m_tokenNum];
    return Token(Token::END_OF_FILE);
  }

  Token CTokenStream::Get()
  {
    if (m_tokenNum < m_tokens.size())
      return m_tokens[m_tokenNum++];
    return Token(Token::END_OF_FILE);
  }
}