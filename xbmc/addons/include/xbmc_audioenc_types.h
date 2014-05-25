#pragma once
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

#ifndef __AUDIOENC_TYPES_H__
#define __AUDIOENC_TYPES_H__

#include <stdint.h>

extern "C"
{
  struct AUDIOENC_INFO
  {
    int dummy;
  };

  struct AUDIOENC_PROPS
  {
    int dummy;
  };

  struct AudioEncoder
  {
    /*! \brief Create encoder context
     \param opaque Pointer passed to write and seek callbacks
     \param write The write callback.
     \param seek The seek callback.
     \return True if a context is created, false on failure.
     \sa IEncoder::Init
     */
    bool (__cdecl* Create) (void *opaque,
                            int (*write)(void* opaque, uint8_t* data, int len),
                            int64_t (*seek)(void* opaque, int64_t pos, int whence));

    //! \brief Start encoder
    //! \param iInChannels Number of channels
    //! \param iInRate Sample rate of input data
    //! \param iInBits Bits per sample in input data
    //! \param title The title of the song
    //! \param artist The artist of the song
    //! \param albumartist The albumartist of the song
    //! \param year The year of the song
    //! \param track The track number of the song
    //! \param genre The genre of the song
    //! \param comment A comment to attach to the song
    //! \param iTrackLength Total track length in seconds
    //! \sa IEncoder::Init
    bool (__cdecl* Start) (int iInChannels, int iInRate, int iInBits,
                           const char* title, const char* artist,
                           const char* albumartist, const char* album,
                           const char* year, const char* track,
                           const char* genre, const char* comment,
                           int iTrackLength);

    //! \brief Encode a chunk of audio
    //! \param nNumBytesRead Number of bytes in input buffer
    //! \param pbtStream the input buffer
    //! \return Number of bytes consumed
    //! \sa IEncoder::Encode
    int  (__cdecl* Encode) (int nNumBytesRead, uint8_t* pbtStream);

    //! \brief Finalize encoding
    //! \return True on success, false on failure.
    bool (__cdecl* Finish) ();

    //! \brief Free encoder context
    //! \param context the encoder context
    void (__cdecl* Free)(void *context);
  };
}

#endif
