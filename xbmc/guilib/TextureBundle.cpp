/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "system.h"
#include "TextureBundle.h"

CTextureBundle::CTextureBundle(void)
{
  m_useXBT = false;
  m_useAtlas = false;
}

CTextureBundle::~CTextureBundle(void)
{
}

bool CTextureBundle::HasFile(const CStdString& Filename)
{
  if (m_useAtlas)
  {
    return m_tbAtlas.HasFile(Filename);
  }
  if (m_useXBT)
  {
    return m_tbXBT.HasFile(Filename);
  }
  else if (m_tbAtlas.HasFile(Filename))
  {
    m_useAtlas = true;
    return true;
  }
  else if (m_tbXBT.HasFile(Filename))
  {
    m_useXBT = true;
    return true;
  }
  else
  {
    return false;
  }
}

void CTextureBundle::GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures)
{
  if (m_useAtlas)
  {
    m_tbAtlas.GetTexturesFromPath(path,textures);
  }
  if (m_useXBT)
  {
    m_tbXBT.GetTexturesFromPath(path, textures);
  }
}

bool CTextureBundle::LoadTexture(const CStdString& Filename, CTextureMap** ppTexture)
{
  if (m_useAtlas)
  {
    return m_tbAtlas.LoadTexture(Filename, ppTexture);
  }
  else if (m_useXBT)
  {
    return m_tbXBT.LoadTexture(Filename, ppTexture);
  }
  else
  {
    return false;
  }
}

bool CTextureBundle::LoadAnim(const CStdString& Filename, CTextureMap** ppTexture)
{
  if (m_useAtlas)
  {
    return m_tbAtlas.LoadTexture(Filename, ppTexture);
  }
  else if (m_useXBT)
  {
    return m_tbXBT.LoadAnim(Filename, ppTexture);
  }
  else
  {
    return false;
  }
}

void CTextureBundle::Cleanup()
{
  m_tbXBT.Cleanup();
  m_tbAtlas.Cleanup();
  m_useXBT = m_useAtlas = false;
}

void CTextureBundle::SetThemeBundle(bool themeBundle)
{
  m_tbXBT.SetThemeBundle(themeBundle);
  m_tbAtlas.SetThemeBundle(themeBundle);
}

CStdString CTextureBundle::Normalize(const CStdString &name)
{
  return CTextureBundleXBT::Normalize(name);
}
