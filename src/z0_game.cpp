#include "z0_game.h"
#include "lib.h"
#include "player.h"
#include "boss.h"
#include "enemy.h"
#include "overmind.h"
#include "stars.h"
#include <algorithm>
#include <iostream>

const int z0Game::STARTING_LIVES = 2;
const int z0Game::BOSSMODE_LIVES = 1;

z0Game::z0Game(Lib& lib, std::vector< std::string > args)
  : _lib(lib)
  , _state(STATE_MENU)
  , _players(1)
  , _lives(0)
  , _mode(NORMAL_MODE)
  , _showHPBar(false)
  , _fillHPBar(0)
  , _selection(0)
  , _specialSelection(0)
  , _killTimer(0)
  , _exitTimer(0)
  , _enterChar(0)
  , _enterTime(0)
  , _scoreScreenTimer(0)
  , _controllersConnected(0)
  , _controllersDialog(false)
  , _firstControllersDialog(false)
  , _overmind(new Overmind(*this))
  , _bossesKilled(0)
  , _hardModeBossesKilled(0)
{
  Lib::SaveData save = lib.LoadSaveData();
  _highScores = save._highScores;
  _bossesKilled = save._bossesKilled;
  _hardModeBossesKilled = save._hardModeBossesKilled;

  // Compliments have a max length of 24
  _compliments.push_back(" is a swell guy!");
  _compliments.push_back(" went absolutely mental!");
  _compliments.push_back(" is the bee's knees.");
  _compliments.push_back(" goes down in history!");
  _compliments.push_back(" is old school!");
  _compliments.push_back(", oh, how cool you are!");
  _compliments.push_back(" got the respect!");
  _compliments.push_back(" had a cow!");
  _compliments.push_back(" is a major badass.");
  _compliments.push_back(" is kickin' rad.");
  _compliments.push_back(" wins a coconut.");
  _compliments.push_back(" wins a kipper.");
  _compliments.push_back(" is probably cheating!");
  _compliments.push_back(" is totally the best!");
  _compliments.push_back(" ate your face!");
  _compliments.push_back(" is feeling kinda funky.");
  _compliments.push_back(" is the cat's pyjamas.");
  _compliments.push_back(", air guitar solo time!");
  _compliments.push_back(", give us a smile.");
  _compliments.push_back(" is a cheeky fellow!");
  _compliments.push_back(" is a slippery customer!");
  _compliments.push_back("... that's is a puzzle!");

  if (!args.empty()) {
    Player::replay = Replay(args[0]);
    Player::replay_frame = 0;
    if (Player::replay.okay) {
      _players = Player::replay.players;
      lib.SetPlayerCount(_players);
      NewGame(Player::replay.can_face_secret_boss,
              true, game_mode(Player::replay.game_mode));
    }
    else {
      _exitTimer = 256;
    }
  }
}

z0Game::~z0Game()
{
  for (const auto& ship : _ships) {
    if (ship->type() & Ship::SHIP_ENEMY) {
      _overmind->OnEnemyDestroy(*ship);
    }
  }
}

void z0Game::Run()
{
  while (true) {
    std::size_t f = _lib.GetFrameCount();
    if (!f) {
      if (_lib.Exit()) {
        return;
      }
      continue;
    }

    for (std::size_t i = 0; i < f; ++i) {
      _lib.BeginFrame();
      if (_lib.Exit()) {
        _lib.EndFrame();
        return;
      }

      Update();
      if (_lib.Exit()) {
        _lib.EndFrame();
        return;
      }

      _lib.EndFrame();
      if (_lib.Exit()) {
        return;
      }
    }

    _lib.ClearScreen();
    Render();
    _lib.Render();
  }
}

void z0Game::Update()
{
  if (_exitTimer) {
    _exitTimer--;
    if (!_exitTimer) {
      lib().Exit(true);
      _exitTimer = -1;
    }
    return;
  }

  for (int32_t i = 0; i < Lib::PLAYERS; i++) {
    if (lib().IsKeyHeld(i, Lib::KEY_FIRE) &&
        lib().IsKeyHeld(i, Lib::KEY_BOMB) &&
        lib().IsKeyPressed(Lib::KEY_MENU)) {
      lib().TakeScreenShot();
      break;
    }
  }
  lib().CaptureMouse(false);

  // Main menu
  //------------------------------
  if (_state == STATE_MENU) {
    if (lib().IsKeyPressed(Lib::KEY_UP)) {
      _selection--;
      if (_selection < 0 && !IsBossModeUnlocked()) {
        _selection = 0;
      }
      else if (_selection < -1 && IsBossModeUnlocked()) {
        _selection = -1;
      }
      else {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().IsKeyPressed(Lib::KEY_DOWN)) {
      _selection++;
      if (_selection > 2) {
        _selection = 2;
      }
      else {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
    }

    if (lib().IsKeyPressed(Lib::KEY_ACCEPT) ||
        lib().IsKeyPressed(Lib::KEY_MENU)) {
      if (_selection == 0) {
        NewGame(IsFastModeUnlocked(), false, NORMAL_MODE);
      }
      else if (_selection == 1) {
        _players++;
        if (_players > Lib::PLAYERS) {
          _players = 1;
        }
        lib().SetPlayerCount(_players);
      }
      else if (_selection == 2) {
        _exitTimer = 2;
      }
      else if (_selection == -1) {
        NewGame(IsFastModeUnlocked(), false,
                _specialSelection == 0 ? BOSS_MODE :
                _specialSelection == 1 ? HARD_MODE :
                _specialSelection == 2 ? FAST_MODE :
                _specialSelection == 3 ? WHAT_MODE : NORMAL_MODE);
      }

      if (_selection == 1) {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      else {
        lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
      }
    }

    if (_selection == 1) {
      int t = _players;
      if (lib().IsKeyPressed(Lib::KEY_LEFT) && _players > 1) {
        _players--;
      }
      else if (lib().IsKeyPressed(Lib::KEY_RIGHT) && _players < Lib::PLAYERS) {
        _players++;
      }
      if (t != _players){
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      lib().SetPlayerCount(_players);
    }

    if (_selection == -1) {
      int t = _specialSelection;
      if (lib().IsKeyPressed(Lib::KEY_LEFT) && _specialSelection > 0) {
        _specialSelection--;
      }
      else if (lib().IsKeyPressed(Lib::KEY_RIGHT)) {
        if ((t == 0 && IsHardModeUnlocked()) ||
            (t == 1 && IsFastModeUnlocked()) ||
            (t == 2 && IsWhatModeUnlocked())) {
          _specialSelection++;
        }
      }
      if (t != _specialSelection) {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
    }

    if (_selection >= 0 || _specialSelection == 0) {
      lib().SetColourCycle(0);
    }
    else if (_specialSelection == 1) {
      lib().SetColourCycle(128);
    }
    else if (_specialSelection == 2) {
      lib().SetColourCycle(192);
    }
    else if (_specialSelection == 3) {
      lib().SetColourCycle((lib().GetColourCycle() + 1) % 256);
    }
  }
  // Paused
  //------------------------------
  else if (_state == STATE_PAUSE) {
    int t = _selection;
    if (lib().IsKeyPressed(Lib::KEY_UP) && _selection > 0) {
      _selection--;
    }
    if (lib().IsKeyPressed(Lib::KEY_DOWN) && _selection < 2) {
      _selection++;
    }
    if (t != _selection) {
      lib().PlaySound(Lib::SOUND_MENU_CLICK);
    }

    if (lib().IsKeyPressed(Lib::KEY_ACCEPT) ||
        lib().IsKeyPressed(Lib::KEY_MENU)) {
      if (_selection == 0) {
        _state = STATE_GAME;
      }
      else if (_selection == 1) {
        _state = STATE_HIGHSCORE;
        _enterChar = 0;
        _compliment = z::rand_int(_compliments.size());
        _killTimer = 0;
      }
      else if (_selection == 2) {
        lib().SetVolume(std::min(100, lib().LoadSettings()._volume.to_int() + 1));
      }
      lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
    }
    if (lib().IsKeyPressed(Lib::KEY_LEFT) && _selection == 2) {
      int t = lib().LoadSettings()._volume.to_int();
      lib().SetVolume(std::max(0, t - 1));
      if (lib().LoadSettings()._volume.to_int() != t) {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().IsKeyPressed(Lib::KEY_RIGHT) && _selection == 2) {
      int t = lib().LoadSettings()._volume.to_int();
      lib().SetVolume(std::min(100, t + 1));
      if (lib().LoadSettings()._volume.to_int() != t) {
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().IsKeyPressed(Lib::KEY_CANCEL)) {
      _state = STATE_GAME;
      lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
    }
  }
  // In-game
  //------------------------------
  else if (_state == STATE_GAME) {
    Boss::_warnings.clear();
    lib().CaptureMouse(true);

    if (_mode == HARD_MODE) {
      lib().SetColourCycle(128);
    }
    else if (_mode == FAST_MODE) {
      lib().SetColourCycle(192);
    }
    else if (_mode == WHAT_MODE) {
      lib().SetColourCycle((lib().GetColourCycle() + 1) % 256);
    }
    else {
      lib().SetColourCycle(0);
    }

    if (Player::replay.recording && _mode == FAST_MODE) {
      lib().SetFrameCount(2);
    }
    else if (Player::replay.recording) {
      lib().SetFrameCount(1);
    }

    int controllers = 0;
    for (int32_t i = 0; i < count_players(); i++) {
      controllers += lib().IsPadConnected(i);
    }
    if (controllers < _controllersConnected &&
        !_controllersDialog && Player::replay.recording) {
      _controllersDialog = true;
      lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
    }
    _controllersConnected = controllers;

    if (_controllersDialog) {
      if (lib().IsKeyPressed(Lib::KEY_MENU) ||
          lib().IsKeyPressed(Lib::KEY_ACCEPT)) {
        _controllersDialog = false;
        _firstControllersDialog = false;
        lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
        for (int32_t i = 0; i < count_players(); i++) {
          lib().Rumble(i, 10);
        }
      }
      return;
    }

    if (lib().IsKeyPressed(Lib::KEY_MENU)) {
      _state = STATE_PAUSE;
      _selection = 0;
      lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
    }

    if (!Player::replay.recording) {
      if (lib().IsKeyPressed(Lib::KEY_BOMB)) {
        lib().SetFrameCount(lib().GetFrameCount() * 2);
      }
      if (lib().IsKeyPressed(Lib::KEY_FIRE) &&
          lib().GetFrameCount() > std::size_t(_mode == FAST_MODE ? 2 : 1)) {
        lib().SetFrameCount(lib().GetFrameCount() / 2);
      }
    }

    Player::UpdateFireTimer();
    ChaserBoss::_hasCounted = false;
    std::stable_sort(_collisions.begin(), _collisions.end(), &SortShips);
    for (std::size_t i = 0; i < _ships.size(); ++i) {
      if (!_ships[i]->is_destroyed()) {
        _ships[i]->update();
      }
    }
    for (const auto& particle : _particles) {
      if (!particle->is_destroyed()) {
        particle->update();
      }
    }
    for (std::size_t i = 0; i < Boss::_fireworks.size(); ++i) {
      if (Boss::_fireworks[i].first <= 0) {
        vec2 v = _ships[0]->position();
        _ships[0]->set_position(Boss::_fireworks[i].second.first);
        _ships[0]->explosion(0xffffffff);
        _ships[0]->explosion(Boss::_fireworks[i].second.second, 16);
        _ships[0]->explosion(0xffffffff, 24);
        _ships[0]->explosion(Boss::_fireworks[i].second.second, 32);
        _ships[0]->set_position(v);
        Boss::_fireworks.erase(Boss::_fireworks.begin() + i);
        --i;
      }
      else {
        --Boss::_fireworks[i].first;
      }
    }

    for (auto it = _ships.begin(); it != _ships.end();) {
      if (!(*it)->is_destroyed()) {
        ++it;
        continue;
      }

      if ((*it)->type() & Ship::SHIP_ENEMY) {
        _overmind->OnEnemyDestroy(**it);
      }
      for (auto jt = _collisions.begin(); jt != _collisions.end();) {
        if (it->get() == *jt) {
          jt = _collisions.erase(jt);
          continue;
        }
        ++jt;
      }

      it = _ships.erase(it);
    }

    for (auto it = _particles.begin(); it != _particles.end();) {
      if ((*it)->is_destroyed()) {
        it = _particles.erase(it);
        continue;
      }
      ++it;
    }
    _overmind->Update();

    if (!_killTimer && ((killed_players() == _players && !get_lives()) ||
                        (_mode == BOSS_MODE &&
                         _overmind->GetKilledBosses() >= 6))) {
      _killTimer = 100;
    }
    if (_killTimer) {
      _killTimer--;
      if (!_killTimer) {
        _state = STATE_HIGHSCORE;
        _scoreScreenTimer = 0;
        _enterChar = 0;
        _compliment = z::rand_int(_compliments.size());
        lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
      }
    }
  }
  // Entering high-score
  //------------------------------
  else if (_state == STATE_HIGHSCORE) {
    ++_scoreScreenTimer;
#ifdef PLATFORM_SCORE
    int64_t score = GetTotalScore();
    if (_mode == BOSS_MODE) {
      score =
          _overmind->GetKilledBosses() >= 6 &&
          _overmind->GetElapsedTime() != 0 ?
              std::max(_overmind->GetElapsedTime() - 600l * get_lives(), 1l) : 0l;
    }
    std::cout << Player::replay.seed << "\n" << _players << "\n" <<
        (_mode == BOSS_MODE) << "\n" << (_mode == HARD_MODE) << "\n" <<
        (_mode == FAST_MODE) << "\n" << (_mode == WHAT_MODE) << "\n" <<
        score << "\n" << std::flush;
    throw score_finished{};
#endif

    if (IsHighScore()) {
      _enterTime++;
      std::string chars = ALLOWED_CHARS;
      if (lib().IsKeyPressed(Lib::KEY_ACCEPT) &&
          _enterName.length() < Lib::MAX_NAME_LENGTH) {
        _enterName += chars.substr(_enterChar, 1);
        _enterTime = 0;
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().IsKeyPressed(Lib::KEY_CANCEL) && _enterName.length() > 0) {
        _enterName = _enterName.substr(0, _enterName.length() - 1);
        _enterTime = 0;
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().IsKeyPressed(Lib::KEY_RIGHT)) {
        _enterR = 0;
        _enterChar++;
        if (_enterChar >= int(chars.length())) {
          _enterChar -= int(chars.length());
        }
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().IsKeyHeld(Lib::KEY_RIGHT)) {
        _enterR++;
        _enterTime = 16;
        if (_enterR % 5 == 0 && _enterR > 5) {
          _enterChar++;
          if (_enterChar >= int(chars.length())) {
            _enterChar -= int(chars.length());
          }
          lib().PlaySound(Lib::SOUND_MENU_CLICK);
        }
      }
      if (lib().IsKeyPressed(Lib::KEY_LEFT)) {
        _enterR = 0;
        _enterChar--;
        if (_enterChar < 0) {
          _enterChar += int(chars.length());
        }
        lib().PlaySound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().IsKeyHeld(Lib::KEY_LEFT)) {
        _enterR++;
        _enterTime = 16;
        if (_enterR % 5 == 0 && _enterR > 5) {
          _enterChar--;
          if (_enterChar < 0) {
            _enterChar += int(chars.length());
          }
          lib().PlaySound(Lib::SOUND_MENU_CLICK);
        }
      }

      if (lib().IsKeyPressed(Lib::KEY_MENU)) {
        lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
        if (_mode != BOSS_MODE) {
          int32_t n =
              _mode == WHAT_MODE ? 3 * Lib::PLAYERS + _players :
              _mode == FAST_MODE ? 2 * Lib::PLAYERS + _players :
              _mode == HARD_MODE ? Lib::PLAYERS + _players : _players - 1;

          Lib::HighScoreList& list = _highScores[n];
          list.push_back(Lib::HighScore(_enterName, GetTotalScore()));
          std::stable_sort(list.begin(), list.end(), &Lib::ScoreSort);
          list.erase(list.begin() + (list.size() - 1));

          Player::replay.end_recording(_enterName, GetTotalScore());
          EndGame();
        }
        else {
          int64_t score = _overmind->GetElapsedTime();
          score -= 600l * get_lives();
          if (score <= 0)
            score = 1;
          _highScores[Lib::PLAYERS][_players - 1].first = _enterName;
          _highScores[Lib::PLAYERS][_players - 1].second = score;

          Player::replay.end_recording(_enterName, score);
          EndGame();
        }
      }
    }
    else if (lib().IsKeyPressed(Lib::KEY_MENU)) {
      Player::replay.end_recording(
          "untitled",
          _mode == BOSS_MODE ?
              (_overmind->GetKilledBosses() >= 6 &&
               _overmind->GetElapsedTime() != 0 ?
                   std::max(_overmind->GetElapsedTime() -
                            600l * get_lives(), 1l)
               : 0l) : GetTotalScore());

      EndGame();
      lib().PlaySound(Lib::SOUND_MENU_ACCEPT);
    }
  }
}

void z0Game::Render() const
{
  if (_exitTimer > 0 && !Player::replay.okay && !Player::replay.error.empty()) {
    int y = 2;
    std::string s = Player::replay.error;
    std::size_t i;
    while ((i = s.find_first_of("\n")) != std::string::npos) {
      std::string t = s.substr(0, i);
      s = s.substr(i + 1);
      lib().RenderText(flvec2(2, float(y)), t, PANEL_TEXT);
      ++y;
    }
    lib().RenderText(flvec2(2, float(y)), s, PANEL_TEXT);
    return;
  }

  _showHPBar = false;
  _fillHPBar = 0;
  if (!_firstControllersDialog) {
    Stars::render(_lib);
    for (const auto& particle : _particles) {
      particle->render(lib());
    }
    for (std::size_t i = _players; i < _ships.size(); ++i) {
      _ships[i]->render();
    }
    for (std::size_t i = 0;
         i < std::size_t(_players) && i < _ships.size(); ++i) {
      _ships[i]->render();
    }
  }

  // In-game
  //------------------------------
  if (_state == STATE_GAME) {
    if (_controllersDialog) {
      RenderPanel(flvec2(3.f, 3.f), flvec2(32.f, 8.f + 2 * _players));
      lib().RenderText(flvec2(4.f, 4.f), "CONTROLLERS FOUND", PANEL_TEXT);

      for (int32_t i = 0; i < _players; i++) {
        std::stringstream ss;
        ss << "PLAYER " << (i + 1) << ": ";
        lib().RenderText(flvec2(4.f, 8.f + 2 * i), ss.str(), PANEL_TEXT);

        std::stringstream ss2;
        int pads = lib().IsPadConnected(i);
        if (!Player::replay.recording) {
          ss2 << "REPLAY";
          pads = 1;
        }
        else {
          if (!pads) {
            ss2 << "NONE";
          }
          if (pads & Lib::PAD_GAMEPAD) {
            ss2 << "GAMEPAD";
            if (pads & Lib::PAD_KEYMOUSE) {
              ss2 << ", KB/MOUSE";
            }
          }
          else if (pads & Lib::PAD_KEYMOUSE) {
            ss2 << "MOUSE & KEYBOARD";
          }
        }

        lib().RenderText(flvec2(14.f, 8.f + 2 * i), ss2.str(),
                         pads ? Player::GetPlayerColour(i) : PANEL_TEXT);
      }
      return;
    }

    std::stringstream ss;
    ss << _lives << " live(s)";
    if (_mode != BOSS_MODE && _overmind->GetTimer() >= 0) {
      int t = int(0.5f + _overmind->GetTimer() / 60);
      ss << " " << (t < 10 ? "0" : "") << t;
    }

    lib().RenderText(
        flvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
               Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f),
        ss.str(), PANEL_TRAN);

    for (std::size_t i = 0; i < _ships.size() + Boss::_warnings.size(); ++i) {
      if (i < _ships.size() && !(_ships[i]->type() & Ship::SHIP_ENEMY)) {
        continue;
      }
      flvec2 v = to_float(
          i < _ships.size() ? _ships[i]->position() :
                              Boss::_warnings[i - _ships.size()]);

      if (v.x < -4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(v.x + Lib::WIDTH, 0.f) / Lib::WIDTH);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().RenderLine(flvec2(0.f, v.y), flvec2(6, v.y - 3), a);
        lib().RenderLine(flvec2(6.f, v.y - 3), flvec2(6, v.y + 3), a);
        lib().RenderLine(flvec2(6.f, v.y + 3), flvec2(0, v.y), a);
      }
      if (v.x >= Lib::WIDTH + 4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(2 * Lib::WIDTH - v.x, 0.f) / Lib::WIDTH);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().RenderLine(flvec2(float(Lib::WIDTH), v.y),
                         flvec2(Lib::WIDTH - 6.f, v.y - 3), a);
        lib().RenderLine(flvec2(Lib::WIDTH - 6, v.y - 3),
                         flvec2(Lib::WIDTH - 6.f, v.y + 3), a);
        lib().RenderLine(flvec2(Lib::WIDTH - 6, v.y + 3),
                         flvec2(float(Lib::WIDTH), v.y), a);
      }
      if (v.y < -4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(v.y + Lib::HEIGHT, 0.f) / Lib::HEIGHT);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().RenderLine(flvec2(v.x, 0.f), flvec2(v.x - 3, 6.f), a);
        lib().RenderLine(flvec2(v.x - 3, 6.f), flvec2(v.x + 3, 6.f), a);
        lib().RenderLine(flvec2(v.x + 3, 6.f), flvec2(v.x, 0.f), a);
      }
      if (v.y >= Lib::HEIGHT + 4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(2 * Lib::HEIGHT - v.y, 0.f) / Lib::HEIGHT);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().RenderLine(flvec2(v.x, float(Lib::HEIGHT)),
                         flvec2(v.x - 3, Lib::HEIGHT - 6.f), a);
        lib().RenderLine(flvec2(v.x - 3, Lib::HEIGHT - 6.f),
                         flvec2(v.x + 3, Lib::HEIGHT - 6.f), a);
        lib().RenderLine(flvec2(v.x + 3, Lib::HEIGHT - 6.f),
                         flvec2(v.x, float(Lib::HEIGHT)), a);
      }
    }
    if (_mode == BOSS_MODE) {
      std::stringstream sst;
      sst << ConvertToTime(_overmind->GetElapsedTime());
      lib().RenderText(
        flvec2(Lib::WIDTH / (2 * Lib::TEXT_WIDTH) -
               sst.str().length() - 1.f, 1.f),
        sst.str(), PANEL_TRAN);
    }

    if (_showHPBar) {
      int x = _mode == BOSS_MODE ? 48 : 0;
      lib().RenderRect(
          flvec2(x + Lib::WIDTH / 2 - 48.f, 16.f),
          flvec2(x + Lib::WIDTH / 2 + 48.f, 32.f),
          PANEL_TRAN, 2);

      lib().RenderRect(
          flvec2(x + Lib::WIDTH / 2 - 44.f, 16.f + 4),
          flvec2(x + Lib::WIDTH / 2 - 44.f + 88.f * _fillHPBar, 32.f - 4),
          PANEL_TRAN);
    }

    if (!Player::replay.recording) {
      int x = _mode == FAST_MODE ? lib().GetFrameCount() / 2 : lib().GetFrameCount();
      std::stringstream ss;
      ss << x << "X " <<
          int(100 * float(Player::replay_frame) /
          Player::replay.player_frames.size()) << "%";

      lib().RenderText(
          flvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                 Lib::HEIGHT / Lib::TEXT_HEIGHT - 3.f),
          ss.str(), PANEL_TRAN);
    }

  }
  // Main menu screen
  //------------------------------
  else if (_state == STATE_MENU) {
    // Main menu
    //------------------------------
    RenderPanel(flvec2(3.f, 3.f), flvec2(19.f, 14.f));

    lib().RenderText(flvec2(37.f - 16, 3.f), "coded by: SEiKEN", PANEL_TEXT);
    lib().RenderText(flvec2(37.f - 16, 4.f), "stu@seiken.co.uk", PANEL_TEXT);
    lib().RenderText(flvec2(37.f - 9, 6.f), "-testers-", PANEL_TEXT);
    lib().RenderText(flvec2(37.f - 9, 7.f), "MATT BELL", PANEL_TEXT);
    lib().RenderText(flvec2(37.f - 9, 8.f), "RUFUZZZZZ", PANEL_TEXT);
    lib().RenderText(flvec2(37.f - 9, 9.f), "SHADOW1W2", PANEL_TEXT);

    std::string b = "BOSSES:  ";
    int bb = IsHardModeUnlocked() ? _hardModeBossesKilled : _bossesKilled;
    if (bb & BOSS_1A) b += "X"; else b += "-";
    if (bb & BOSS_1B) b += "X"; else b += "-";
    if (bb & BOSS_1C) b += "X"; else b += "-";
    if (bb & BOSS_3A) b += "X"; else b += " ";
    if (bb & BOSS_2A) b += "X"; else b += "-";
    if (bb & BOSS_2B) b += "X"; else b += "-";
    if (bb & BOSS_2C) b += "X"; else b += "-";
    lib().RenderText(flvec2(37.f - 16, 13.f), b, PANEL_TEXT);

    lib().RenderText(flvec2(4.f, 4.f), "WiiSPACE", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 8.f), "START GAME", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 10.f), "PLAYERS", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 12.f), "EXiT", PANEL_TEXT);

    if (_specialSelection == 0 && IsBossModeUnlocked()) {
      lib().RenderText(flvec2(6.f, 6.f), "BOSS MODE", PANEL_TEXT);
      if (IsHardModeUnlocked() && _selection == -1) {
        lib().RenderText(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_specialSelection == 1 && IsHardModeUnlocked()) {
      lib().RenderText(flvec2(6.f, 6.f), "HARD MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().RenderText(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
      if (IsFastModeUnlocked() && _selection == -1) {
        lib().RenderText(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_specialSelection == 2 && IsFastModeUnlocked()) {
      lib().RenderText(flvec2(6.f, 6.f), "FAST MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().RenderText(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
      if (IsWhatModeUnlocked() && _selection == -1) {
        lib().RenderText(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_specialSelection == 3 && IsWhatModeUnlocked()) {
      lib().RenderText(flvec2(6.f, 6.f), "W-HAT MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().RenderText(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
    }

    flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
               float((8 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
    flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
              float((9 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
    lib().RenderRect(low, hi, PANEL_TEXT, 1);

    if (_players > 1 && _selection == 1) {
      lib().RenderText(flvec2(5.f, 10.f), "<", PANEL_TRAN);
    }
    if (_players < 4 && _selection == 1) {
      lib().RenderText(flvec2(14.f + _players, 10.f), ">", PANEL_TRAN);
    }
    for (int32_t i = 0; i < _players; ++i) {
      std::stringstream ss;
      ss << (i + 1);
      lib().RenderText(flvec2(14.f + i, 10.f), ss.str(),
                       Player::GetPlayerColour(i));
    }

    // High score table
    //------------------------------
    RenderPanel(flvec2(3.f, 15.f), flvec2(37.f, 27.f));

    std::stringstream ss;
    ss << _players;
    std::string s = "HiGH SCORES    ";
    s += _selection == -1 ?
        (_specialSelection == 0 ? "BOSS MODE" :
         _specialSelection == 1 ? "HARD MODE (" + ss.str() + "P)" :
         _specialSelection == 2 ? "FAST MODE (" + ss.str() + "P)" :
         "W-HAT MODE (" + ss.str() + "P)") :
        _players == 1 ? "ONE PLAYER" :
        _players == 2 ? "TWO PLAYERS" :
        _players == 3 ? "THREE PLAYERS" :
        _players == 4 ? "FOUR PLAYERS" : "";
    lib().RenderText(flvec2(4.f, 16.f), s, PANEL_TEXT);

    if (_selection == -1 && _specialSelection == 0) {
      lib().RenderText(flvec2(4.f, 18.f), "ONE PLAYER", PANEL_TEXT);
      lib().RenderText(flvec2(4.f, 20.f), "TWO PLAYERS", PANEL_TEXT);
      lib().RenderText(flvec2(4.f, 22.f), "THREE PLAYERS", PANEL_TEXT);
      lib().RenderText(flvec2(4.f, 24.f), "FOUR PLAYERS", PANEL_TEXT);

      for (std::size_t i = 0; i < Lib::PLAYERS; i++) {
        std::string score = ConvertToTime(_highScores[Lib::PLAYERS][i].second);
        if (score.length() > Lib::MAX_SCORE_LENGTH) {
          score = score.substr(0, Lib::MAX_SCORE_LENGTH);
        }
        std::string name = _highScores[Lib::PLAYERS][i].first;
        if (name.length() > Lib::MAX_NAME_LENGTH) {
          name = name.substr(0, Lib::MAX_NAME_LENGTH);
        }

        lib().RenderText(flvec2(19.f, 18.f + i * 2), score, PANEL_TEXT);
        lib().RenderText(flvec2(19.f, 19.f + i * 2), name, PANEL_TEXT);
      }
    }
    else {
      for (std::size_t i = 0; i < Lib::NUM_HIGH_SCORES; i++) {
        std::stringstream ssi;
        ssi << (i + 1) << ".";
        lib().RenderText(flvec2(4.f, 18.f + i), ssi.str(), PANEL_TEXT);

        int n = _selection != -1 ? _players - 1 :
            _specialSelection * Lib::PLAYERS + _players;

        if (_highScores[n][i].second <= 0) {
          continue;
        }

        std::stringstream ss;
        ss << _highScores[n][i].second;
        std::string score = ss.str();
        if (score.length() > Lib::MAX_SCORE_LENGTH) {
          score = score.substr(0, Lib::MAX_SCORE_LENGTH);
        }
        std::string name = _highScores[n][i].first;
        if (name.length() > Lib::MAX_NAME_LENGTH) {
          name = name.substr(0, Lib::MAX_NAME_LENGTH);
        }

        lib().RenderText(flvec2(7.f, 18.f + i), score, PANEL_TEXT);
        lib().RenderText(flvec2(19.f, 18.f + i), name, PANEL_TEXT);
      }
    }
  }
  // Pause menu
  //------------------------------
  else if (_state == STATE_PAUSE) {
    RenderPanel(flvec2(3.f, 3.f), flvec2(15.f, 14.f));

    lib().RenderText(flvec2(4.f, 4.f), "PAUSED", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 8.f), "CONTINUE", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 10.f), "END GAME", PANEL_TEXT);
    lib().RenderText(flvec2(6.f, 12.f), "VOL.", PANEL_TEXT);
    std::stringstream vol;
    int v = lib().LoadSettings()._volume.to_int();
    vol << " " << (v < 10 ? " " : "") << v;
    lib().RenderText(flvec2(10.f, 12.f), vol.str(), PANEL_TEXT);
    if (_selection == 2 && lib().LoadSettings()._volume.to_int() > 0) {
      lib().RenderText(flvec2(5.f, 12.f), "<", PANEL_TRAN);
    }
    if (_selection == 2 && lib().LoadSettings()._volume.to_int() < 100) {
      lib().RenderText(flvec2(13.f, 12.f), ">", PANEL_TRAN);
    }

    flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
               float((8 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
    flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
              float((9 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
    lib().RenderRect(low, hi, PANEL_TEXT, 1);
  }
  // Score screen
  //------------------------------
  else if (_state == STATE_HIGHSCORE) {
    lib().SetFrameCount(1);

    // Name enter
    //------------------------------
    if (IsHighScore()) {
      std::string chars = ALLOWED_CHARS;

      RenderPanel(flvec2(3.f, 20.f), flvec2(28.f, 27.f));
      lib().RenderText(flvec2(4.f, 21.f), "It's a high score!", PANEL_TEXT);
      lib().RenderText(flvec2(4.f, 23.f),
                       _players == 1 ? "Enter name:" : "Enter team name:",
                       PANEL_TEXT);
      lib().RenderText(flvec2(6.f, 25.f), _enterName, PANEL_TEXT);
      if ((_enterTime / 16) % 2 && _enterName.length() < Lib::MAX_NAME_LENGTH) {
        lib().RenderText(flvec2(6.f + _enterName.length(), 25.f),
                         chars.substr(_enterChar, 1), 0xbbbbbbff);
      }
      flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
                 float((25 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
      flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
                float((26 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
      lib().RenderRect(low, hi, PANEL_TEXT, 1);
    }

    // Boss mode score listing
    //------------------------------
    if (_mode == BOSS_MODE) {
      int extraLives = get_lives();
      bool b = extraLives > 0 && _overmind->GetKilledBosses() >= 6;

      long score = _overmind->GetElapsedTime();
      if (b) {
        score -= 10 * extraLives;
      }
      if (score <= 0) {
        score = 1;
      }

      RenderPanel(flvec2(3.f, 3.f), flvec2(37.f, b ? 10.f : 8.f));
      if (b) {
        std::stringstream ss;
        ss << (extraLives * 10) << "-second extra-life bonus!";
        lib().RenderText(flvec2(4.f, 4.f), ss.str(), PANEL_TEXT);
      }

      lib().RenderText(flvec2(4.f, b ? 6.f : 4.f),
                       "TIME ELAPSED: " + ConvertToTime(score), PANEL_TEXT);
      std::stringstream ss;
      ss << "BOSS DESTROY: " << _overmind->GetKilledBosses();
      lib().RenderText(flvec2(4.f, b ? 8.f : 6.f), ss.str(), PANEL_TEXT);
      return;
    }

    // Score listing
    //------------------------------
    RenderPanel(flvec2(3.f, 3.f),
                flvec2(37.f, 8.f + 2 * _players + (_players > 1 ? 2 : 0)));

    std::stringstream ss;
    ss << GetTotalScore();
    std::string score = ss.str();
    if (score.length() > Lib::MAX_SCORE_LENGTH) {
      score = score.substr(0, Lib::MAX_SCORE_LENGTH);
    }
    lib().RenderText(flvec2(4.f, 4.f), "TOTAL SCORE: " + score, PANEL_TEXT);

    for (int32_t i = 0; i < _players; ++i) {
      std::stringstream sss;
      if (_scoreScreenTimer % 600 < 300) {
        sss << GetPlayerScore(i);
      }
      else {
        sss << GetPlayerDeaths(i) << " death" <<
            (GetPlayerDeaths(i) != 1 ? "s" : "");
      }
      score = sss.str();
      if (score.length() > Lib::MAX_SCORE_LENGTH) {
        score = score.substr(0, Lib::MAX_SCORE_LENGTH);
      }

      std::stringstream ssp;
      ssp << "PLAYER " << (i + 1) << ":";
      lib().RenderText(flvec2(4.f, 8.f + 2 * i), ssp.str(), PANEL_TEXT);
      lib().RenderText(flvec2(14.f, 8.f + 2 * i), score,
                       Player::GetPlayerColour(i));
    }

    // Top-ranked player
    //------------------------------
    if (_players > 1) {
      bool first = true;
      int64_t max = 0;
      std::size_t best = 0;
      for (int32_t i = 0; i < _players; ++i) {
        if (first || GetPlayerScore(i) > max) {
          max = GetPlayerScore(i);
          best = i;
        }
        first = false;
      }

      if (GetTotalScore() > 0) {
        std::stringstream s;
        s << "PLAYER " << (best + 1);
        lib().RenderText(flvec2(4.f, 8.f + 2 * _players), s.str(),
                       Player::GetPlayerColour(best));

        std::string compliment = _compliments[_compliment];
        lib().RenderText(
            flvec2(12.f, 8.f + 2 * _players), compliment, PANEL_TEXT);
      }
      else {
        lib().RenderText(
            flvec2(4.f, 8.f + 2 * _players), "Oh dear!", PANEL_TEXT);
      }
    }
  }
}

// Game control
//------------------------------
void z0Game::NewGame(bool canFaceSecretBoss, bool replay, game_mode mode)
{
  if (!replay) {
    Player::replay = Replay(_players, mode, canFaceSecretBoss);
    lib().NewGame();
  }
  _controllersDialog = true;
  _firstControllersDialog = true;
  _controllersConnected = 0;
  _mode = mode;
  _overmind->Reset(canFaceSecretBoss);
  _lives = mode == BOSS_MODE ? _players * BOSSMODE_LIVES : STARTING_LIVES;
  lib().SetFrameCount(_mode == FAST_MODE ? 2 : 1);

  Stars::clear();
  for (int32_t i = 0; i < _players; ++i) {
    vec2 v((1 + i) * Lib::WIDTH / (1 + _players), Lib::HEIGHT / 2);
    Player* p = new Player(v, i);
    AddShip(p);
    _playerList.push_back(p);
  }
  _state = STATE_GAME;
}

void z0Game::EndGame()
{
  for (const auto& ship : _ships) {
    if (ship->type() & Ship::SHIP_ENEMY) {
      _overmind->OnEnemyDestroy(*ship);
    }
  }

  Stars::clear();
  _ships.clear();
  _particles.clear();
  _playerList.clear();
  _collisions.clear();
  Boss::_fireworks.clear();
  Boss::_warnings.clear();

  Lib::SaveData save2;
  save2._highScores = _highScores;
  save2._bossesKilled = _bossesKilled;
  save2._hardModeBossesKilled = _hardModeBossesKilled;
  lib().SaveSaveData(save2);
  lib().SetFrameCount(1);
  _state = STATE_MENU;
  _selection =
      (_mode == BOSS_MODE && IsBossModeUnlocked()) ||
      (_mode == HARD_MODE && IsHardModeUnlocked()) ||
      (_mode == FAST_MODE && IsFastModeUnlocked()) ||
      (_mode == WHAT_MODE && IsWhatModeUnlocked()) ? -1 : 0;
  _specialSelection =
      (_mode == BOSS_MODE && IsBossModeUnlocked()) ? 0 :
      (_mode == HARD_MODE && IsHardModeUnlocked()) ? 1 :
      (_mode == FAST_MODE && IsFastModeUnlocked()) ? 2 :
      (_mode == WHAT_MODE && IsWhatModeUnlocked()) ? 3 : 0;
}

void z0Game::AddShip(Ship* ship)
{
  ship->set_game(*this);
  if (ship->type() & Ship::SHIP_ENEMY) {
    _overmind->OnEnemyCreate(*ship);
  }
  _ships.emplace_back(ship);

  if (ship->bounding_width() > 1) {
    _collisions.push_back(ship);
  }
}

void z0Game::AddParticle(Particle* particle)
{
  _particles.emplace_back(particle);
}

// Ship info
//------------------------------
int z0Game::get_non_wall_count() const
{
  return _overmind->CountNonWallEnemies();
}

z0Game::ShipList z0Game::all_ships(int32_t ship_mask) const
{
  ShipList r;
  for (auto& ship : _ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

z0Game::ShipList z0Game::ships_in_radius(const vec2& point, fixed radius,
                                         int32_t ship_mask) const
{
  ShipList r;
  for (auto& ship : _ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->position() - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool z0Game::any_collision(const vec2& point, int32_t category) const
{
  fixed x = point.x;
  fixed y = point.y;

  for (unsigned int i = 0; i < _collisions.size(); i++) {
    fixed sx = _collisions[i]->position().x;
    fixed sy = _collisions[i]->position().y;
    fixed w = _collisions[i]->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (_collisions[i]->check_point(point, category)) {
      return true;
    }
  }
  return false;
}

z0Game::ShipList z0Game::collision_list(const vec2& point, int32_t category) const
{
  ShipList r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : _collisions) {
    fixed sx = collision->position().x;
    fixed sy = collision->position().y;
    fixed w = collision->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (collision->check_point(point, category)) {
      r.push_back(collision);
    }
  }
  return r;
}

int32_t z0Game::alive_players() const
{
  return count_players() - killed_players();
}

int32_t z0Game::killed_players() const
{
  return Player::CountKilledPlayers();
}

Player* z0Game::nearest_player(const vec2& point) const
{
  Ship* r = 0;
  Ship* deadR = 0;
  fixed d = Lib::WIDTH * Lib::HEIGHT;
  fixed deadD = Lib::WIDTH * Lib::HEIGHT;

  for (Ship* ship : _playerList) {
    if (!((Player*) ship)->IsKilled() &&
        (ship->position() - point).length() < d) {
      d = (ship->position() - point).length();
      r = ship;
    }
    if ((ship->position() - point).length() < deadD) {
      deadD = (ship->position() - point).length();
      deadR = ship;
    }
  }
  return (Player*) (r != 0 ? r : deadR);
}

void z0Game::set_boss_killed(boss_list boss)
{
  if (!Player::replay.recording) {
    return;
  }
  if (boss == BOSS_3A || (_mode != BOSS_MODE && _mode != NORMAL_MODE)) {
    _hardModeBossesKilled |= boss;
  }
  else {
    _bossesKilled |= boss;
  }
}

bool z0Game::SortShips(Ship* const& a, Ship* const& b)
{
  return
      a->position().x - a->bounding_width() <
      b->position().x - b->bounding_width();
}

// UI layout
//------------------------------
void z0Game::RenderPanel(const flvec2& low, const flvec2& hi) const
{
  flvec2 tlow(low.x * Lib::TEXT_WIDTH, low.y * Lib::TEXT_HEIGHT);
  flvec2 thi(hi.x * Lib::TEXT_WIDTH, hi.y * Lib::TEXT_HEIGHT);
  lib().RenderRect(tlow, thi, PANEL_BACK);
  lib().RenderRect(tlow, thi, PANEL_TEXT, 4);
}

std::string z0Game::ConvertToTime(int64_t score) const
{
  if (score == 0) {
    return "--:--";
  }
  int64_t mins = 0;
  while (score >= 60 * 60 && mins < 99) {
    score -= 60 * 60;
    ++mins;
  }
  int64_t secs = score / 60;

  std::stringstream r;
  if (mins < 10) {
    r << "0";
  }
  r << mins << ":";
  if (secs < 10) {
    r << "0";
  }
  r << secs;
  return r.str();
}

// Score calculation
//------------------------------
int64_t z0Game::GetPlayerScore(int32_t playerNumber) const
{
  for (Ship* ship : _playerList) {
    Player* p = (Player*) ship;
    if (p->GetPlayerNumber() == playerNumber) {
      return p->GetScore();
    }
  }
  return 0;
}

int64_t z0Game::GetPlayerDeaths(int32_t playerNumber) const
{
  for (Ship* ship : _playerList) {
    Player* p = (Player*) ship;
    if (p->GetPlayerNumber() == playerNumber) {
      return p->GetDeaths();
    }
  }
  return 0;
}

int64_t z0Game::GetTotalScore() const
{
  int64_t total = 0;
  for (Ship* ship : _playerList) {
    Player* p = (Player*) ship;
    total += p->GetScore();
  }
  return total;
}

bool z0Game::IsHighScore() const
{
  if (!Player::replay.recording) {
    return false;
  }

  if (_mode == BOSS_MODE) {
    return
        _overmind->GetKilledBosses() >= 6 && _overmind->GetElapsedTime() != 0 &&
        (_overmind->GetElapsedTime() <
             _highScores[Lib::PLAYERS][_players - 1].second ||
         _highScores[Lib::PLAYERS][_players - 1].second == 0);
  }

  int n =
      _mode == WHAT_MODE ? 3 * Lib::PLAYERS + _players :
      _mode == FAST_MODE ? 2 * Lib::PLAYERS + _players :
      _mode == HARD_MODE ? Lib::PLAYERS + _players : _players - 1;

  const Lib::HighScoreList& list = _highScores[n];
  for (unsigned int i = 0; i < list.size(); i++) {
    if (GetTotalScore() > list[i].second) {
      return true;
    }
  }
  return false;
}