#ifndef IISPACE_GAME_IO_SDL_CONVERT_H
#define IISPACE_GAME_IO_SDL_CONVERT_H
#include "game/io/input.h"
#include <SDL_gamecontroller.h>
#include <optional>

namespace ii::io {

std::optional<controller::axis> convert_controller_axis(SDL_GameControllerAxis axis) {
  switch (axis) {
  case SDL_CONTROLLER_AXIS_LEFTX:
    return controller::axis::kLX;
  case SDL_CONTROLLER_AXIS_LEFTY:
    return controller::axis::kLY;
  case SDL_CONTROLLER_AXIS_RIGHTX:
    return controller::axis::kRX;
  case SDL_CONTROLLER_AXIS_RIGHTY:
    return controller::axis::kRY;
  case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
    return controller::axis::kLT;
  case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
    return controller::axis::kRT;
  default:
    return std::nullopt;
  }
}

std::optional<controller::button> convert_controller_button(SDL_GameControllerButton button) {
  switch (button) {
  case SDL_CONTROLLER_BUTTON_A:
    return controller::button::kA;
  case SDL_CONTROLLER_BUTTON_B:
    return controller::button::kB;
  case SDL_CONTROLLER_BUTTON_X:
    return controller::button::kX;
  case SDL_CONTROLLER_BUTTON_Y:
    return controller::button::kY;
  case SDL_CONTROLLER_BUTTON_BACK:
    return controller::button::kBack;
  case SDL_CONTROLLER_BUTTON_GUIDE:
    return controller::button::kGuide;
  case SDL_CONTROLLER_BUTTON_START:
    return controller::button::kStart;
  case SDL_CONTROLLER_BUTTON_LEFTSTICK:
    return controller::button::kLStick;
  case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
    return controller::button::kRStick;
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
    return controller::button::kLShoulder;
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
    return controller::button::kRShoulder;
  case SDL_CONTROLLER_BUTTON_DPAD_UP:
    return controller::button::kDpadUp;
  case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
    return controller::button::kDpadDown;
  case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
    return controller::button::kDpadLeft;
  case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
    return controller::button::kDpadRight;
  case SDL_CONTROLLER_BUTTON_MISC1:
    return controller::button::kMisc1;
  case SDL_CONTROLLER_BUTTON_PADDLE1:
    return controller::button::kPaddle1;
  case SDL_CONTROLLER_BUTTON_PADDLE2:
    return controller::button::kPaddle2;
  case SDL_CONTROLLER_BUTTON_PADDLE3:
    return controller::button::kPaddle3;
  case SDL_CONTROLLER_BUTTON_PADDLE4:
    return controller::button::kPaddle4;
  case SDL_CONTROLLER_BUTTON_TOUCHPAD:
    return controller::button::kTouchpad;
  default:
    return std::nullopt;
  }
}

}  // namespace ii::io

#endif