#pragma once

#include <cstdint>

#include "core/frame_buffer.hpp"
#include "expressions/expression.hpp"
#include "expressions/expression_schema.hpp"

namespace lamp {

// The instances one (type, target) builds: one per surface, shade first, or a
// single base-bound instance joining the shade `above` it when the type spans
// surfaces. `count` is 0 for an unknown target. A null descriptor plans per
// surface.
struct SurfacePlan {
  FrameBuffer* buffers[2] = {};
  uint8_t count = 0;
  FrameBuffer* above = nullptr;
};

inline SurfacePlan surfacePlan(const ExpressionDescriptor* d, ExpressionTarget target,
                               FrameBuffer* shade, FrameBuffer* base) {
  SurfacePlan plan;
  switch (target) {
    case TARGET_SHADE:
      plan.buffers[plan.count++] = shade;
      break;
    case TARGET_BASE:
      plan.buffers[plan.count++] = base;
      break;
    case TARGET_BOTH:
      if (d && d->spansSurfaces) {
        plan.buffers[plan.count++] = base;
        plan.above = shade;
      } else {
        plan.buffers[plan.count++] = shade;
        plan.buffers[plan.count++] = base;
      }
      break;
  }
  return plan;
}

}  // namespace lamp
