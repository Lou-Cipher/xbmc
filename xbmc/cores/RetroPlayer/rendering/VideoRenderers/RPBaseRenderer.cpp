/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/buffers/IRenderBuffer.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/rendering/VideoShaders/IVideoShaderPreset.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

using namespace KODI;
using namespace RETRO;

// Consider renderer visible until this many frames have passed without rendering
#define VISIBLE_DURATION_FRAME_COUNT  1

CRPBaseRenderer::CRPBaseRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  m_context(context),
  m_bufferPool(std::move(bufferPool)),
  m_renderSettings(renderSettings),
  m_shadersNeedUpdate(true),
  m_bUseShaderPreset(false)
{
  m_oldDestRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);

  for(int i=0; i < 4; i++)
  {
    m_rotatedDestCoords[i].x = 0;
    m_rotatedDestCoords[i].y = 0;
  }

  m_bufferPool->RegisterRenderer(this);
}

CRPBaseRenderer::~CRPBaseRenderer()
{
  SetBuffer(nullptr);

  m_bufferPool->UnregisterRenderer(this);
}

bool CRPBaseRenderer::IsCompatible(const CRenderVideoSettings &settings) const
{
  if (!m_bufferPool->IsCompatible(settings))
    return false;

  // Shader preset must match
  std::string shaderPreset;
  if (m_shaderPreset)
    shaderPreset = m_shaderPreset->GetShaderPreset();

  if (settings.GetShaderPreset() != shaderPreset)
    return false;

  return true;
}

bool CRPBaseRenderer::Configure(AVPixelFormat format, unsigned int width, unsigned int height)
{
  m_format = format;
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = 0; //! @todo

  if (!m_bufferPool->IsConfigured())
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Configuring buffer pool");

    if (!m_bufferPool->Configure(format, width, height))
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to configure buffer pool");
      return false;
    }
  }

  ManageRenderArea();

  if (ConfigureInternal())
    m_bConfigured = true;

  return m_bConfigured;
}

void CRPBaseRenderer::FrameMove()
{
  m_renderFrameCount++;
}

bool CRPBaseRenderer::IsVisible() const
{
  if (m_renderFrameCount <= m_lastRender + VISIBLE_DURATION_FRAME_COUNT)
    return true;

  return false;
}

void CRPBaseRenderer::SetBuffer(IRenderBuffer *buffer)
{
  if (m_renderBuffer != buffer)
  {
    if (m_renderBuffer != nullptr)
      m_renderBuffer->Release();

    m_renderBuffer = buffer;

    if (m_renderBuffer != nullptr)
      m_renderBuffer->Acquire();
  }
}

void CRPBaseRenderer::RenderFrame(bool clear, uint8_t alpha)
{
  m_lastRender = m_renderFrameCount;

  if (!m_bConfigured || m_renderBuffer == nullptr)
    return;

  RenderInternal(clear, alpha);
  PostRender();

  m_renderBuffer->SetRendered(true);
}

void CRPBaseRenderer::Flush()
{
  SetBuffer(nullptr);
  FlushInternal();
}

void CRPBaseRenderer::SetSpeed(double speed)
{
  if (m_shaderPreset)
    m_shaderPreset->SetSpeed(speed);
}

float CRPBaseRenderer::GetAspectRatio() const
{
  return static_cast<float>(m_sourceWidth) / static_cast<float>(m_sourceHeight);
}

void CRPBaseRenderer::SetShaderPreset(const std::string &presetPath)
{
  if (presetPath != m_renderSettings.VideoSettings().GetShaderPreset())
  {
    m_renderSettings.VideoSettings().SetShaderPreset(presetPath);
    m_shadersNeedUpdate = true;
  }
}

void CRPBaseRenderer::SetScalingMethod(SCALINGMETHOD method)
{
  m_renderSettings.VideoSettings().SetScalingMethod(method);
}

void CRPBaseRenderer::SetViewMode(VIEWMODE viewMode)
{
  m_renderSettings.VideoSettings().SetRenderViewMode(viewMode);

  float screenWidth;
  float screenHeight;
  GetScreenDimensions(screenWidth, screenHeight);
  CalculateViewMode(viewMode, m_sourceWidth, m_sourceHeight, screenWidth, screenHeight, m_pixelRatio, m_zoomAmount);
}

void CRPBaseRenderer::SetRenderRotation(unsigned int rotationDegCCW)
{
  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CRPBaseRenderer::GetScreenDimensions(float &screenWidth, float &screenHeight)
{
  // Get our calibrated full screen resolution
  RESOLUTION_INFO info = m_context.GetResInfo();

  screenWidth = static_cast<float>(info.Overscan.right - info.Overscan.left);
  screenHeight = static_cast<float>(info.Overscan.bottom - info.Overscan.top);

  // Splitres scaling factor
  float xscale = static_cast<float>(info.iScreenWidth) / static_cast<float>(info.iWidth);
  float yscale = static_cast<float>(info.iScreenHeight) / static_cast<float>(info.iHeight);

  screenWidth *= xscale;
  screenHeight *= yscale;
}

void CRPBaseRenderer::CalculateViewMode(VIEWMODE viewMode, unsigned int sourceWidth, unsigned int sourceHeight, float screenWidth, float screenHeight, float &pixelRatio, float &zoomAmount)
{
  const float sourceFrameRatio = static_cast<float>(sourceWidth) / static_cast<float>(sourceHeight);

  switch (viewMode)
  {
  case VIEWMODE::Stretch4x3:
  {
    // Stretch image to 4:3 ratio
    zoomAmount = 1.0f;

    // Now we need to set pixelRatio so that fOutputFrameRatio = 4:3
    pixelRatio = (4.0f / 3.0f) / sourceFrameRatio;

    break;
  }
  case VIEWMODE::Stretch16x9:
  {
    // Stretch image to 16:9 ratio
    zoomAmount = 1.0f;

    // Stretch to the limits of the 16:9 screen
    pixelRatio = (screenWidth / screenHeight) / sourceFrameRatio;

    break;
  }
  case VIEWMODE::Original:
  {
    // Zoom image so that the height is the original size
    pixelRatio = 1.0f;

    // Get the size of the media file
    // Calculate the desired output ratio
    float outputFrameRatio = sourceFrameRatio * pixelRatio;

    // Now calculate the correct zoom amount.  First zoom to full width.
    float newHeight = screenWidth / outputFrameRatio;
    if (newHeight > screenHeight)
    {
      // Zoom to full height
      newHeight = screenHeight;
    }

    // Now work out the zoom amount so that no zoom is done
    zoomAmount = sourceHeight / newHeight;

    break;
  }
  case VIEWMODE::Normal:
  {
    pixelRatio = 1.0f;
    zoomAmount = 1.0f;
    break;
  }
  default:
    break;
  }
}

std::array<CPoint, 4> CRPBaseRenderer::ReorderDrawPoints(const CRect &destRect, const CRect &viewRect, unsigned int orientationDegCCW, float aspectRatio)
{
  std::array<CPoint, 4> rotatedDestCoords =
  {{
    CPoint{ destRect.x1, destRect.y1 }, // Top left
    CPoint{ destRect.x2, destRect.y1 }, // Top right
    CPoint{ destRect.x2, destRect.y2 }, // Bottom right
    CPoint{ destRect.x1, destRect.y2 }, // Bottom left
  }};

  switch (orientationDegCCW)
  {
  case 180:
  {
    std::swap(rotatedDestCoords[0], rotatedDestCoords[2]);
    std::swap(rotatedDestCoords[1], rotatedDestCoords[3]);
    break;
  }
  case 90:
  case 270:
  {
    const float oldWidth = destRect.Width();
    const float oldHeight = destRect.Height();

    // New width is old height, new height is old width
    float newWidth = oldHeight;
    float newHeight = oldWidth;

    const float diffWidth = newWidth - oldWidth;
    const float diffHeight = newHeight - oldHeight;

    // If the new width or new height is bigger than the old, we need to
    // scale down
    if (diffWidth > 0.0f || diffHeight > 0.0f)
    {
      // Scale to fit screen width because the difference in width is bigger
      // then the difference in height
      if (diffWidth > diffHeight)
      {
        // Clamp to the width of the old dest rect
        newWidth = oldWidth;
        newHeight *= aspectRatio;
      }
      else // Scale to fit screen height
      {
        // Clamp to the height of the old dest rect
        newHeight = oldHeight;
        newWidth /= aspectRatio;
      }
    }

    // Calculate the center point of the view
    const float centerX = viewRect.x1 + viewRect.Width() / 2.0f;
    const float centerY = viewRect.y1 + viewRect.Height() / 2.0f;

    // Calculate the number of pixels we need to go in each x direction from
    // the center point
    const float diffX = newWidth / 2;
    // Calculate the number of pixels we need to go in each y direction from
    // the center point
    const float diffY = newHeight / 2;

    unsigned int pointOffset;
    switch (orientationDegCCW)
    {
    case 90:
      pointOffset = 3;
      break;
    case 270:
      pointOffset = 1;
      break;
    default:
      break;
    }

    for (unsigned int i = 0; i < rotatedDestCoords.size(); i++)
    {
      switch ((i + pointOffset) % 4)
      {
      case 0:// top left
        rotatedDestCoords[i].x = centerX - diffX;
        rotatedDestCoords[i].y = centerY - diffY;
        break;
      case 1:// top right
        rotatedDestCoords[i].x = centerX + diffX;
        rotatedDestCoords[i].y = centerY - diffY;
        break;
      case 2:// bottom right
        rotatedDestCoords[i].x = centerX + diffX;
        rotatedDestCoords[i].y = centerY + diffY;
        break;
      case 3:// bottom left
        rotatedDestCoords[i].x = centerX - diffX;
        rotatedDestCoords[i].y = centerY + diffY;
        break;
      default:
        break;
      }
    }

    break;
  }
  default:
    break;
  }

  return rotatedDestCoords;
}

void CRPBaseRenderer::CalcNormalRenderRect(const CRect &viewRect, float inputFrameRatio, float zoomAmount, float pixelRatio, CRect &sourceRect, CRect &destRect)
{
  const float offsetX = viewRect.x1;
  const float offsetY = viewRect.y1;
  const float width = viewRect.Width();
  const float height = viewRect.Height();

  // If view window is empty, set empty destination
  if (height == 0 || width == 0)
  {
    destRect.SetRect(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  // Scale up image as much as possible and keep the aspect ratio (introduces
  // with black bars)
  // Calculate the correct output frame ratio (using the users pixel ratio
  // setting and the output pixel ratio setting)
  float outputFrameRatio = inputFrameRatio / pixelRatio;

  // Allow a certain error to maximize size of render area
  float fCorrection = width / height / outputFrameRatio - 1.0f;
  float fAllowed = GetAllowedErrorInAspect();

  if (fCorrection > fAllowed)
    fCorrection = fAllowed;

  if (fCorrection < -fAllowed)
    fCorrection = -fAllowed;

  outputFrameRatio *= 1.0f + fCorrection;

  // Maximize the game width
  float newWidth = width;
  float newHeight = newWidth / outputFrameRatio;

  if (newHeight > height)
  {
    newHeight = height;
    newWidth = newHeight * outputFrameRatio;
  }

  // Scale the game up by set zoom amount
  newWidth *= zoomAmount;
  newHeight *= zoomAmount;

  // If we are less than one pixel off use the complete screen instead
  if (std::abs(newWidth - width) < 1.0f)
    newWidth = width;
  if (std::abs(newHeight - height) < 1.0f)
    newHeight = height;

  // Center the game
  float posY = (height - newHeight) / 2;
  float posX = (width - newWidth) / 2;

  destRect.x1 = static_cast<float>(MathUtils::round_int(posX + offsetX));
  destRect.x2 = destRect.x1 + MathUtils::round_int(newWidth);
  destRect.y1 = static_cast<float>(MathUtils::round_int(posY + offsetY));
  destRect.y2 = destRect.y1 + MathUtils::round_int(newHeight);
}

void CRPBaseRenderer::ManageRenderArea()
{
  // Entire target rendering area for the video (including black bars)
  CRect viewRect = m_context.GetViewWindow();

  VIEWMODE viewMode = m_renderSettings.VideoSettings().GetRenderViewMode();
  m_renderOrientation = m_renderSettings.VideoSettings().GetRenderRotation();

  m_sourceRect.x1 = 0.0f;
  m_sourceRect.y1 = 0.0f;
  m_sourceRect.x2 = static_cast<float>(m_sourceWidth);
  m_sourceRect.y2 = static_cast<float>(m_sourceHeight);

  CalcNormalRenderRect(viewRect, GetAspectRatio() * m_pixelRatio, m_zoomAmount, m_context.GetResInfo().fPixelRatio, m_sourceRect, m_renderSettings.Geometry().Dimensions());

  // Clip as needed
  if (!(m_context.IsFullScreenVideo() || m_context.IsCalibrating()))
    ClipRect(viewRect, m_sourceRect, m_renderSettings.Geometry().Dimensions());

  const CRect &destRect = m_renderSettings.Geometry().Dimensions();
  if (m_oldDestRect != destRect || m_oldRenderOrientation != m_renderOrientation)
  {
    // Adapt the drawing rect points if we have to rotate and either destrect
    // or orientation changed
    m_rotatedDestCoords = ReorderDrawPoints(destRect, viewRect, m_renderOrientation, GetAspectRatio());
    m_oldDestRect = destRect;
    m_oldRenderOrientation = m_renderOrientation;
  }

  float screenWidth;
  float screenHeight;
  GetScreenDimensions(screenWidth, screenHeight);
  CalculateViewMode(viewMode, m_sourceWidth, m_sourceHeight, screenWidth, screenHeight, m_pixelRatio, m_zoomAmount);
}

void CRPBaseRenderer::ClipRect(const CRect &viewRect, CRect &sourceRect, CRect &destRect)
{
  const float offsetX = viewRect.x1;
  const float offsetY = viewRect.y1;
  const float width = viewRect.Width();
  const float height = viewRect.Height();

  CRect original(destRect);
  destRect.Intersect(CRect(offsetX, offsetY, offsetX + width, offsetY + height));
  if (destRect != original)
  {
    float scaleX = sourceRect.Width() / original.Width();
    float scaleY = sourceRect.Height() / original.Height();
    sourceRect.x1 += (destRect.x1 - original.x1) * scaleX;
    sourceRect.y1 += (destRect.y1 - original.y1) * scaleY;
    sourceRect.x2 += (destRect.x2 - original.x2) * scaleX;
    sourceRect.y2 += (destRect.y2 - original.y2) * scaleY;
  }
}

void CRPBaseRenderer::MarkDirty()
{
  //CServiceBroker::GetGUI()->GetWindowManager().MarkDirty(m_renderSettings.Geometry().Dimensions()); //! @todo
}

float CRPBaseRenderer::GetAllowedErrorInAspect()
{
  return CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT) * 0.01f;
}

void CRPBaseRenderer::UpdateVideoShaders()
{
  if (m_shadersNeedUpdate && !m_renderSettings.VideoSettings().GetShaderPreset().empty())
  {
    m_shadersNeedUpdate = false;

    if (m_shaderPreset)
    {
      auto sourceWidth = static_cast<unsigned>(m_sourceRect.Width());
      auto sourceHeight = static_cast<unsigned>(m_sourceRect.Height());

      // We need to set this here because m_sourceRect isn't valid on init/pre-init
      m_shaderPreset->SetVideoSize(sourceWidth, sourceHeight);
      m_bUseShaderPreset = m_shaderPreset->SetShaderPreset(m_renderSettings.VideoSettings().GetShaderPreset());
    }
  }
}

void CRPBaseRenderer::PreRender(bool clear)
{
  if (!m_bConfigured)
    return;

  // Clear screen
  if (clear)
    m_context.Clear(m_context.UseLimitedColor() ? 0x101010 : 0);

  ManageRenderArea();

  UpdateVideoShaders();
}

void CRPBaseRenderer::PostRender()
{
  m_context.ApplyStateBlock();
}
