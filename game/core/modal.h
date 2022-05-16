#ifndef IISPACE_GAME_CORE_MODAL_H
#define IISPACE_GAME_CORE_MODAL_H
#include <memory>
#include <vector>

class Lib;
class ModalStack;
class Modal {
public:
  Modal(bool capture_updates, bool capture_rendering)
  : _capture_updates(capture_updates), _capture_rendering(capture_rendering), _quit(false) {}
  virtual ~Modal() {}

  virtual void update(Lib& lib) = 0;
  virtual void render(Lib& lib) const = 0;

protected:
  void add(Modal* modal);
  void quit();

private:
  friend class ModalStack;
  bool _capture_updates;
  bool _capture_rendering;
  bool _quit;
  ModalStack* _stack;
};

class ModalStack {
public:
  void add(Modal* modal);

  // Returns true if any modal captured the update chain.
  bool update(Lib& lib);

  // Returns true if any modal captured the render chain.
  bool render(Lib& lib) const;

private:
  std::vector<std::unique_ptr<Modal>> _stack;
  std::vector<std::unique_ptr<Modal>> _new_stack;
};

#endif