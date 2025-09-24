#include "./expression_manager.hpp"
#include <Arduino.h>
#include <algorithm>

namespace lamp {

// Define the global frame buffer vector for expressions
std::vector<FrameBuffer*> expressionFrameBuffers;

// Global expression manager pointer
static ExpressionManager* globalExpressionManager = nullptr;

void setGlobalExpressionManager(ExpressionManager* manager) {
  globalExpressionManager = manager;
}

ExpressionManager* getGlobalExpressionManager() {
  return globalExpressionManager;
}

void ExpressionManager::begin(FrameBuffer* shade, FrameBuffer* base) {
  shadeBuffer = shade;
  baseBuffer = base;

  // Set up global frame buffer references
  expressionFrameBuffers.clear();
  expressionFrameBuffers.push_back(shade);
  expressionFrameBuffers.push_back(base);
}

void ExpressionManager::loadFromConfig(const ExpressionSettings& settings) {
  clear();

  for (const auto& config : settings.expressions) {
    if (config.enabled) {
      addExpression(config);
    }
  }
}


void ExpressionManager::addExpression(const ExpressionConfig& config) {
  if (!shadeBuffer || !baseBuffer) return;
#ifdef LAMP_DEBUG
  Serial.printf("ExpressionManager::addExpression - type: %s\n", config.type.c_str());
#endif

  auto target = static_cast<ExpressionTarget>(config.target);

  // Lambda to create appropriate expression type
  auto createExpression = [&](FrameBuffer* buffer) -> std::unique_ptr<Expression> {
    std::unique_ptr<Expression> expr;

    if (config.type == "glitchy") {
      auto glitchyExpr = std::make_unique<GlitchyExpression>(buffer, 3);
      glitchyExpr->configure(config.colors, config.intervalMin, config.intervalMax, target);
      glitchyExpr->configureFromParameters(config.parameters);
      expr = std::move(glitchyExpr);
    } else if (config.type == "shifty") {
#ifdef LAMP_DEBUG
      Serial.printf("Creating ShiftyExpression with generic parameters\n");
#endif
      auto shiftyExpr = std::make_unique<ShiftyExpression>(buffer, 120);
      shiftyExpr->configure(config.colors, config.intervalMin, config.intervalMax, target);
      shiftyExpr->configureFromParameters(config.parameters);
      expr = std::move(shiftyExpr);
    } else if (config.type == "pulse") {
      auto pulseExpr = std::make_unique<PulseExpression>(buffer, 60);
      pulseExpr->configure(config.colors, config.intervalMin, config.intervalMax, target);
      pulseExpr->configureFromParameters(config.parameters);
      expr = std::move(pulseExpr);
    }

    return expr;
  };

  // Determine target buffers
  std::vector<FrameBuffer*> targetBuffers;
  if (target == TARGET_BOTH) {
    targetBuffers = {shadeBuffer, baseBuffer};
  } else {
    targetBuffers = {(target == TARGET_SHADE) ? shadeBuffer : baseBuffer};
  }

  // Create expressions for each target buffer
  for (auto* buffer : targetBuffers) {
    if (auto expr = createExpression(buffer)) {
      expressions.push_back({std::move(expr), config.type});
    }
  }
}

std::vector<AnimatedBehavior*> ExpressionManager::getBehaviors() {
  std::vector<AnimatedBehavior*> behaviors;
  for (auto& entry : expressions) {
    behaviors.push_back(entry.expression.get());
  }
  return behaviors;
}

void ExpressionManager::removeExpression(size_t index) {
  if (index < expressions.size()) {
    expressions.erase(expressions.begin() + index);
  }
}

void ExpressionManager::clear() {
  expressions.clear();
}

bool ExpressionManager::triggerExpression(const std::string& type) {
#ifdef LAMP_DEBUG
  Serial.printf("ExpressionManager::triggerExpression() - looking for type: %s\n", type.c_str());
  Serial.printf("ExpressionManager::triggerExpression() - total expressions: %zu\n", expressions.size());
#endif

  bool triggered = false;
  int matchCount = 0;

  for (auto& entry : expressions) {
#ifdef LAMP_DEBUG
    Serial.printf("ExpressionManager::triggerExpression() - checking expression: %s\n", entry.type.c_str());
#endif
    if (entry.type == type && entry.expression) {
      matchCount++;
#ifdef LAMP_DEBUG
      Serial.printf("ExpressionManager::triggerExpression() - triggering match #%d: %s\n", matchCount, entry.type.c_str());
#endif
      entry.expression->trigger();
      triggered = true;  // Don't return here, continue to trigger all matches
    }
  }

#ifdef LAMP_DEBUG
  if (triggered) {
    Serial.printf("ExpressionManager::triggerExpression() - successfully triggered %d expressions of type: %s\n", matchCount, type.c_str());
  } else {
    Serial.printf("ExpressionManager::triggerExpression() - no expressions found for type: %s\n", type.c_str());
  }
#endif

  return triggered;
}

}  // namespace lamp