#ifndef IISPACE_GAME_CORE_MODAL_H
#define IISPACE_GAME_CORE_MODAL_H
#include <memory>
#include <vector>

class Lib;
class ModalStack;
class Modal {
public:
  Modal(bool capture_updates, bool capture_rendering)
  : capture_updates_{capture_updates}, capture_rendering_{capture_rendering} {}
  virtual ~Modal() {}

  virtual void update(Lib& lib) = 0;
  virtual void render(Lib& lib) const = 0;

protected:
  void add(std::unique_ptr<Modal> modal);
  void quit();

private:
  friend class ModalStack;
  bool capture_updates_ = false;
  bool capture_rendering_ = false;
  bool quit_ = false;
  ModalStack* stack_ = nullptr;
};

class ModalStack {
public:
  void add(std::unique_ptr<Modal> modal);

  // Returns true if any modal captured the update chain.
  bool update(Lib& lib);

  // Returns true if any modal captured the render chain.
  bool render(Lib& lib) const;

private:
  std::vector<std::unique_ptr<Modal>> stack_;
  std::vector<std::unique_ptr<Modal>> new_stack_;
};

#endif