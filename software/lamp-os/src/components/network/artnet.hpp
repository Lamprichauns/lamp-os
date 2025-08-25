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

#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>

#include <functional>

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

#define DMX_FUNC_PARAM \
  uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data
#if !defined(ARDUINO_AVR_UNO_WIFI_REV2)
typedef std::function<void(DMX_FUNC_PARAM)> StdFuncDmx_t;
#endif
namespace lamp {
class ArtnetWifi {
 public:
  ArtnetWifi();
  void begin();
  inline void setArtDmxCallback(void (*fptr)(uint16_t universe, uint16_t length,
                                             uint8_t sequence, uint8_t* data)) {
    artDmxCallback = fptr;
  }

 private:
  AsyncUDP udp;
  String host;
  uint8_t artnetPacket[MAX_BUFFER_ARTNET];
  uint16_t packetSize;
  uint16_t opcode;
  uint8_t sequence;
  uint16_t incomingUniverse;
  uint16_t dmxDataLength;
  void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence,
                         uint8_t* data);
  static const char artnetId[];
};
}  // namespace lamp
#endif
