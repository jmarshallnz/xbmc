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

#include "libsquish/squish.h"
#include "system.h"
#include "TextureBundleAtlas.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "utils/log.h"
#include "addons/Skin.h"
#include "settings/GUISettings.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/EndianSwap.h"
#include "utils/URIUtils.h"
#include "XBTF.h"
#include "pictures/DllImageLib.h"
#include <lzo/lzo1x.h>
#include "windowing/WindowingFactory.h"
#include "TextureManager.h"
#include "GUITexture.h"

#ifdef _WIN32
#pragma comment(lib,"liblzo2.lib")
#endif

CTextureBundleAtlas::CTextureBundleAtlas(void)
{
  m_themeBundle = false;
}

CTextureBundleAtlas::~CTextureBundleAtlas(void)
{
  Cleanup();
}

bool CTextureBundleAtlas::OpenBundle()
{
//  Cleanup();

  // Find the correct texture file (skin or theme)
  CStdString strPath;
  CStdString strIndex;

  /*
  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    CStdString theme = g_guiSettings.GetString("lookandfeel.skintheme");
    if (!theme.IsEmpty() && theme.CompareNoCase("SKINDEFAULT"))
    {
      CStdString themeXBT(URIUtils::ReplaceExtension(theme, ".png"));
      CStdString themeIDX(URIUtils::ReplaceExtension(theme, ".idx"));
      strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
      strPath = URIUtils::AddFileToFolder(strPath, themeXBT);
      strIndex = URIUtils::AddFileToFolder(strPath, themeIDX);
    }
    else
    {
      return false;
    }
  }
  else
  {
  */
    strIndex = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media/Textures.xml");
    /*
  }

  strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);
  */

  if (!m_atlasReader.Open(strIndex))
  {
    return false;
  }

  m_TimeStamp = m_atlasReader.GetLastModificationTimestamp();
  CLog::Log(LOGDEBUG, "%s - Opened bundle %s", __FUNCTION__, strIndex.c_str());

  return true;
}

bool CTextureBundleAtlas::HasFile(const CStdString& Filename)
{

  if (!m_atlasReader.IsOpen() && !OpenBundle())
    return false;

  if (m_atlasReader.GetLastModificationTimestamp() > m_TimeStamp)
  {
    CLog::Log(LOGINFO, "Texture bundle has changed, reloading");
    if (!OpenBundle())
      return false;
  }

  CStdString name = Normalize(Filename);
  return m_atlasReader.Exists(name);
}

void CTextureBundleAtlas::GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures)
{
  if (path.GetLength() > 1 && path[1] == ':')
    return;

  if (!m_atlasReader.IsOpen() && !OpenBundle())
    return;

  CStdString testPath = Normalize(path);
  URIUtils::AddSlashAtEnd(testPath);
  int testLength = testPath.GetLength();

  std::vector<CXBTFFile>& files = m_atlasReader.GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    CStdString path = files[i].GetPath();
    if (path.Left(testLength).Equals(testPath))
      textures.push_back(path);
  }
}

bool CTextureBundleAtlas::LoadTexture(const CStdString& Filename, CTextureMap **ppTexture)
{
  if (!m_atlasReader.IsOpen() && !OpenBundle())
    return false;

  CStdString name = Normalize(Filename);

  CXBTFFile* file = m_atlasReader.Find(name);
  if (!file)
    return false;

  // TODO: load the atlas here...
  CStdString strAtlas = "media/" + file->GetAtlas();
  strAtlas = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), strAtlas);

  // TODO: from the atlas, construct a CTextureMap with everything we need
  return false;
}

bool CTextureBundleAtlas::LoadAnim(const CStdString& Filename, CTextureMap **ppTexture)
{
  // TODO: this is probably as simple as calling LoadTexture() to load the atlas?
  return false;
}

void CTextureBundleAtlas::SetThemeBundle(bool themeBundle)
{
  m_themeBundle = themeBundle;
}

void CTextureBundleAtlas::Cleanup()
{
  if (m_atlasReader.IsOpen())
  {
    CLog::Log(LOGDEBUG, "%s - Closed %sbundle", __FUNCTION__, m_themeBundle ? "theme " : "");
  }
}

// normalize to how it's stored within the bundle
// lower case + using forward slash rather than back slash
CStdString CTextureBundleAtlas::Normalize(const CStdString &name)
{
  CStdString newName(name);
  newName.Normalize();
  newName.Replace('\\','/');

  return newName;
}

