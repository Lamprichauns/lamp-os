# Creating Custom Expressions

Expressions add personality to your lamp through time-triggered visual behaviors that create ambient effects and character.

## Architecture Overview

Expressions inherit from the `Expression` base class, which provides:
- Random scheduling within configured intervals
- Target buffer management (shade/base/both)
- Color palette support
- Frame buffer state saving/restoration
- Integration with the animation system

## Creating a New Expression

### 1. Define Your Expression Class

Create a header file in `src/expressions/`:

```cpp
// src/expressions/my_expression.hpp
#ifndef LAMP_EXPRESSIONS_MY_EXPRESSION_H
#define LAMP_EXPRESSIONS_MY_EXPRESSION_H

#include "./expression.hpp"

namespace lamp {

class MyExpression : public Expression {
 private:
  // Add custom state variables
  uint32_t myCustomParam;

  // State management if needed
  enum MyState {
    STATE_IDLE,
    STATE_ACTIVE
  };
  MyState currentState = STATE_IDLE;

 public:
  using Expression::Expression;

  // Override control() for state management
  void control() override;

  // Override draw() for visual rendering
  void draw() override;

  // Add custom configuration if needed
  void configure(const std::vector<Color>& inColors,
                 uint32_t inIntervalMin,
                 uint32_t inIntervalMax,
                 ExpressionTarget inTarget,
                 uint32_t customParam);
};

}  // namespace lamp
#endif
```

### 2. Implement Behavior Logic

Create the implementation file:

```cpp
// src/expressions/my_expression.cpp
#include "./my_expression.hpp"

namespace lamp {

void MyExpression::configureFromParameters(const std::map<std::string, std::variant<uint32_t, float, double>>& parameters) {
  // Extract custom parameters using helper methods
  myCustomParam = extractUint32Parameter(parameters, "myCustomParam", 100);

  // Configure expression-specific settings
  frames = 60;  // Animation duration
}

void MyExpression::control() {
  // Check if it's time to trigger
  if (animationState == STOPPED && millis() > nextTriggerMs) {
    if (shouldAffectBuffer()) {
      saveBufferState();  // Save current colors for restoration
      scheduleNextTrigger();  // Schedule next activation

      // Set up your animation
      frames = 60;  // Animation duration in frames
      currentState = STATE_ACTIVE;
      playOnce();   // Start animation
    }
  }

  // Handle state transitions
  if (currentState == STATE_ACTIVE && animationState == STOPPED) {
    currentState = STATE_IDLE;
  }
}

void MyExpression::draw() {
  if (!shouldAffectBuffer()) {
    nextFrame();
    return;
  }

  // Implement your visual effect
  switch (currentState) {
    case STATE_ACTIVE: {
      // Access pixels via: fb->buffer[i]
      // Use savedBuffer for original colors
      // Use currentFrame/frames for animation progress
      float progress = static_cast<float>(currentFrame) / frames;

      for (int i = 0; i < fb->pixelCount; i++) {
        // Example: fade from saved color to expression color
        fb->buffer[i] = savedBuffer[i].lerp(getRandomColor(), progress);
      }
      break;
    }

    case STATE_IDLE:
    default:
      // Restore original buffer if needed
      break;
  }

  nextFrame();  // Advance animation frame
}

}  // namespace lamp
```

### 3. Register in ExpressionManager

Add your type to `src/expressions/expression_manager.cpp`:

```cpp
// Add include at top
#include "./my_expression.hpp"

// In addExpression() method, add your type:
else if (config.type == "my_expression") {
  // Handle TARGET_BOTH by creating instances for each buffer
  if (target == TARGET_BOTH) {
    // Shade instance
    auto shadeExpr = std::make_unique<MyExpression>(shadeBuffer, frames);
    shadeExpr->configure(config.colors, config.intervalMin, config.intervalMax,
                         target, config.myCustomParam);
    shadeExpr->allowedInHomeMode = true;
    expressions.push_back(std::move(shadeExpr));

    // Base instance
    auto baseExpr = std::make_unique<MyExpression>(baseBuffer, frames);
    baseExpr->configure(config.colors, config.intervalMin, config.intervalMax,
                        target, config.myCustomParam);
    baseExpr->allowedInHomeMode = true;
    expressions.push_back(std::move(baseExpr));
  } else {
    // Single buffer instance
    FrameBuffer* targetBuffer = (target == TARGET_SHADE) ? shadeBuffer : baseBuffer;
    auto expr = std::make_unique<MyExpression>(targetBuffer, frames);
    expr->configure(config.colors, config.intervalMin, config.intervalMax,
                    target, config.myCustomParam);
    expr->allowedInHomeMode = true;
    expressions.push_back(std::move(expr));
  }
}
```

### 4. Add Configuration Parameters

Update `src/config/config_types.hpp` if you need custom parameters:

```cpp
class ExpressionConfig {
 public:
  // ... existing fields ...

  // Add your custom parameters
  uint32_t myCustomParam = 100;  // Default value
};
```

### 5. Define UI Configuration

Add to `behavior_configs/expressions.json`:

```json
"my_expression": {
  "name": "My Expression",
  "description": "What your expression does",
  "config": {
    "target": {
      "type": "select",
      "options": ["shade", "base", "both"],
      "default": "both",
      "label": "Target"
    },
    "colors": {
      "type": "color_array",
      "min": 1,
      "max": 5,
      "default": ["#FF0000FF", "#00FF00FF"],
      "label": "Expression Colors"
    },
    "intervalMin": {
      "type": "range",
      "min": 60,
      "max": 1800,
      "default": 300,
      "step": 30,
      "unit": "s",
      "label": "Min Interval"
    },
    "intervalMax": {
      "type": "range",
      "min": 60,
      "max": 1800,
      "default": 600,
      "step": 30,
      "unit": "s",
      "label": "Max Interval"
    },
    "myCustomParam": {
      "type": "range",
      "min": 0,
      "max": 255,
      "default": 100,
      "step": 5,
      "label": "Custom Parameter"
    }
  }
}
```

## Reusable Patterns

### Interval-Based Triggering
```cpp
// In control():
if (animationState == STOPPED && millis() > nextTriggerMs) {
  if (shouldAffectBuffer()) {
    saveBufferState();
    scheduleNextTrigger();  // Automatically randomizes within interval
    // Start your effect...
  }
}
```

### State Management
```cpp
enum State { IDLE, TRANSITIONING, ACTIVE, RETURNING };
State state = IDLE;

void control() override {
  switch (state) {
    case IDLE:
      // Check for trigger
      break;
    case TRANSITIONING:
      // Monitor transition completion
      break;
    // etc...
  }
}
```

### Color Transitions
```cpp
// Smooth fade between colors
float progress = static_cast<float>(currentFrame) / frames;
for (int i = 0; i < fb->pixelCount; i++) {
  fb->buffer[i] = startColor.lerp(endColor, progress);
}
```

### Multi-Buffer Support
```cpp
// Always check before modifying
if (!shouldAffectBuffer()) {
  nextFrame();
  return;
}
// Your effect code here...
```

## Available UI Input Types

- **`range`**: Single slider for numeric values
- **`range_interval`**: Dual-handle slider for min/max ranges (future)
- **`color_array`**: Dynamic list of color pickers
- **`select`**: Dropdown menu for options
- **`checkbox`**: Boolean toggle
- **`text`**: Text input field

## Best Practices

1. **Always call `nextFrame()`** at the end of `draw()` to advance animation
2. **Check `shouldAffectBuffer()`** before modifying pixels
3. **Save buffer state** with `saveBufferState()` before modifications
4. **Use `scheduleNextTrigger()`** for random timing within intervals
5. **Set `allowedInHomeMode`** appropriately (true for ambient, false for attention-seeking)
6. **Keep state machines simple** - prefer clear states over complex conditions
7. **Reuse existing patterns** - look at glitchy and shifty for examples
8. **Test interaction** with other expressions running simultaneously

## Example Expressions

### Glitchy
Brief color flashes that add character. Key features:
- Very short duration (1-30 frames)
- Random color selection from palette
- Saves and restores original buffer

### Shifty
Smooth ambient color transitions. Key features:
- Long fade duration (15s - 10min)
- Multiple palette generation from base colors
- State machine for shift lifecycle
- Optional interaction with other expressions

## Testing Your Expression

1. Add test configuration to your lamp
2. Use web UI to enable and configure
3. Adjust intervals for faster testing (shorter min/max)
4. Verify it works with different targets (shade/base/both)
5. Check interaction with other expressions
6. Test home mode compatibility
7. Monitor performance with multiple instances

## Debugging Tips

- Use ESP32 serial output for state changes
- Add logging in control() to track triggers
- Verify frame counting with currentFrame/frames
- Check buffer modifications are applying correctly
- Test edge cases (empty colors, extreme intervals)

This modular architecture ensures expressions can be developed independently while playing nicely together in the lamp ecosystem.