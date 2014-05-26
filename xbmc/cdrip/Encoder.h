#ifndef _ENCODER_H
#define _ENCODER_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include "addons/include/xbmc_audioenc_types.h"

#define WRITEBUFFER_SIZE 131072 // 128k buffer

namespace XFILE { class CFile; }

class IEncoder
{
public:
  IEncoder() :
    m_iTrackLength(0),
    m_iInChannels(0),
    m_iInSampleRate(0),
    m_iInBitsPerSample(0)
  {
  }
  virtual ~IEncoder() {}
  virtual bool Init(audioenc_callbacks &callbacks) = 0;
  virtual int Encode(int nNumBytesRead, uint8_t* pbtStream) = 0;
  virtual bool Close() = 0;

  // tag info
  std::string m_strComment;
  std::string m_strArtist;
  std::string m_strAlbumArtist;
  std::string m_strTitle;
  std::string m_strAlbum;
  std::string m_strGenre;
  std::string m_strTrack;
  std::string m_strYear;
  std::string m_strFile;
  int m_iTrackLength;
  int m_iInChannels;
  int m_iInSampleRate;
  int m_iInBitsPerSample;
};

class CEncoder
{
public:
  CEncoder(boost::shared_ptr<IEncoder> encoder);
  virtual ~CEncoder();
  virtual bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  virtual int Encode(int nNumBytesRead, uint8_t* pbtStream);
  virtual bool CloseEncode();

  void SetComment(const std::string& str) { m_impl->m_strComment = str; }
  void SetArtist(const std::string& str) { m_impl->m_strArtist = str; }
  void SetTitle(const std::string& str) { m_impl->m_strTitle = str; }
  void SetAlbum(const std::string& str) { m_impl->m_strAlbum = str; }
  void SetAlbumArtist(const std::string& str) { m_impl->m_strAlbumArtist = str; }
  void SetGenre(const std::string& str) { m_impl->m_strGenre = str; }
  void SetTrack(const std::string& str) { m_impl->m_strTrack = str; }
  void SetTrackLength(int length) { m_impl->m_iTrackLength = length; }
  void SetYear(const std::string& str) { m_impl->m_strYear = str; }

  bool FileCreate(const char* filename);
  bool FileClose();
  int FileWrite(const void *pBuffer, uint32_t iBytes);
  int64_t FileSeek(int64_t iFilePosition, int iWhence = SEEK_SET);
protected:

  int WriteStream(const void *pBuffer, uint32_t iBytes);
  int FlushStream();

  static int WriteCallback(void *opaque, uint8_t *data, int size);
  static int64_t SeekCallback(void *opaque, int64_t offset, int whence);

  boost::shared_ptr<IEncoder> m_impl;

  XFILE::CFile *m_file;

  uint8_t m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  uint32_t m_dwWriteBufferPointer;
};

#endif // _ENCODER_H

