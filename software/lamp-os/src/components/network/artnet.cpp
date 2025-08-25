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

namespace lamp {
const char ArtnetWifi::artnetId[] = ART_NET_ID;

ArtnetWifi::ArtnetWifi() : artDmxCallback(nullptr) {}

void ArtnetWifi::begin() {
  udp.listen(ART_NET_PORT);
  udp.onPacket([&](AsyncUDPPacket packet) {
    packetSize = packet.length();

    if (packetSize == MAX_BUFFER_ARTNET) {
      uint8_t* artnetPacket = packet.data();

      opcode = artnetPacket[8] | artnetPacket[9] << 8;

      if (opcode == ART_DMX) {
        incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
        sequence = artnetPacket[12];
        dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

        if (artDmxCallback)
          (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence,
                            artnetPacket + ART_DMX_START);
      }
    }
  });
}
}  // namespace lamp
