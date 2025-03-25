#include "input_system.h"

#include <GLFW/glfw3.h>

#include <string>

#include "maze.h"
#include "renderer.h"

namespace PathGlyph {

InputSystem::InputSystem(Renderer *renderer, Maze *maze)
    : renderer_(renderer),
      maze_(maze),
      currentMode_(EditMode::Browsing),
      isMouseInBorderArea_(false),
      startSet_(false),
      goalSet_(false) {
  initializeBindings();
}

void InputSystem::initializeBindings() {
  keyBindings_[GLFW_KEY_E] = InputAction::ToggleEditMode;
  keyBindings_[GLFW_KEY_ENTER] = InputAction::ConfirmEdit;
  keyBindings_[GLFW_KEY_P] = InputAction::RecalculatePath;
  keyBindings_[GLFW_KEY_ESCAPE] = InputAction::Exit;
}

// 处理键盘输入
void InputSystem::handleInput(int key, int action) {
  // 如果是按键按下事件
  if (action == GLFW_PRESS) {
    // 在键盘映射中找到这个键
    auto it = keyBindings_.find(key);
    if (it != keyBindings_.end()) {
      // 执行对应动作
      executeAction(it->second);
    }
  }
}

// 处理鼠标输入
void InputSystem::handleMouseInput(int button, int action,
                                   const glm::vec2 &cursorPos) {
  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      executeAction(InputAction::MouseLeftClick, cursorPos);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      executeAction(InputAction::MouseRightClick, cursorPos);
    }
  }
}

void InputSystem::handleMouseScroll(float yoffset) {
  executeAction(yoffset > 0 ? InputAction::MouseScrollUp
                            : InputAction::MouseScrollDown);
}

void InputSystem::handleCursorPos(const glm::vec2 &cursorPos,
                                  const glm::vec2 &windowSize) {
  updateBorderPan(cursorPos, windowSize);
}

void InputSystem::executeAction(InputAction action,
                                const glm::vec2 &cursorPos) {
  switch (action) {
    case InputAction::ToggleEditMode:
      toggleEditMode();
      break;

    case InputAction::ConfirmEdit:
      if (currentMode_ != EditMode::Browsing && canExitEditMode()) {
        currentMode_ = EditMode::Browsing;
      }
      break;

    case InputAction::MouseLeftClick:
      handleEditAction(cursorPos);
      break;

    case InputAction::MouseRightClick:
      if (currentMode_ != EditMode::Browsing) {
        currentMode_ = EditMode::Browsing;
      }
      break;

    case InputAction::RecalculatePath:
      if (hasValidEdit()) {
        maze_->calculatePath(startPos_, goalPos_);
      }
      break;

    case InputAction::MouseScrollUp:
      renderer_->zoomCamera(1.1f);
      break;

    case InputAction::MouseScrollDown:
      renderer_->zoomCamera(0.9f);
      break;

    case InputAction::Exit:
      renderer_->requestClose();
      break;
  }
}

void InputSystem::toggleEditMode() {
  if (currentMode_ == EditMode::Browsing) {
    currentMode_ = EditMode::SettingStart;
    startSet_ = false;
    goalSet_ = false;
  } else if (canExitEditMode()) {
    currentMode_ = EditMode::Browsing;
  }
}

void InputSystem::handleEditAction(const glm::vec2 &cursorPos) {
  glm::ivec2 gridPos = renderer_->screenToGrid(cursorPos);

  if (!maze_->isPositionValid(gridPos)) return;

  switch (currentMode_) {
    case EditMode::SettingStart:
      if (maze_->isWalkable(gridPos)) {
        startPos_ = gridPos;
        startSet_ = true;
        maze_->setStart(gridPos);
        currentMode_ = EditMode::SettingGoal;
      }
      break;

    case EditMode::SettingGoal:
      if (maze_->isWalkable(gridPos) && gridPos != startPos_) {
        goalPos_ = gridPos;
        goalSet_ = true;
        maze_->setGoal(gridPos);
        currentMode_ = EditMode::AddingWall;
      }
      break;

    case EditMode::AddingWall:
      if (gridPos != startPos_ && gridPos != goalPos_) {
        maze_->toggleWall(gridPos, true);
      }
      break;

    case EditMode::RemovingWall:
      maze_->toggleWall(gridPos, false);
      break;

    default:
      break;
  }
}

void InputSystem::updateBorderPan(const glm::vec2 &cursorPos,
                                  const glm::vec2 &windowSize) {
  const float borderThreshold = 0.05f;
  const float maxSpeed = 30.0f;
  glm::vec2 normalizedPos = cursorPos / windowSize;

  glm::vec2 speed(0.0f);

  if (currentMode_ == EditMode::Browsing) {
    if (normalizedPos.x < borderThreshold)
      speed.x = -maxSpeed;
    else if (normalizedPos.x > 1.0f - borderThreshold)
      speed.x = maxSpeed;

    if (normalizedPos.y < borderThreshold)
      speed.y = maxSpeed;
    else if (normalizedPos.y > 1.0f - borderThreshold)
      speed.y = -maxSpeed;

    if (speed.x != 0.0f || speed.y != 0.0f) {
      renderer_->panCamera(speed.x * 0.016f,
                           speed.y * 0.016f);  // 60fps frame time
    }
  }
}

bool InputSystem::hasValidEdit() const { return startSet_ && goalSet_; }

bool InputSystem::canExitEditMode() const {
  return hasValidEdit() && maze_->isPositionValid(startPos_) &&
         maze_->isPositionValid(goalPos_) && maze_->isWalkable(startPos_) &&
         maze_->isWalkable(goalPos_);
}

std::string InputSystem::getModeDescription() const {
  switch (currentMode_) {
    case EditMode::Browsing:
      return "Navigation Mode [Press E to edit]";
    case EditMode::SettingStart:
      return "Setting Start Point - Click valid tile";
    case EditMode::SettingGoal:
      return "Setting Goal Point - Click different tile";
    case EditMode::AddingWall:
      return "Wall Editing - LMB:Add  RMB:Cancel";
    case EditMode::RemovingWall:
      return "Wall Removing - LMB:Remove  RMB:Cancel";
    default:
      return "";
  }
}

}  // namespace PathGlyph
