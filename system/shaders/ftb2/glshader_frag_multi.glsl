/*
 *      Copyright (C) 2010 Team XBMC
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

uniform   sampler2D m_samp0;
uniform   sampler2D m_samp1;
varying   vec4      m_cord0;
varying   vec4      m_cord1;
varying   vec4      m_colour;
uniform   float     m_depth;

// SM_MULTI shader
void main ()
{
  vec4 col = (texture2D(m_samp0, m_cord0.xy) * texture2D(m_samp1, m_cord1.xy)).bgra;
  gl_FragColor.rgb = col.rgb * col.a;
  gl_FragColor.a = col.a;
  if (col.a < 0.996)
    gl_FragDepth = 0.0;
  else
    gl_FragDepth = m_depth;
}
