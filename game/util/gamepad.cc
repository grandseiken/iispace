#include "game/util/gamepad.h"
#include <OISForceFeedback.h>
#include <OISInputManager.h>
#include <OISJoyStick.h>
#include <algorithm>
#include <iostream>
#include <string>

#ifdef _MSC_VER
#include <windows.h>
#endif

class Handler : public OIS::JoyStickListener {
public:
  Handler() : arg_(0), button_(-1), axis1_(-1), axis2_(-1), pov_(-1) {}

  ~Handler() {}

  void clear() {
    button_ = -1;
    pov_ = -1;
    axis1_ = -1;
    axis2_ = -1;
  }

  const OIS::Object* arg() {
    return arg_;
  }

  int button() {
    int t = button_;
    button_ = -1;
    return t;
  }

  std::pair<std::pair<int, bool>, std::pair<int, bool>> axis() {
    if (axis1_ == -1 || axis2_ == -1) {
      return std::make_pair(std::make_pair(-1, false), std::make_pair(-1, false));
    }
    std::pair<std::pair<int, bool>, std::pair<int, bool>> axes(std::make_pair(axis1_, axis1r_),
                                                               std::make_pair(axis2_, axis2r_));
    axis1_ = -1;
    axis2_ = -1;
    return axes;
  }

  int pov() {
    int t = pov_;
    pov_ = -1;
    return t;
  }

  bool buttonPressed(const OIS::JoyStickEvent& arg, int button) override {
    arg_ = arg.device;
    button_ = button;
    return true;
  }

  bool buttonReleased(const OIS::JoyStickEvent& arg, int button) override {
    return true;
  }

  bool axisMoved(const OIS::JoyStickEvent& arg, int axis) override {
    if ((axis1_ == -1 || axis1_ != axis) &&
        (arg.state.mAxes[axis].abs > 20000 || arg.state.mAxes[axis].abs < -20000)) {
      if (axis1_ == -1) {
        axis1_ = axis;
        axis1r_ = arg.state.mAxes[axis].abs > 0;
      } else {
        axis2_ = axis;
        axis2r_ = arg.state.mAxes[axis].abs < 0;
      }
    }
    return true;
  }

  bool sliderMoved(const OIS::JoyStickEvent& arg, int slider) override {
    return true;
  }

  bool povMoved(const OIS::JoyStickEvent& arg, int pov) override {
    if (arg.state.mPOV[pov].direction != OIS::Pov::Centered) {
      pov_ = pov;
    }
    return true;
  }

  bool vector3Moved(const OIS::JoyStickEvent& arg, int index) override {
    return true;
  }

private:
  const OIS::Object* arg_;
  int button_;
  int axis1_;
  int axis2_;
  bool axis1r_;
  bool axis2r_;
  int pov_;
};

int assign_start(Handler& handler, OIS::JoyStick** pads, int& i, const std::string& name,
                 PadConfig::Buttons& buttons, std::string& dev_name, const std::vector<int>& done) {
  std::cout << "Press the [" << name << "] button on the gamepad for PLAYER " << i + 1 << ":"
            << std::endl;

  handler.clear();
  while (true) {
    for (int j = 0; j < 4; ++j) {
      if (pads[j]) {
        pads[j]->capture();
      }
    }
    int t = handler.button();
    if (t >= 0) {
      int p;
      for (p = 0; p < 4; ++p) {
        if (handler.arg() == 0) {
          continue;
        }
        if (handler.arg() == pads[p]) {
          break;
        }
      }

      for (std::size_t j = 0; j < done.size(); ++j) {
        if (done[j] == p) {
          p = 4;
        }
      }
      if (p == 4) {
        continue;
      }

      bool found = false;
      for (std::size_t j = 0; j < buttons.size(); ++j) {
        if (buttons[j] == t) {
          found = true;
        }
      }

      if (!found) {
        i = p;
        buttons.push_back(t);
        dev_name = handler.arg()->vendor();
        std::cout << "\tAssigned button " << t << " to " << name << " on " << dev_name << std::endl;
        return t;
      }
    }
  }
}

void assign(Handler& handler, OIS::JoyStick** pads, int p, int i, const std::string& name,
            PadConfig::Sticks& sticks, PadConfig::Buttons& dpads, int start,
            const std::string& dev_name) {
  std::cout << "Tilt any [" << name
            << "] sticks FIRST UPWARDS AND THEN TO THE RIGHT, and press any [" << name
            << "] D-pads, on gamepad for PLAYER " << i + 1
            << "\n(or press [START] to continue):" << std::endl;

  handler.clear();
  while (true) {
    pads[p]->capture();
    int t = handler.pov();
    if (t >= 0) {
      bool found = false;
      for (std::size_t j = 0; j < dpads.size(); ++j) {
        if (dpads[j] == t) {
          found = true;
        }
      }
      if (!found) {
        dpads.push_back(t);
        std::cout << "\tAssigned D-pad " << t << " to " << name << " on " << dev_name << std::endl;
      }
    }
    std::pair<std::pair<int, bool>, std::pair<int, bool>> u = handler.axis();
    if (u.first.first >= 0 && u.second.first >= 0 && u.first.first != u.second.first) {
      bool found = false;
      for (std::size_t j = 0; j < sticks.size(); ++j) {
        if (sticks[j].axis1_ == u.first.first || sticks[j].axis2_ == u.second.first ||
            sticks[j].axis1_ == u.second.first || sticks[j].axis2_ == u.first.first) {
          found = true;
        }
      }
      if (!found) {
        PadConfig::Stick stick;
        stick.axis1_ = u.first.first;
        stick.axis1r_ = u.first.second;
        stick.axis2_ = u.second.first;
        stick.axis2r_ = u.second.second;
        sticks.push_back(stick);
        std::cout << "\tAssigned stick " << stick.axis1_ << ", " << stick.axis2_ << " to " << name
                  << " on " << dev_name << std::endl;
      }
    }
    if (handler.button() == start) {
      return;
    }
  }
}

void assign(Handler& handler, OIS::JoyStick** pads, int p, int i, const std::string& name,
            PadConfig::Buttons& buttons, int start, const std::string& dev_name) {
  std::cout << "Press any [" << name << "] buttons on gamepad #" << i + 1
            << "\n(or press [START] to continue):" << std::endl;

  handler.clear();
  while (true) {
    pads[p]->capture();
    int t = handler.button();
    if (t == start) {
      return;
    }
    if (t >= 0) {
      bool found = false;
      for (std::size_t j = 0; j < buttons.size(); ++j) {
        if (buttons[j] == t) {
          found = true;
        }
      }
      if (!found) {
        buttons.push_back(t);
        std::cout << "\tAssigned button " << t << " to " << name << " on " << dev_name << std::endl;
      }
    }
  }
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
  static constexpr std::size_t kBufferSize = 1024;
  char old_title[kBufferSize];
  char new_title[kBufferSize];
  GetConsoleTitle(old_title, kBufferSize);
  wsprintf(new_title, "%d/%d", GetTickCount(), GetCurrentProcessId());
  SetConsoleTitle(new_title);
  Sleep(40);
  HWND handle = FindWindow(nullptr, new_title);
  SetConsoleTitle(old_title);
  OIS::InputManager* manager =
      OIS::InputManager::createInputSystem(reinterpret_cast<std::size_t>(handle));
#else
  OIS::InputManager* manager = OIS::InputManager::createInputSystem(0);
#endif
  int n = std::min(4, manager->getNumberOfDevices(OIS::OISJoyStick));
  std::cout << "\nNumber of gamepads found: " << n << "\n" << std::endl;

  OIS::JoyStick* pads[4] = {0, 0, 0, 0};
  Handler handler;
  for (int i = 0; i < n; ++i) {
    try {
      pads[i] = (OIS::JoyStick*)manager->createInputObject(OIS::OISJoyStick, true);
      pads[i]->setEventCallback(&handler);

      std::cout << pads[i]->vendor()
                << "\n\tAxes: " << pads[i]->getNumberOfComponents(OIS::OIS_Axis)
                << "\n\tD-pads: " << pads[i]->getNumberOfComponents(OIS::OIS_POV)
                << "\n\tButtons: " << pads[i]->getNumberOfComponents(OIS::OIS_Button) << "\n"
                << std::endl;
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  }

  PadConfig config[4];
  std::vector<int> done;
  for (int i = 0; i < n; ++i) {
    handler.clear();
    int p = i;
    int start = assign_start(handler, pads, p, "START/MENU", config[i]._startButtons,
                             config[i].name_, done);
    done.push_back(p);

    assign(handler, pads, p, i, "MOVEMENT", config[i]._moveSticks, config[i]._moveDpads, start,
           config[i].name_);
    assign(handler, pads, p, i, "AIMING", config[i]._aimSticks, config[i]._aimDpads, start,
           config[i].name_);

    assign(handler, pads, p, i, "FIRE/ACCEPT", config[i]._fireButtons, start, config[i].name_);
    assign(handler, pads, p, i, "BOMB/CANCEL", config[i]._bombButtons, start, config[i].name_);
    assign(handler, pads, p, i, "MOVE UP", config[i]._moveUpButtons, start, config[i].name_);
    assign(handler, pads, p, i, "MOVE DOWN", config[i]._moveDownButtons, start, config[i].name_);
    assign(handler, pads, p, i, "MOVE LEFT", config[i]._moveLeftButtons, start, config[i].name_);
    assign(handler, pads, p, i, "MOVE RIGHT", config[i]._moveRightButtons, start, config[i].name_);
    std::cout << std::endl;
  }
  std::ofstream f("gamepads.txt");
  f << n << "\n";
  for (int i = 0; i < n; ++i) {
    config[i].Write(f);
  }
  f.close();
  OIS::InputManager::destroyInputSystem(manager);
  return 0;
}
