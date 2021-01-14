#pragma once

struct InputJoypadSDL {
  Input& input;
  InputJoypadSDL(Input& input) : input(input) {}

  struct Joypad {
    shared_pointer<HID::Joypad> hid{new HID::Joypad};
    vector<bool> axisPolled;

    u32 id = 0;
    SDL_Joystick* handle = nullptr;
  };
  vector<Joypad> joypads;

  auto assign(Joypad& joypad, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = joypad.hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    if(groupID == HID::Joypad::GroupID::Axis && !joypad.axisPolled(inputID)) {
      //suppress the first axis polling event, because the value can change dramatically.
      //SDL seems to return 0 for all axes, until the first movement, where it jumps to the real value.
      //this triggers onChange handlers to instantly bind inputs erroneously if not suppressed.
      joypad.axisPolled[inputID] = true;
    } else {
      input.doChange(joypad.hid, groupID, inputID, group.input(inputID).value(), value);
    }
    group.input(inputID).setValue(value);
  }

  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {
    SDL_JoystickUpdate();
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED) {
        enumerate();
      }
    }

    for(auto& jp : joypads) {
      for(u32 n : range(jp.hid->axes().size())) {
        assign(jp, HID::Joypad::GroupID::Axis, n, (s16)SDL_JoystickGetAxis(jp.handle, n));
      }

      for(s32 n = 0; n < (s32)jp.hid->hats().size() - 1; n += 2) {
        u8 state = SDL_JoystickGetHat(jp.handle, n >> 1);
        assign(jp, HID::Joypad::GroupID::Hat, n + 0, state & SDL_HAT_LEFT ? -32767 : state & SDL_HAT_RIGHT ? +32767 : 0);
        assign(jp, HID::Joypad::GroupID::Hat, n + 1, state & SDL_HAT_UP   ? -32767 : state & SDL_HAT_DOWN  ? +32767 : 0);
      }

      for(u32 n : range(jp.hid->buttons().size())) {
        assign(jp, HID::Joypad::GroupID::Button, n, (bool)SDL_JoystickGetButton(jp.handle, n));
      }

      devices.append(jp.hid);
    }
  }

  auto initialize() -> bool {
    terminate();
    SDL_Init(SDL_INIT_EVENTS);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
    enumerate();
    return true;
  }

  auto terminate() -> void {
    for(auto& jp : joypads) {
      SDL_JoystickClose(jp.handle);
    }
    joypads.reset();
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  }

private:
  auto enumerate() -> void {
    for(auto& joypad : joypads) {
      SDL_JoystickClose(joypad.handle);
    }
    joypads.reset();

    for(u32 id : range(SDL_NumJoysticks())) {
      Joypad jp;
      jp.id = id;
      jp.handle = SDL_JoystickOpen(jp.id);

      u32 axes = SDL_JoystickNumAxes(jp.handle);
      u32 hats = SDL_JoystickNumHats(jp.handle) * 2;
      u32 buttons = SDL_JoystickNumButtons(jp.handle);

      jp.hid->setVendorID(HID::Joypad::GenericVendorID);
      jp.hid->setProductID(HID::Joypad::GenericProductID);
      jp.hid->setPathID(jp.id);
      for(u32 n : range(axes)) jp.hid->axes().append(n);
      for(u32 n : range(hats)) jp.hid->hats().append(n);
      for(u32 n : range(buttons)) jp.hid->buttons().append(n);
      jp.hid->setRumble(false);

      joypads.append(jp);
    }

    SDL_JoystickUpdate();
  }
};
