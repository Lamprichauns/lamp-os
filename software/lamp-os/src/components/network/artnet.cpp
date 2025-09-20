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
 * Converted to Async UDP using the same basic algorithm
 */
#include "./artnet.hpp"

#include <AsyncUDP.h>

#include <cstdint>

#include "../../util/color.hpp"

namespace lamp {
ArtnetDetail::ArtnetDetail() {};

ArtnetDetail::ArtnetDetail(Color inShadeColor, Color inBaseColor, uint8_t inMode, uint8_t inParameter) {
  shadeColor = inShadeColor,
  baseColor = inBaseColor,
  mode = inMode;
  parameter = inParameter;
};

const char ArtnetWifi::artnetId[] = ART_NET_ID;

ArtnetWifi::ArtnetWifi() {};

void ArtnetWifi::begin() {
  udp.listen(ART_NET_PORT);
  lampNumber = random(0, 7);
#ifdef LAMP_DEBUG
  Serial.printf("Artnet index: %d\n", lampNumber);
#endif
  udp.onPacket([&](AsyncUDPPacket packet) {
    packetSize = packet.length();

    if (packetSize == MAX_BUFFER_ARTNET) {
      uint8_t* artnetPacket = packet.data();

      opcode = artnetPacket[8] | artnetPacket[9] << 8;

      if (opcode == ART_DMX) {
        incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
        sequence = artnetPacket[12];
        dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;
        data = artnetPacket + ART_DMX_START;

        if (incomingUniverse == 1) {
          lastDmxFrameMs = millis();

          /**
           * Artnet lds should be sending 8 lamp fixtures @ 10ch each
           * the lamps will have picked one based on their colors
           */
          int index = lampNumber * 10;
          artnetData = ArtnetDetail(
              Color(data[index + 0], data[index + 1], data[index + 2], data[index + 3]),  // shade color
              Color(data[index + 4], data[index + 5], data[index + 6], data[index + 7]),  // base color
              data[index + 8],                                                            // mode
              data[index + 9]);                                                           // parameter
          seq = sequence;
        }
      }
    }
  });
};
}  // namespace lamp
