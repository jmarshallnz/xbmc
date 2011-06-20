/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#if defined(HAS_GL)
#include "GUITextureGL.h"
#endif
#include "Texture.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"

#if defined(HAS_GL)

static bool useShaders = true;

CGUITextureGL::CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureGL::Begin(color_t color)
{
  // Setup Colors
  for (int i = 0; i < 4; i++)
  {
    m_col[i][0] = (GLubyte)GET_R(color);
    m_col[i][1] = (GLubyte)GET_G(color);
    m_col[i][2] = (GLubyte)GET_B(color);
    m_col[i][3] = (GLubyte)GET_A(color);
  }

  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  glActiveTextureARB(GL_TEXTURE0_ARB);
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
  glEnable(GL_TEXTURE_2D);

  if (useShaders)
  {
    GLint posLoc  = g_Windowing.GUIShaderGetPos();
    GLint colLoc  = g_Windowing.GUIShaderGetCol();
    GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();

    glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, m_vert);
    glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, m_col);
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, m_tex0);

    glEnableVertexAttribArray(posLoc);
    glEnableVertexAttribArray(colLoc);
    glEnableVertexAttribArray(tex0Loc);

    bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[0][3] < 255;
    if (g_advancedSettings.m_renderFrontToBack)
      hasAlpha = true;

    if (m_diffuse.size())
    {
      if (m_col[0][0] == 255 && m_col[0][1] == 255 && m_col[0][2] == 255 && m_col[0][3] == 255 )
      {
        g_Windowing.EnableGUIShader(SM_MULTI);
      }
      else
      {
        g_Windowing.EnableGUIShader(SM_MULTI_BLENDCOLOR);
      }

      hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, m_diffuse.m_textures[0]->GetTextureObject());
      glEnable(GL_TEXTURE_2D);

      GLint tex1Loc = g_Windowing.GUIShaderGetCoord1();
      glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, 0, m_tex1);
      glEnableVertexAttribArray(tex1Loc);

      hasAlpha = true;
    }
    else
    {
      if ( hasAlpha )
      {
        g_Windowing.EnableGUIShader(SM_TEXTURE);
      }
      else
      {
        g_Windowing.EnableGUIShader(SM_TEXTURE_NOBLEND);
      }
    }


    if ( hasAlpha )
    {
      if (g_advancedSettings.m_renderFrontToBack)
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
      else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable( GL_BLEND );
    }
    else
    {
      glDisable(GL_BLEND);
    }
  }
  else
  {
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);          // Turn Blending On
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // diffuse coloring
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    VerifyGLState();

    if (m_diffuse.size())
    {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glBindTexture(GL_TEXTURE_2D, m_diffuse.m_textures[0]->GetTextureObject());
      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
      glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
      VerifyGLState();
    }
    //glDisable(GL_TEXTURE_2D); // uncomment these 2 lines to switch to wireframe rendering
    //glBegin(GL_LINE_LOOP);
    glBegin(GL_QUADS);
  }
}

void CGUITextureGL::End()
{
  if (useShaders)
  {
    if (m_diffuse.size())
    {
      glDisable(GL_TEXTURE_2D);
      glDisableVertexAttribArray(g_Windowing.GUIShaderGetCoord1());
      glActiveTexture(GL_TEXTURE0);
    }

    glDisable(GL_TEXTURE_2D);
    glDisableVertexAttribArray(g_Windowing.GUIShaderGetPos());
    glDisableVertexAttribArray(g_Windowing.GUIShaderGetCol());
    glDisableVertexAttribArray(g_Windowing.GUIShaderGetCoord0());

    glEnable(GL_BLEND);
    g_Windowing.DisableGUIShader();
  }
  else
  {
    glEnd();
    if (m_diffuse.size())
    {
      glDisable(GL_TEXTURE_2D);
      glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    glDisable(GL_TEXTURE_2D);
  }
}

void CGUITextureGL::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  if (useShaders)
  {
    GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

    // Setup vertex position values
    for (int i=0; i<4; i++)
    {
      m_vert[i][0] = x[i];
      m_vert[i][1] = y[i];
      m_vert[i][2] = z[i];
    }

    // Setup texture coordinates
    //TopLeft
    m_tex0[0][0] = texture.x1;
    m_tex0[0][1] = texture.y1;
    //TopRight
    if (orientation & 4)
    {
      m_tex0[1][0] = texture.x1;
      m_tex0[1][1] = texture.y2;
    }
    else
    {
      m_tex0[1][0] = texture.x2;
      m_tex0[1][1] = texture.y1;
    }
    //BottomRight
    m_tex0[2][0] = texture.x2;
    m_tex0[2][1] = texture.y2;
    //BottomLeft
    if (orientation & 4)
    {
      m_tex0[3][0] = texture.x2;
      m_tex0[3][1] = texture.y1;
    }
    else
    {
      m_tex0[3][0] = texture.x1;
      m_tex0[3][1] = texture.y2;
    }

    if (m_diffuse.size())
    {
      //TopLeft
      m_tex1[0][0] = diffuse.x1;
      m_tex1[0][1] = diffuse.y1;
      //TopRight
      if (m_info.orientation & 4)
      {
        m_tex1[1][0] = diffuse.x1;
        m_tex1[1][1] = diffuse.y2;
      }
      else
      {
        m_tex1[1][0] = diffuse.x2;
        m_tex1[1][1] = diffuse.y1;
      }
      //BottomRight
      m_tex1[2][0] = diffuse.x2;
      m_tex1[2][1] = diffuse.y2;
      //BottomLeft
      if (m_info.orientation & 4)
      {
        m_tex1[3][0] = diffuse.x2;
        m_tex1[3][1] = diffuse.y1;
      }
      else
      {
        m_tex1[3][0] = diffuse.x1;
        m_tex1[3][1] = diffuse.y2;
      }
    }

    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);
  }
  else
  {
    // Top-left vertex (corner)
    glColor4ub(m_col[0][0], m_col[0][1], m_col[0][2], m_col[0][3]);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y1);
    if (m_diffuse.size())
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y1);
    glVertex3f(x[0], y[0], z[0]);

    // Top-right vertex (corner)
    glColor4ub(m_col[1][0], m_col[1][1], m_col[1][2], m_col[1][3]);
    if (orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
    else
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
    if (m_diffuse.size())
    {
      if (m_info.orientation & 4)
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
      else
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
    }
    glVertex3f(x[1], y[1], z[1]);

    // Bottom-right vertex (corner)
    glColor4ub(m_col[2][0], m_col[2][1], m_col[2][2], m_col[2][3]);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y2);
    if (m_diffuse.size())
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y2);
    glVertex3f(x[2], y[2], z[2]);

    // Bottom-left vertex (corner)
    glColor4ub(m_col[3][0], m_col[3][1], m_col[3][2], m_col[3][3]);
    if (orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
    else
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
    if (m_diffuse.size())
    {
      if (m_info.orientation & 4)
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
      else
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
    }
    glVertex3f(x[3], y[3], z[3]);
  }
}

void CGUITextureGL::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  if (texture)
  {
    glActiveTextureARB(GL_TEXTURE0_ARB);
    texture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
  }
  else
  glDisable(GL_TEXTURE_2D);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // diffuse coloring
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
  VerifyGLState();

  glBegin(GL_QUADS);

  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));

  CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
  glTexCoord2f(coords.x1, coords.y1);
  glVertex3f(rect.x1, rect.y1, 0);
  glTexCoord2f(coords.x2, coords.y1);
  glVertex3f(rect.x2, rect.y1, 0);
  glTexCoord2f(coords.x2, coords.y2);
  glVertex3f(rect.x2, rect.y2, 0);
  glTexCoord2f(coords.x1, coords.y2);
  glVertex3f(rect.x1, rect.y2, 0);

  glEnd();
  if (texture)
    glDisable(GL_TEXTURE_2D);
}

#endif
