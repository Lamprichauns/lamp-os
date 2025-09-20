/*The MIT License (MIT)

Copyright (c) 2014 Nathanaël Lécaudé
https://github.com/natcl/Artnet,
http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811

Copyright (c) 2016,2019 Stephan Ruloff
https://github.com/rstephan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/**
 * Simplified to only read universe 1 to prevent overhead and use less footprint
 * Based on version 1.6.1
 */
#ifndef LAMP_COMPONENTS_NETWORK_ARTNET_H
#define LAMP_COMPONENTS_NETWORK_ARTNET_H

#include <AsyncUDP.h>
#include <WiFi.h>

#include <cstdint>
#include <functional>

#include "../../util/color.hpp"

// UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net"
#define ART_DMX_START 18

namespace lamp {
/**
 * @brief abstraction for 10 DMX channels to the lamp
 *        {shade r,g,b,w, base r,g,b,w, mode, parameter}
 *        the mode will enable lamp effects. 0 is artnet pass through
 *        the parameter will adjust a single. 0 is artnet pass thruough
 * @property shadeColor - an rgbw value to send to shade pixels
 * @property baseColor - and rgbw value to send to base pixels
 * @property mode - a lamp mode from 0-255
 * @property parameter - a mode parameter to help give more data to the lamp modes
 */
class ArtnetDetail {
 public:
  Color shadeColor;
  Color baseColor;
  uint8_t mode;
  uint8_t parameter;

  ArtnetDetail();

  ArtnetDetail(Color inShadeColor, Color inBaseColor, uint8_t inMode = 0, uint8_t inParameter = 0);
};

/**
 * @brief the artnet udp packet handler
 * @property artnetData the record for the target lamp
 * @property lastDmxFrameMs the last time a UDP frame event occurred
 * @property seq tracking Artnet DMX sequence numbers to show jitter
 * @property lampNumber a specific 10 channel fixture profile to use out of 80 possible channels
 */
class ArtnetWifi {
 public:
  ArtnetDetail artnetData = ArtnetDetail();
  uint32_t lastDmxFrameMs = 0;
  uint8_t seq = 0;
  uint8_t lampNumber = 0;

  ArtnetWifi();
  void begin();

 private:
  AsyncUDP udp;
  uint8_t artnetPacket[MAX_BUFFER_ARTNET];
  uint16_t packetSize = 0;
  uint16_t opcode = 0;
  uint8_t sequence = 0;
  uint16_t incomingUniverse = 0;
  uint16_t dmxDataLength = 0;
  uint8_t* data = {};
  static const char artnetId[];
};
}  // namespace lamp
#endif
