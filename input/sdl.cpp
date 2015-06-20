#include <SDL/SDL.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "keyboard/xlib.cpp"
#include "mouse/xlib.cpp"
#include "joypad/sdl.cpp"

struct InputSDL : Input {
  InputKeyboardXlib xlibKeyboard;
  InputMouseXlib xlibMouse;
  InputJoypadSDL sdl;
  InputSDL() : xlibKeyboard(*this), xlibMouse(*this), sdl(*this) {}
  ~InputSDL() { term(); }

  struct Settings {
    uintptr_t handle = 0;
  } settings;

  auto cap(const string& name) -> bool {
    if(name == Input::Handle) return true;
    if(name == Input::KeyboardSupport) return true;
    if(name == Input::MouseSupport) return true;
    if(name == Input::JoypadSupport) return true;
    return false;
  }

  auto get(const string& name) -> any {
    if(name == Input::Handle) return (uintptr_t)settings.handle;
    return {};
  }

  auto set(const string& name, const any& value) -> bool {
    if(name == Input::Handle && value.is<uintptr_t>()) {
      settings.handle = value.get<uintptr_t>();
      return true;
    }

    return false;
  }

  auto acquire() -> bool {
    return xlibMouse.acquire();
  }

  auto release() -> bool {
    return xlibMouse.release();
  }

  auto acquired() -> bool {
    return xlibMouse.acquired();
  }

  auto poll() -> vector<shared_pointer<HID::Device>> {
    vector<shared_pointer<HID::Device>> devices;
    xlibKeyboard.poll(devices);
    xlibMouse.poll(devices);
    sdl.poll(devices);
    return devices;
  }

  auto rumble(uint64_t id, bool enable) -> bool {
    return false;
  }

  auto init() -> bool {
    if(!xlibKeyboard.init()) return false;
    if(!xlibMouse.init(settings.handle)) return false;
    if(!sdl.init()) return false;
    return true;
  }

  auto term() -> void {
    xlibKeyboard.term();
    xlibMouse.term();
    sdl.term();
  }
};
