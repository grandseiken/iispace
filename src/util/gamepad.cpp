#include "gamepad.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <OISJoyStick.h>
#include <OISInputManager.h>
#include <OISForceFeedback.h>

class Handler : public OIS::JoyStickListener {
public:
  Handler()
    : _arg(0)
    , _button(-1)
    , _axis1(-1)
    , _axis2(-1)
    , _pov(-1) {}

  ~Handler() {}

  void clear()
  {
    _button = -1;
    _pov = -1;
    _axis1 = -1;
    _axis2 = -1;
  }

  const OIS::Object* arg()
  {
    return _arg;
  }

  int button()
  {
    int t = _button;
    _button = -1;
    return t;
  }

  std::pair<std::pair<int, bool>, std::pair<int, bool>> axis()
  {
    if (_axis1 == -1 || _axis2 == -1) {
      return std::make_pair(std::make_pair(-1, false),
                            std::make_pair(-1, false));
    }
    std::pair<std::pair<int, bool>, std::pair<int, bool>> axes(
        std::make_pair(_axis1, _axis1r), std::make_pair(_axis2, _axis2r));
    _axis1 = -1;
    _axis2 = -1;
    return axes;
  }

  int pov()
  {
    int t = _pov;
    _pov = -1;
    return t;
  }

  bool buttonPressed(const OIS::JoyStickEvent& arg, int button)
  {
    _arg = arg.device;
    _button = button;
    return true;
  }

  bool buttonReleased(const OIS::JoyStickEvent& arg, int button)
  {
    return true;
  }

  bool axisMoved(const OIS::JoyStickEvent& arg, int axis)
  {
    if ((_axis1 == -1 || _axis1 != axis) &&
        (arg.state.mAxes[axis].abs > 20000 ||
         arg.state.mAxes[axis].abs < -20000)) {
      if (_axis1 == -1) {
        _axis1 = axis;
        _axis1r = arg.state.mAxes[axis].abs > 0;
      }
      else {
        _axis2 = axis;
        _axis2r = arg.state.mAxes[axis].abs < 0;
      }
    }
    return true;
  }

  bool sliderMoved(const OIS::JoyStickEvent& arg, int slider)
  {
    return true;
  }

  bool povMoved(const OIS::JoyStickEvent& arg, int pov)
  {
    if (arg.state.mPOV[pov].direction != OIS::Pov::Centered) {
      _pov = pov;
    }
    return true;
  }

  bool vector3Moved(const OIS::JoyStickEvent& arg, int index)
  {
    return true;
  }

private:

  const OIS::Object* _arg;
  int _button;
  int _axis1;
  int _axis2;
  bool _axis1r;
  bool _axis2r;
  int _pov;

};

int assignStart(Handler& handler, OIS::JoyStick** pads,
                int& i, const std::string& name,
                PadConfig::Buttons& buttons, std::string& devName,
                const std::vector<int>& done)
{
  std::cout <<
      "Press the [" << name << "] button on the gamepad for PLAYER " <<
      i + 1 << ":\n";

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
        devName = handler.arg()->vendor();
        std::cout <<
            "\tAssigned button " << t << " to " <<
            name << " on " << devName << "\n";
        return t;
      }
    }
  }
}

void assign(Handler& handler, OIS::JoyStick** pads, int p, int i,
            const std::string& name, PadConfig::Sticks& sticks,
            PadConfig::Buttons& dpads, int start, const std::string& devName)
{
  std::cout <<
      "Tilt any [" << name <<
      "] sticks FIRST UPWARDS AND THEN TO THE RIGHT, and press any [" << name <<
      "] D-pads, on gamepad for PLAYER " << i + 1 <<
      "\n(or press [START] to continue):\n";

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
        std::cout <<
            "\tAssigned D-pad " << t << " to " <<
            name << " on " << devName << "\n";
      }
    }
    std::pair<std::pair<int, bool>, std::pair<int, bool>> u = handler.axis();
    if (u.first.first >= 0 && u.second.first >= 0 &&
        u.first.first != u.second.first) {
      bool found = false;
      for (std::size_t j = 0; j < sticks.size(); ++j) {
        if (sticks[j]._axis1 == u.first.first ||
            sticks[j]._axis2 == u.second.first ||
            sticks[j]._axis1 == u.second.first ||
            sticks[j]._axis2 == u.first.first) {
          found = true;
        }
      }
      if (!found) {
        PadConfig::Stick stick;
        stick._axis1 = u.first.first;
        stick._axis1r = u.first.second;
        stick._axis2 = u.second.first;
        stick._axis2r = u.second.second;
        sticks.push_back(stick);
        std::cout <<
            "\tAssigned stick " <<
            stick._axis1 << ", " << stick._axis2 << " to " <<
            name << " on " << devName << "\n";
      }
    }
    if (handler.button() == start) {
      return;
    }
  }
}

void assign(Handler& handler, OIS::JoyStick** pads, int p, int i,
            const std::string& name, PadConfig::Buttons& buttons,
            int start, const std::string& devName)
{
  std::cout <<
      "Press any [" << name << "] buttons on gamepad #" << i + 1 <<
      "\n(or press [START] to continue):\n";

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
        std::cout <<
            "\tAssigned button " << t << " to " <<
            name << " on " << devName << "\n";
      }
    }
  }
}

int main(int argc, char** argv)
{
  OIS::InputManager* manager = OIS::InputManager::createInputSystem(0);
  int n = std::min(4, manager->getNumberOfDevices(OIS::OISJoyStick));
  std::cout << "\nNumber of gamepads found: " << n << "\n\n";

  OIS::JoyStick* pads[4] = {0, 0, 0, 0};
  Handler handler;
  for (int i = 0; i < n; ++i) {
    try {
      pads[i] =
          (OIS::JoyStick*) manager->createInputObject(OIS::OISJoyStick, true);
      pads[i]->setEventCallback(&handler);

      std::cout << pads[i]->vendor() <<
        "\n\tAxes: " << pads[i]->getNumberOfComponents(OIS::OIS_Axis) <<
        "\n\tD-pads: " << pads[i]->getNumberOfComponents(OIS::OIS_POV) <<
        "\n\tButtons: " << pads[i]->getNumberOfComponents(OIS::OIS_Button) <<
        "\n\n";
    }
    catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }

  PadConfig config[4];
  std::vector< int > done;
  for (int i = 0; i < n; ++i) {
    handler.clear();
    int p = i;
    int start = assignStart(handler, pads, p, "START/MENU",
                            config[i]._startButtons, config[i]._name, done);
    done.push_back(p);

    assign(handler, pads, p, i, "MOVEMENT",
           config[i]._moveSticks, config[i]._moveDpads,
           start, config[i]._name);
    assign(handler, pads, p, i, "AIMING",
           config[i]._aimSticks, config[i]._aimDpads,
           start, config[i]._name);

    assign(handler, pads, p, i, "FIRE/ACCEPT",
           config[i]._fireButtons, start, config[i]._name);
    assign(handler, pads, p, i, "BOMB/CANCEL",
           config[i]._bombButtons, start, config[i]._name);
    assign(handler, pads, p, i, "MOVE UP",
           config[i]._moveUpButtons, start, config[i]._name);
    assign(handler, pads, p, i, "MOVE DOWN",
           config[i]._moveDownButtons, start, config[i]._name);
    assign(handler, pads, p, i, "MOVE LEFT",
           config[i]._moveLeftButtons, start, config[i]._name);
    assign(handler, pads, p, i, "MOVE RIGHT",
           config[i]._moveRightButtons, start, config[i]._name);
    std::cout << "\n";
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
