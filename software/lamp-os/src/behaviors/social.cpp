#include "../components/network/bluetooth.hpp"
#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"
#include "../util/fade.hpp"

#define LAMP_TIME_BETWEEN_ACKNOWLEDGEMENT_MS 30000

/**
 * @brief social color exchange
 */
namespace lamp {
class SocialBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  // how many frames to ease when greeting and returning to the lamp's
  // personality- the total frame count must be a multiple of the ease frames
  uint32_t easeFrames = 120;
  uint32_t nextAcknowledgeTimeMs = 0;
  Color foundLampColor;

  std::vector<BluetoothRecord>* foundLamps;

  void draw() {
    for (int i = 0; i < fb->pixelCount; i++) {
      Color buf = fb->buffer[i];
      if (frame < easeFrames) {
        fb->buffer[i] =
            fade(fb->defaultColor, foundLampColor, easeFrames, frame);
      } else if (frame > (frames - easeFrames)) {
        fb->buffer[i] = fade(foundLampColor, fb->defaultColor, easeFrames,
                             frame % easeFrames);
      } else {
        fb->buffer[i] = foundLampColor;
      }
    }

    nextFrame();
  };

  void control() {
    if (animationState == STOPPED &&
        millis() > nextAcknowledgeTimeMs) {
      for (std::vector<BluetoothRecord>::reverse_iterator revIter =
               foundLamps->rbegin();
           revIter != foundLamps->rend(); ++revIter) {
        if (!revIter->acknowledged) {
#ifdef LAMP_DEBUG
          Serial.printf("Acknowledging %s\n", revIter->name.c_str());
#endif
          revIter->acknowledged = true;
          foundLampColor = revIter->baseColor;
          nextAcknowledgeTimeMs = millis() + LAMP_TIME_BETWEEN_ACKNOWLEDGEMENT_MS;
          playOnce();
          break;
        }
      }
    }
  };

  void updateFoundLamps(std::vector<BluetoothRecord>* inFoundLamps) {
    foundLamps = inFoundLamps;
  }
};
}  // namespace lamp