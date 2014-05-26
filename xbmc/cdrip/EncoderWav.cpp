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

#include "EncoderWav.h"
#include "filesystem/File.h"
#include "utils/log.h"

CEncoderWav::CEncoderWav()
{
  m_iBytesWritten = 0;
  first = true;
  memset(&m_callbacks, 0, sizeof(m_callbacks));
}

bool CEncoderWav::Init(audioenc_callbacks &callbacks)
{
  m_iBytesWritten = 0;

  // we only accept 2 / 44100 / 16 atm
  if (m_iInChannels != 2 ||
      m_iInSampleRate != 44100 ||
      m_iInBitsPerSample != 16)
    return false;

  m_callbacks = callbacks;

  // write dummy header file
  first = true;

  return true;
}

int CEncoderWav::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  if (!m_callbacks.write)
    return -1;

  if (first)
  {
    if (!WriteWavHeader())
      return -1;

    first = false;
  }

  // write the information from the stream directly out to our file
  int numWritten = m_callbacks.write(m_callbacks.opaque, pbtStream, nNumBytesRead);
  if (numWritten < 0)
    return -1;

  m_iBytesWritten += numWritten;
  return numWritten;
}

bool CEncoderWav::WriteWavHeader(uint32_t dataSize)
{
  if (!m_callbacks.write)
    return false;

  // write a dummer wav header
  WAVHDR wav;
  memcpy(wav.riff, "RIFF", 4);
  wav.len = m_iBytesWritten + 44 - 8;
  memcpy(wav.cWavFmt, "WAVEfmt ", 8);
  wav.dwHdrLen = 16;
  wav.wFormat = WAVE_FORMAT_PCM;
  wav.wNumChannels = m_iInChannels;
  wav.dwSampleRate = m_iInSampleRate;
  wav.wBitsPerSample = m_iInBitsPerSample;
  wav.dwBytesPerSec = m_iInSampleRate * m_iInChannels * (m_iInBitsPerSample >> 3);
  wav.wBlockAlign = 4;
  memcpy(wav.cData, "data", 4);
  wav.dwDataLen = dataSize;

  return m_callbacks.write(m_callbacks.opaque, (uint8_t*)&wav, sizeof(WAVHDR));
}

bool CEncoderWav::Close()
{
  if (!m_callbacks.write || !m_callbacks.seek)
    return false;

  // seek back to the start of the file
  if (m_callbacks.seek(m_callbacks.opaque, 0, SEEK_SET) == 0 && WriteWavHeader(m_iBytesWritten))
    return true;

  return false;
}
