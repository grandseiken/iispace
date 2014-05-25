#ifndef LIBWII_H
#define LIBWII_H

#include "lib.h"
#include "gamepad/gamepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <OISJoyStick.h>
#include <OISInputManager.h>
#include <OISForceFeedback.h>

class LibWin : public Lib {
public:

  LibWin();
  virtual ~LibWin();

  // General
  //------------------------------
  virtual void Init();
  virtual void SetWorkingDirectory(bool original);
  virtual void BeginFrame();
  virtual void EndFrame();
  virtual void CaptureMouse(bool enabled);
  virtual void NewGame();

  virtual void Exit(ExitType t);
  virtual ExitType GetExitType() const;
  virtual void SystemExit(bool powerOff) const;

  virtual int RandInt(int lessThan);
  virtual fixed RandFloat();

  virtual Settings LoadSettings() const;
  virtual void SetVolume(int volume);

  virtual std::string SavePath() const
  {
    return "wiispace.sav";
  }

  virtual std::string SettingsPath() const
  {
    return "wiispace.txt";
  }

  virtual std::string ScreenshotPath() const
  {
    std::stringstream ss;
    ss << "screenshot" << time(0) % 10000000 << ".png";
    return ss.str();
  }

  // Input
  //------------------------------
  virtual PadType IsPadConnected(int player) const;

  virtual bool IsKeyPressed(int player, Key k) const;
  virtual bool IsKeyReleased(int player, Key k) const;
  virtual bool IsKeyHeld(int player, Key k) const;

  virtual Vec2 GetMoveVelocity(int player) const;
  virtual Vec2 GetFireTarget(int player, const Vec2& position) const;

  // Output
  //------------------------------
  virtual void ClearScreen() const;
  virtual void RenderLine(const Vec2f& a, const Vec2f& b, Colour c) const;
  virtual void RenderText(
      const Vec2f& v, const std::string& text, Colour c) const;
  virtual void RenderRect(
      const Vec2f& low, const Vec2f& hi, Colour c, int lineWidth = 0) const;
  virtual void Render() const;

  virtual void Rumble(int player, int time);
  virtual void StopRumble();
  virtual bool PlaySound(
      Sound sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);

  virtual void TakeScreenShot();

private:

  // Internal
  //------------------------------
  mutable sf::RenderWindow _window;
  sf::Image _image;
  mutable sf::Sprite _font;
  ExitType _exitType;
  char* _cwd;
  std::vector<char>  _exe;

  std::vector<std::vector<bool>> _keysPressed;
  std::vector<std::vector<bool>> _keysHeld;
  std::vector<std::vector<bool>> _keysReleased;

  bool _captureMouse;
  int _mousePosX;
  int _mousePosY;
  int _extraX;
  int _extraY;
  mutable bool _mouseMoving;

  class Handler : public OIS::JoyStickListener {
  public:

    void SetLib(LibWin* lib)
    {
      _lib = lib;
    }

    bool buttonPressed(const OIS::JoyStickEvent& arg, int button);
    bool buttonReleased(const OIS::JoyStickEvent& arg, int button);
    bool axisMoved(const OIS::JoyStickEvent& arg, int axis);
    bool povMoved(const OIS::JoyStickEvent& arg, int pov);

  private:

    LibWin* _lib;

  };

  OIS::InputManager*  _manager;
  Handler _padHandler;
  int _padCount;
  PadConfig _padConfig[PLAYERS];
  OIS::JoyStick* _pads[PLAYERS];
  OIS::ForceFeedback* _ff[PLAYERS];
  fixed _padMoveVAxes[PLAYERS];
  fixed _padMoveHAxes[PLAYERS];
  fixed _padAimVAxes[PLAYERS];
  fixed _padAimHAxes[PLAYERS];
  int _padMoveDpads[PLAYERS];
  int _padAimDpads[PLAYERS];
  mutable Vec2 _padLastAim[PLAYERS];

  typedef std::pair<int, sf::SoundBuffer*> NamedSound;
  typedef std::pair<int, NamedSound> SoundResource;
  typedef std::vector<SoundResource> SoundList;
  SoundList _sounds;
  std::vector<sf::Sound*> _voices;

  void LoadSounds();

  // Data
  //------------------------------
  Settings _settings;

};

#endif
