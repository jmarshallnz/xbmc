/*
*      Copyright (C) 2005-2014 Team XBMC
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
#include "system.h"
#if defined(HAS_GIFLIB)
#include "Gif.h"
#include "utils/log.h"
#include "guilib/Texture.h"
#include "filesystem/File.h"

#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))
#define GIF_MAX_MEMORY 82944000U // about 79 MB, which is equivalent to 10 full hd frames.

class Gifreader
{
public:
  unsigned char* buffer;
  unsigned int buffSize;
  unsigned int readPosition;

  Gifreader() : buffer(NULL), buffSize(0), readPosition(0) {}
};

int ReadFromMemory(GifFileType* gif, GifByteType* gifbyte, int len)
{
  unsigned int alreadyRead = ((Gifreader*)gif->UserData)->readPosition;
  unsigned int buffSizeLeft = ((Gifreader*)gif->UserData)->buffSize - alreadyRead;
  int readBytes = len;

  if (len <= 0)
    readBytes = 0;

  if (len > buffSizeLeft)
    readBytes = buffSizeLeft;

  if (readBytes > 0)
  {
    unsigned char* src = ((Gifreader*)gif->UserData)->buffer + alreadyRead;
    memcpy(gifbyte, src, readBytes);
    ((Gifreader*)gif->UserData)->readPosition += readBytes;
  }
  return readBytes;
}

int ReadFromVfs(GifFileType* gif, GifByteType* gifbyte, int len)
{
  XFILE::CFile *gifFile = (XFILE::CFile*)gif->UserData;
  return gifFile->Read(gifbyte, len);
}


Gif::Gif() :
  m_imageSize(0),
  m_pitch(0),
  m_loops(0),
  m_numFrames(0),
  m_filename(""),
  m_gif(NULL),
  m_hasBackground(false),
  m_pTemplate(NULL),
  m_isAnimated(-1)
{
  if (!m_dll.Load())
    CLog::Log(LOGERROR, "Gif::Gif(): Could not load giflib");
  memset(&m_backColor, 0, sizeof(m_backColor));
  m_gifFile = new XFILE::CFile();
}

Gif::~Gif()
{
  if (m_dll.IsLoaded())
  {
    int err = m_dll.DGifCloseFile(m_gif);
    if (err == D_GIF_ERR_CLOSE_FAILED)
    {
      CLog::Log(LOGDEBUG, "Gif::~Gif(): D_GIF_ERR_CLOSE_FAILED");
      free(m_gif);
    }
    m_dll.Unload();
    Release();
  }
  delete m_gifFile;
}

void Gif::Release()
{
  delete[] m_pTemplate;
  m_pTemplate = NULL;
  m_globalPalette.clear();
  m_frames.clear();
}

void Gif::ConvertColorTable(std::vector<GifColor> &dest, ColorMapObject* src, unsigned int size)
{
  for (unsigned int i = 0; i < size; ++i)
  {
    GifColor c;

    c.r = src->Colors[i].Red;
    c.g = src->Colors[i].Green;
    c.b = src->Colors[i].Blue;
    c.a = 0xff;
    dest.push_back(c);
  }
}

bool Gif::LoadGifMetaData(GifFileType* file)
{
  if (!m_dll.IsLoaded())
    return false;

  if (!m_dll.DGifSlurp(m_gif))
  {
#if GIFLIB_MAJOR == 5
    char* error = m_dll.GifErrorString(m_gif->Error);
#else
    char* error = m_dll.GifErrorString();
#endif
    if (error)
      CLog::Log(LOGERROR, "Gif::LoadGif(): Could not read file %s - %s", m_filename.c_str(), error);
    else
      CLog::Log(LOGERROR, "Gif::LoadGif(): Could not read file %s (reasons unknown)", m_filename.c_str());
    return false;
  }

  m_height = m_gif->SHeight;
  m_width  = m_gif->SWidth;
  if (!m_height || !m_width)
  {
    CLog::Log(LOGERROR, "Gif::LoadGif(): Zero sized image. File %s", m_filename.c_str());
    return false;
  }

  m_numFrames = m_gif->ImageCount;
  if (m_numFrames > 0)
  {
    ExtensionBlock* extb = m_gif->SavedImages[0].ExtensionBlocks;
    if (extb && extb->Function == APPLICATION_EXT_FUNC_CODE)
    {
      // Read number of loops
      if(++extb && extb->Function == CONTINUE_EXT_FUNC_CODE)
      {
        m_loops = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1],extb->Bytes[2]);
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Gif::LoadGif(): No images found in file %s", m_filename.c_str());
    return false;
  }

  m_pitch     = m_width * sizeof(GifColor);
  m_imageSize = m_pitch * m_height;
  unsigned long memoryUsage = m_numFrames * m_imageSize;
  if (memoryUsage > GIF_MAX_MEMORY)
  {
    // at least 1 image
    m_numFrames = std::max(1U, GIF_MAX_MEMORY / m_imageSize);
    CLog::Log(LOGERROR, "Gif::LoadGif(): Memory consumption too high: %lu bytes. Restricting animation to %u. File %s", memoryUsage, m_numFrames, m_filename.c_str());
  }

  return true;
}

bool Gif::LoadGifMetaData(const char* file)
{
  if (!m_dll.IsLoaded())
    return false;

  m_gifFile->Close();
  char *error = NULL;
  if (m_gifFile->Open(file))
  {
#if GIFLIB_MAJOR == 5
    int err = 0;
    m_gif = m_dll.DGifOpen(m_gifFile, ReadFromVfs, &err);
    if (!m_gif)
      error = m_dll.GifErrorString(err);
#else
    m_gif = m_dll.DGifOpen(m_gifFile, ReadFromVfs);
    if (!m_gif)
      error = m_dll.GifErrorString();
#endif
  }
  if (!m_gif)
  {
    if (error)
      CLog::Log(LOGERROR, "Gif::LoadGif(): Could not open file %s - %s", m_filename.c_str(), error);
    else
      CLog::Log(LOGERROR, "Gif::LoadGif(): Could not open file %s (reasons unknown)", m_filename.c_str());
    return false;
  }
  return LoadGifMetaData(m_gif);
}

bool Gif::LoadGif(const char* file)
{
  m_filename = file;
  if (!LoadGifMetaData(m_filename.c_str()))
    return false;

  try
  {
    InitTemplateAndColormap();

    return ExtractFrames(m_numFrames);
  }
  catch (std::bad_alloc& ba)
  {
    CLog::Log(LOGERROR, "Gif::Load(): Out of memory while reading file %s - %s", m_filename.c_str(), ba.what());
    Release();
    return false;
  }
}

bool Gif::IsAnimated(const char* file)
{
  if (!m_dll.IsLoaded())
    return false;

  if (m_isAnimated < 0)
  {
    m_isAnimated = 0;

    GifFileType *gif = NULL;
    XFILE::CFile gifFile;
    if (gifFile.Open(file))
    {
#if GIFLIB_MAJOR == 5
      int err = 0;
      gif = m_dll.DGifOpen(&gifFile, ReadFromMemory, &err);
#else
      gif = m_dll.DGifOpen(&gifFile, ReadFromMemory);
#endif
    }
    if (gif)
    {
      if (m_dll.DGifSlurp(gif) && gif->ImageCount > 1)
        m_isAnimated = 1;
      m_dll.DGifCloseFile(gif);
      gifFile.Close();
    }
  }
  return m_isAnimated > 0;
}

void Gif::InitTemplateAndColormap()
{
  m_pTemplate = new unsigned char[m_imageSize];
  memset(m_pTemplate, 0, m_imageSize);

  if (m_gif->SColorMap)
  {
    m_globalPalette.clear();
    ConvertColorTable(m_globalPalette, m_gif->SColorMap, m_gif->SColorMap->ColorCount);

    // draw the canvas
    m_backColor = m_globalPalette[m_gif->SBackGroundColor];
    m_hasBackground = true;

    for (unsigned int i = 0; i < m_height * m_width; ++i)
    {
      unsigned char *dest = m_pTemplate + (i *sizeof(GifColor));
      memcpy(dest, &m_backColor, sizeof(GifColor));
    }
  }
  else
    m_globalPalette.clear();
}

bool Gif::gcbToFrame(GifFrame &frame, unsigned int imgIdx)
{
  int transparent = 0;
#if GIFLIB_MAJOR == 5
  GraphicsControlBlock gcb;
  if (!m_dll.DGifSavedExtensionToGCB(m_gif, imgIdx, &gcb))
  {
    char* error = m_dll.GifErrorString(m_gif->Error);
    if (error)
      CLog::Log(LOGERROR, "Gif::ExtractFrames(): Could not read GraphicsControlBlock of frame %d - %s", imgIdx, error);
    else
      CLog::Log(LOGERROR, "Gif::ExtractFrames(): Could not read GraphicsControlBlock of frame %d (reasons unknown)", imgIdx);
    return false;
  }
  // delay in ms
  frame.m_delay = gcb.DelayTime * 10;
  frame.m_disposal = gcb.DisposalMode;
  transparent = gcb.TransparentColor;
#else
  ExtensionBlock* extb = m_gif->SavedImages[imgIdx].ExtensionBlocks;
  while (extb && extb->Function != GRAPHICS_EXT_FUNC_CODE)
    extb++;

  if (extb)
  {
    frame.m_delay = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1], extb->Bytes[2]) * 10;
    frame.m_disposal = (extb->Bytes[0] >> 2) & 0x7;
    if (extb->Bytes[0] & 0x1)
      transparent = extb->Bytes[3];
    else
      transparent = -1;
  }
#endif
  if (transparent >= 0 && (unsigned)transparent < frame.m_palette.size())
    frame.m_palette[transparent].a = 0;
  return true;
}

bool Gif::ExtractFrames(unsigned int count)
{
  if (!m_gif)
    return false;

  if (!m_pTemplate)
  {
    CLog::Log(LOGDEBUG, "Gif::ExtractFrames(): No frame template available");
    return false;
  }

  for (unsigned int i = 0; i < count; i++)
  {
    FramePtr frame(new GifFrame);
    SavedImage savedImage = m_gif->SavedImages[i];
    GifImageDesc imageDesc = m_gif->SavedImages[i].ImageDesc;
    frame->m_height = imageDesc.Height;
    frame->m_width = imageDesc.Width;
    frame->m_top = imageDesc.Top;
    frame->m_left = imageDesc.Left;

    if (frame->m_top + frame->m_height > m_height || frame->m_left + frame->m_width > m_width
      || !frame->m_width || !frame->m_height)
    {
      CLog::Log(LOGDEBUG, "Gif::ExtractFrames(): Illegal frame dimensions: width: %d, height: %d, left: %d, top: %d instead of (%d,%d)",
        frame->m_width, frame->m_height, frame->m_left, frame->m_top, m_width, m_height);
      return false;
    }

    if (imageDesc.ColorMap)
    {
      frame->m_palette.clear();
      ConvertColorTable(frame->m_palette, imageDesc.ColorMap, imageDesc.ColorMap->ColorCount);
      // TODO save a backup of the palette for frames without a table in case there's no gloabl table.
    }
    else if (m_gif->SColorMap)
    {
      frame->m_palette = m_globalPalette;
    }

    // fill delay, disposal and transparent color into frame
    if (!gcbToFrame(*frame, i))
      return false;

    frame->m_pImage = new unsigned char[m_imageSize];
    frame->m_imageSize = m_imageSize;
    memcpy(frame->m_pImage, m_pTemplate, m_imageSize);

    ConstructFrame(*frame, savedImage.RasterBits);

    if(!PrepareTemplate(*frame))
      return false;

    m_frames.push_back(frame);
  }
  return true;
}

void Gif::ConstructFrame(GifFrame &frame, const unsigned char* src) const
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = frame.m_pImage + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));

    const unsigned char *from = src + (src_y * frame.m_width);
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      GifColor col = frame.m_palette[*from++];
      if (col.a != 0)
      {
        *to++ = col.b;
        *to++ = col.g;
        *to++ = col.r;
        *to++ = col.a;
      }
      else
      {
        to += 4;
      }
    }
  }
}

bool Gif::PrepareTemplate(const GifFrame &frame)
{
  switch (frame.m_disposal)
  {
    /* No disposal specified. */
  case DISPOSAL_UNSPECIFIED:
    /* Leave image in place */
  case DISPOSE_DO_NOT:
    memcpy(m_pTemplate, frame.m_pImage, m_imageSize);
    break;

    /* Set area too background color */
  case DISPOSE_BACKGROUND:
    {
      if (!m_hasBackground)
      {
        CLog::Log(LOGDEBUG, "Gif::PrepareTemplate(): Disposal method DISPOSE_BACKGROUND encountered, but the gif has no background.");
        return false;
      }
      SetFrameAreaToBack(m_pTemplate, frame);
      break;
    }
    /* Restore to previous content */
  case DISPOSE_PREVIOUS:
    {
      bool valid = false;

      for (int i = m_frames.size() - 1 ; i >= 0; --i)
      {
        if (m_frames[i]->m_disposal != DISPOSE_PREVIOUS)
        {
          memcpy(m_pTemplate, m_frames[i]->m_pImage, m_imageSize);
          valid = true;
          break;
        }
      }
      if (!valid)
      {
        CLog::Log(LOGDEBUG, "Gif::PrepareTemplate(): Disposal method DISPOSE_PREVIOUS encountered, but could not find a suitable frame.");
        return false;
      }
      break;
    }
  default:
    {
      CLog::Log(LOGDEBUG, "Gif::PrepareTemplate(): Unknown disposal method: %d", frame.m_disposal);
      return false;
    }
  }
  return true;
}

void Gif::SetFrameAreaToBack(unsigned char* dest, const GifFrame &frame)
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = dest + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      memcpy(to, &m_backColor, sizeof(m_backColor));
      to += 4;
    }
  }
}

bool Gif::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)
{
  if (!m_dll.IsLoaded())
    return false;

  if (!buffer || !bufSize || !width || !height)
    return false;

  Gifreader reader;
  reader.buffer = buffer;
  reader.buffSize = bufSize;

  int err = 0;
#if GIFLIB_MAJOR == 5
  m_gif = m_dll.DGifOpen((void *)&reader, (InputFunc)&ReadFromMemory, &err);
#else
  m_gif = m_dll.DGifOpen((void *)&reader, (InputFunc)&ReadFromMemory);
#endif
  if (!m_gif)
  {
#if GIFLIB_MAJOR == 5
    char* error = m_dll.GifErrorString(err);
#else
    char* error = m_dll.GifErrorString();
#endif
    if (error)
      CLog::Log(LOGERROR, "Gif::LoadImageFromMemory(): Could not open gif from memory - %s", error);
    else
      CLog::Log(LOGERROR, "Gif::LoadImageFromMemory(): Could not open gif from memory (reasons unknown)");
    return false;
  }

  if (!LoadGifMetaData(m_gif))
    return false;

  m_originalWidth = m_width;
  m_originalHeight = m_height;

  try
  {
    InitTemplateAndColormap();

    if (!ExtractFrames(m_numFrames))
      return false;
  }
  catch (std::bad_alloc& ba)
  {
    CLog::Log(LOGERROR, "Gif::LoadImageFromMemory(): Out of memory while extracting gif frames - %s", ba.what());
    Release();
    return false;
  }

  return true;
}

bool Gif::Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format)
{
  if (m_width == 0 || m_height == 0
    || !m_dll.IsLoaded() || !m_gif
    || format != XB_FMT_A8R8G8B8 || !m_numFrames)
    return false;

  const unsigned char *src = m_frames[0]->m_pImage;
  unsigned char *dst = (unsigned char*)pixels;

  if (pitch == m_pitch)
    memcpy(dst, src, m_imageSize);
  else
  {
    for (unsigned int y = 0; y < m_height; y++)
    {
      memcpy(dst, src, m_pitch);
      src += m_pitch;
      dst += pitch;
    }
  }
  return true;
}

bool Gif::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile,
                                     unsigned char* &bufferout, unsigned int &bufferoutSize)
{
  CLog::Log(LOGERROR, "Gif::CreateThumbnailFromSurface(): Not implemented. Something went wrong, we don't store thumbnails as gifs!");
  return false;
}

GifFrame::GifFrame() :
  m_pImage(NULL),
  m_delay(0),
  m_imageSize(0),
  m_height(0),
  m_width(0),
  m_top(0),
  m_left(0),
  m_disposal(0),
  m_transparent(0)
{}


GifFrame::GifFrame(const GifFrame& src) :
  m_pImage(NULL),
  m_delay(src.m_delay),
  m_imageSize(src.m_imageSize),
  m_height(src.m_height),
  m_width(src.m_width),
  m_top(src.m_top),
  m_left(src.m_left),
  m_disposal(src.m_disposal),
  m_transparent(src.m_transparent)
{
  if (src.m_pImage)
  {
    m_pImage = new unsigned char[m_imageSize];
    memcpy(m_pImage, src.m_pImage, m_imageSize);
  }

  if (src.m_palette.size())
  {
    m_palette = src.m_palette;
  }
}

GifFrame::~GifFrame()
{
  delete[] m_pImage;
  m_pImage = NULL;
}
#endif//HAS_GIFLIB
