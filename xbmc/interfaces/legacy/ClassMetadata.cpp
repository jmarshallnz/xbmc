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

// This file contains some instantiations of class meta data for 
// API objects (AddonClasses) that otherwise only have header files.

#include "RenderCapture.h"
#include "Stat.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    DEF_CLASS_INFO(RenderCapture);
  }

  namespace xbmcvfs
  {
    DEF_CLASS_INFO(Stat);
  }

}
