#pragma once

/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include <list>
#include <memory>

#include "DVDAudioCodec.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"

class CProcessInfo;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType);
  ~CDVDAudioCodecPassthrough() override;

  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  void Dispose() override;
  bool AddData(const DemuxPacket &packet) override;
  void GetData(DVDAudioFrame &frame) override;
  int GetData(uint8_t** dst) override;
  void Reset() override;
  AEAudioFormat GetFormat() override { return m_format; }
  bool NeedPassthrough() override { return true; }
  const char* GetName() override { return "passthrough"; }
  int GetBufferSize() override;

private:
  CAEStreamParser m_parser;
  uint8_t* m_buffer = nullptr;
  unsigned int m_bufferSize = 0;
  unsigned int m_dataSize = 0;
  AEAudioFormat m_format;
  uint8_t *m_backlogBuffer = nullptr;
  unsigned int m_backlogBufferSize = 0;
  unsigned int m_backlogSize = 0;
  double m_currentPts = DVD_NOPTS_VALUE;
  double m_nextPts = DVD_NOPTS_VALUE;

  // TrueHD specifics
  std::unique_ptr<uint8_t[]> m_trueHDBuffer;
  unsigned int m_trueHDoffset = 0;
};

