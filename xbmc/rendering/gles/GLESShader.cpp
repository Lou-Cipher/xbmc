/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "GLESShader.h"
#include "xbmc/guilib/MatrixGLES.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "rendering/RenderSystem.h"
#include "guilib/GraphicContext.h"

using namespace Shaders;

CGLESShader::CGLESShader( const char *shader ) : CGLSLShaderProgram("gles_shader.vert", shader)
{
  m_proj = nullptr;
  m_model = nullptr;
  m_clipPossible = false;
}

CGLESShader::CGLESShader(const char *vshader, const char *fshader) : CGLSLShaderProgram(vshader, fshader)
{
  m_proj = nullptr;
  m_model  = nullptr;
  m_clipPossible = false;
}

void CGLESShader::OnCompiledAndLinked()
{
  // This is called after CompileAndLink()

  // Variables passed directly to the Fragment shader
  m_hTex0   = glGetUniformLocation(ProgramHandle(), "m_samp0");
  m_hTex1   = glGetUniformLocation(ProgramHandle(), "m_samp1");
  m_hUniCol = glGetUniformLocation(ProgramHandle(), "m_unicol");
  m_hField  = glGetUniformLocation(ProgramHandle(), "m_field");
  m_hStep   = glGetUniformLocation(ProgramHandle(), "m_step");
  m_hContrast   = glGetUniformLocation(ProgramHandle(), "m_contrast");
  m_hBrightness = glGetUniformLocation(ProgramHandle(), "m_brightness");

  // Variables passed directly to the Vertex shader
  m_hProj  = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hCoord0Matrix = glGetUniformLocation(ProgramHandle(), "m_coord0Matrix");

  // Vertex attributes
  m_hPos    = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hCol    = glGetAttribLocation(ProgramHandle(),  "m_attrcol");
  m_hCord0  = glGetAttribLocation(ProgramHandle(),  "m_attrcord0");
  m_hCord1  = glGetAttribLocation(ProgramHandle(),  "m_attrcord1");

  // It's okay to do this only one time. Textures units never change.
  glUseProgram( ProgramHandle() );
  glUniform1i(m_hTex0, 0);
  glUniform1i(m_hTex1, 1);
  glUniform4f(m_hUniCol, 1.0, 1.0, 1.0, 1.0);

  const float identity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  glUniformMatrix4fv(m_hCoord0Matrix,  1, GL_FALSE, identity);

  glUseProgram( 0 );
}

bool CGLESShader::OnEnabled()
{
  // This is called after glUseProgram()

  GLfloat *projMatrix = glMatrixProject.Get().m_pMatrix;
  GLfloat *modelMatrix = glMatrixModview.Get().m_pMatrix;
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, projMatrix);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, modelMatrix);

  const TransformMatrix &guiMatrix = g_graphicsContext.GetGUIMatrix();
  CRect viewPort; // absolute positions of corners
  CServiceBroker::GetRenderSystem().GetViewPort(viewPort);

  /* glScissor operates in window coordinates. In order that we can use it to
   * perform clipping, we must ensure that there is an independent linear
   * transformation from the coordinate system used by CGraphicContext::ClipRect
   * to window coordinates, separately for X and Y (in other words, no
   * rotation or shear is introduced at any stage). To do, this, we need to
   * check that zeros are present in the following locations:
   *
   * GUI matrix:
   * / * 0 * * \
   * | 0 * * * |
   * \ 0 0 * * /
   *       ^ TransformMatrix::TransformX/Y/ZCoord are only ever called with
   *         input z = 0, so this column doesn't matter
   * Model-view matrix:
   * / * 0 0 * \
   * | 0 * 0 * |
   * | 0 0 * * |
   * \ * * * * /  <- eye w has no influence on window x/y (last column below
   *                                                       is either 0 or ignored)
   * Projection matrix:
   * / * 0 0 0 \
   * | 0 * 0 0 |
   * | * * * * |  <- normalised device coordinate z has no influence on window x/y
   * \ 0 0 * 0 /
   *
   * Some of these zeros are not strictly required to ensure this, but they tend
   * to be zeroed in the common case, so by checking for zeros here, we simplify
   * the calculation of the window x/y coordinates further down the line.
   *
   * (Minor detail: we don't quite deal in window coordinates as defined by
   * OpenGL, because CRenderSystemGLES::SetScissors flips the Y axis. But all
   * that's needed to handle that is an effective negation at the stage where
   * Y is in normalised device coordinates.)
   */
  m_clipPossible = guiMatrix.m[0][1] == 0 &&
      guiMatrix.m[1][0] == 0 &&
      guiMatrix.m[2][0] == 0 &&
      guiMatrix.m[2][1] == 0 &&
      modelMatrix[0+1*4] == 0 &&
      modelMatrix[0+2*4] == 0 &&
      modelMatrix[1+0*4] == 0 &&
      modelMatrix[1+2*4] == 0 &&
      modelMatrix[2+0*4] == 0 &&
      modelMatrix[2+1*4] == 0 &&
      projMatrix[0+1*4] == 0 &&
      projMatrix[0+2*4] == 0 &&
      projMatrix[0+3*4] == 0 &&
      projMatrix[1+0*4] == 0 &&
      projMatrix[1+2*4] == 0 &&
      projMatrix[1+3*4] == 0 &&
      projMatrix[3+0*4] == 0 &&
      projMatrix[3+1*4] == 0 &&
      projMatrix[3+3*4] == 0;

  m_clipXFactor = 0.0;
  m_clipXOffset = 0.0;
  m_clipYFactor = 0.0;
  m_clipYOffset = 0.0;

  if (m_clipPossible)
  {
    m_clipXFactor = guiMatrix.m[0][0] * modelMatrix[0+0*4] * projMatrix[0+0*4];
    m_clipXOffset = (guiMatrix.m[0][3] * modelMatrix[0+0*4] + modelMatrix[0+3*4]) * projMatrix[0+0*4];
    m_clipYFactor = guiMatrix.m[1][1] * modelMatrix[1+1*4] * projMatrix[1+1*4];
    m_clipYOffset = (guiMatrix.m[1][3] * modelMatrix[1+1*4] + modelMatrix[1+3*4]) * projMatrix[1+1*4];
    float clipW = (guiMatrix.m[2][3] * modelMatrix[2+2*4] + modelMatrix[2+3*4]) * projMatrix[3+2*4];
    float xMult = (viewPort.x2 - viewPort.x1) / (2 * clipW);
    float yMult = (viewPort.y1 - viewPort.y2) / (2 * clipW); // correct for inverted window coordinate scheme
    m_clipXFactor = m_clipXFactor * xMult;
    m_clipXOffset = m_clipXOffset * xMult + (viewPort.x2 + viewPort.x1) / 2;
    m_clipYFactor = m_clipYFactor * yMult;
    m_clipYOffset = m_clipYOffset * yMult + (viewPort.y2 + viewPort.y1) / 2;
  }

  glUniform1f(m_hBrightness, 0.0f);
  glUniform1f(m_hContrast, 1.0f);

  return true;
}

void CGLESShader::Free()
{
  // Do Cleanup here
  CGLSLShaderProgram::Free();
}

