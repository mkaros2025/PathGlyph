#pragma once
#include <glm/glm.hpp>
#include <unordered_map>

namespace PathGlyph {

// 前向声明
class Renderer;
class Maze;

enum class InputAction {
  MouseScrollUp,
  MouseScrollDown,
  MouseLeftClick,
  MouseRightClick,
  ToggleEditMode,
  ConfirmEdit,
  RecalculatePath,
  Exit
};

enum class EditMode {
  Browsing,
  SettingStart,
  SettingGoal,
  AddingWall,
  RemovingWall
};

class InputSystem {
 public:
  InputSystem(Renderer* renderer, Maze* maze);

  // 输入处理接口
  void handleInput(int key, int action);
  void handleMouseInput(int button, int action, const glm::vec2& cursorPos);
  void handleMouseScroll(float yoffset);
  void handleCursorPos(const glm::vec2& cursorPos, const glm::vec2& windowSize);

  // 状态查询
  EditMode getCurrentMode() const { return currentMode_; }
  bool hasValidEdit() const;
  std::string getModeDescription() const;

 private:
  Renderer* renderer_;
  Maze* maze_;
  EditMode currentMode_;
  bool isMouseInBorderArea_;
  bool startSet_;
  bool goalSet_;
  glm::ivec2 startPos_;
  glm::ivec2 goalPos_;

  std::unordered_map<int, InputAction> keyBindings_;

  // 内部实现方法
  void initializeBindings();
  void executeAction(InputAction action,
                     const glm::vec2& cursorPos = glm::vec2(0));
  void toggleEditMode();
  void handleEditAction(const glm::vec2& cursorPos);
  void executePanAction(int dx, int dy);
  void updateBorderPan(const glm::vec2& cursorPos, const glm::vec2& windowSize);
  bool canExitEditMode() const;
};

}  // namespace PathGlyph
