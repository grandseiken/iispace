#include "game/io/input.h"

namespace ii::io {
namespace controller {
const char* to_string(axis a, type t) {
  bool xbox = t != type::kPsx;
  switch (a) {
  case axis::kLX:
    return "LStick X";
  case axis::kLY:
    return "LStick Y";
  case axis::kRX:
    return "RStick X";
  case axis::kRY:
    return "RStick Y";
  case axis::kLT:
    return xbox ? "LT" : "L2";
  case axis::kRT:
    return xbox ? "RT" : "R2";
  default:
    return "???";
  }
}

const char* to_string(button b, type t) {
  bool xbox = t != type::kPsx;
  switch (b) {
  case button::kA:
    return xbox ? "A" : "X";
  case button::kB:
    return xbox ? "B" : "Circle";
  case button::kX:
    return xbox ? "X" : "Square";
  case button::kY:
    return xbox ? "Y" : "Triangle";
  case button::kBack:
    return "Back";
  case button::kGuide:
    return "Guide";
  case button::kStart:
    return "Start";
  case button::kLStick:
    return xbox ? "LStick" : "L3";
  case button::kRStick:
    return xbox ? "RStick" : "R3";
  case button::kLShoulder:
    return xbox ? "LB" : "L1";
  case button::kRShoulder:
    return xbox ? "RB" : "R1";
  case button::kDpadUp:
    return "Up";
  case button::kDpadDown:
    return "Down";
  case button::kDpadLeft:
    return "Left";
  case button::kDpadRight:
    return "Right";
  case button::kMisc1:
    return "Misc";
  case button::kPaddle1:
    return "Paddle1";
  case button::kPaddle2:
    return "Paddle2";
  case button::kPaddle3:
    return "Paddle3";
  case button::kPaddle4:
    return "Paddle4";
  case button::kTouchpad:
    return "Touchpad";
  default:
    return "???";
  }
}
}  // namespace controller

namespace keyboard {
const char* to_string(key k) {
  switch (k) {
  case key::k0:
    return "0";
  case key::k1:
    return "1";
  case key::k2:
    return "2";
  case key::k3:
    return "3";
  case key::k4:
    return "4";
  case key::k5:
    return "5";
  case key::k6:
    return "6";
  case key::k7:
    return "7";
  case key::k8:
    return "8";
  case key::k9:
    return "9";
  case key::kA:
    return "A";
  case key::kB:
    return "B";
  case key::kC:
    return "C";
  case key::kD:
    return "D";
  case key::kE:
    return "E";
  case key::kF:
    return "F";
  case key::kG:
    return "G";
  case key::kH:
    return "H";
  case key::kI:
    return "I";
  case key::kJ:
    return "J";
  case key::kK:
    return "K";
  case key::kL:
    return "L";
  case key::kM:
    return "M";
  case key::kN:
    return "N";
  case key::kO:
    return "O";
  case key::kP:
    return "P";
  case key::kQ:
    return "Q";
  case key::kR:
    return "R";
  case key::kS:
    return "S";
  case key::kT:
    return "T";
  case key::kU:
    return "U";
  case key::kV:
    return "V";
  case key::kW:
    return "W";
  case key::kX:
    return "X";
  case key::kY:
    return "Y";
  case key::kZ:
    return "Z";
  case key::kF1:
    return "F1";
  case key::kF2:
    return "F2";
  case key::kF3:
    return "F3";
  case key::kF4:
    return "F4";
  case key::kF5:
    return "F5";
  case key::kF6:
    return "F6";
  case key::kF7:
    return "F7";
  case key::kF8:
    return "F8";
  case key::kF9:
    return "F9";
  case key::kF10:
    return "F10";
  case key::kF11:
    return "F11";
  case key::kF12:
    return "F12";
  case key::kF13:
    return "F13";
  case key::kF14:
    return "F14";
  case key::kF15:
    return "F15";
  case key::kF16:
    return "F16";
  case key::kF17:
    return "F17";
  case key::kF18:
    return "F18";
  case key::kF19:
    return "F19";
  case key::kF20:
    return "F20";
  case key::kF21:
    return "F21";
  case key::kF22:
    return "F22";
  case key::kF23:
    return "F23";
  case key::kF24:
    return "F24";
  case key::kEscape:
    return "Escape";
  case key::kTab:
    return "Tab";
  case key::kCapslock:
    return "CapsLock";
  case key::kBackspace:
    return "Backspace";
  case key::kSpacebar:
    return "Space";
  case key::kReturn:
    return "Return";
  case key::kLShift:
    return "LeftShift";
  case key::kRShift:
    return "RightShift";
  case key::kLAlt:
    return "LeftAlt";
  case key::kRAlt:
    return "RightAlt";
  case key::kLCtrl:
    return "LeftCtrl";
  case key::kRCtrl:
    return "RightCtrl";
  case key::kLSuper:
    return "LeftSuper";
  case key::kRSuper:
    return "RightSuper";
  case key::kLArrow:
    return "Left";
  case key::kRArrow:
    return "Right";
  case key::kUArrow:
    return "Up";
  case key::kDArrow:
    return "Down";
  case key::kGrave:
    return "`";
  case key::kBackslash:
    return "\\";
  case key::kComma:
    return ",";
  case key::kQuote:
    return "'";
  case key::kSlash:
    return "/";
  case key::kMinus:
    return "-";
  case key::kSemicolon:
    return ";";
  case key::kPeriod:
    return ".";
  case key::kLBracket:
    return "[";
  case key::kRBracket:
    return "]";
  case key::kLParen:
    return "(";
  case key::kRParen:
    return ")";
  case key::kEquals:
    return "=";
  case key::kLess:
    return "<";
  case key::kGreater:
    return ">";
  case key::kAmpersand:
    return "&";
  case key::kAsterisk:
    return "*";
  case key::kAt:
    return "@";
  case key::kCaret:
    return "^";
  case key::kColon:
    return ":";
  case key::kDollar:
    return "$";
  case key::kHash:
    return "#";
  case key::kExclamation:
    return "!";
  case key::kPercent:
    return "%";
  case key::kPlus:
    return "+";
  case key::kQuestion:
    return "?";
  case key::kDoubleQuote:
    return "\"";
  case key::kUnderscore:
    return "_";
  case key::kHome:
    return "Home";
  case key::kEnd:
    return "End";
  case key::kDelete:
    return "Delete";
  case key::kInsert:
    return "Insert";
  case key::kPageDown:
    return "PageDown";
  case key::kPageUp:
    return "PageUp";
  case key::kPrintScreen:
    return "PrintScreen";
  case key::kPauseBreak:
    return "Pause/Break";
  case key::kScrollLock:
    return "ScrollLock";
  case key::kNumlock:
    return "NumLock";
  case key::kKeypad0:
    return "Keypad0";
  case key::kKeypad1:
    return "Keypad1";
  case key::kKeypad2:
    return "Keypad2";
  case key::kKeypad3:
    return "Keypad3";
  case key::kKeypad4:
    return "Keypad4";
  case key::kKeypad5:
    return "Keypad5";
  case key::kKeypad6:
    return "Keypad6";
  case key::kKeypad7:
    return "Keypad7";
  case key::kKeypad8:
    return "Keypad8";
  case key::kKeypad9:
    return "Keypad9";
  case key::kKeypadEnter:
    return "KeypadEnter";
  case key::kKeypadPeriod:
    return "KeypadPeriod";
  case key::kKeypadPlus:
    return "Keypad+";
  case key::kKeypadMinus:
    return "Keypad-";
  case key::kKeypadMultiply:
    return "Keypad*";
  case key::kKeypadDivide:
    return "Keypad/";
  default:
    return "???";
  }
}
}  // namespace keyboard

namespace mouse {
const char* to_string(button b) {
  switch (b) {
  case button::kL:
    return "LMB";
  case button::kM:
    return "Mouse3";
  case button::kR:
    return "RMB";
  case button::kX1:
    return "Mouse4";
  case button::kX2:
    return "Mouse5";
  default:
    return "???";
  }
}
}  // namespace mouse
}  // namespace ii::io