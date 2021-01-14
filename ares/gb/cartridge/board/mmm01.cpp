struct MMM01 : Interface {
  using Interface::Interface;
  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
    Interface::load(ram, board["memory(type=RAM,content=Save)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(ram, board["memory(type=RAM,content=Save)"]);
  }

  auto unload() -> void override {
  }

  auto read(uint16 address, uint8 data) -> uint8 override {
    if(io.mode == 0) {
      if(address >= 0x0000 && address <= 0x7fff) {
        return rom.read(rom.size() - 0x8000 + (uint15)address);
      }

      return data;
    }

    if(io.mode == 1) {
      if(address >= 0x0000 && address <= 0x3fff) {
        return rom.read((io.rom.base << 14) + (uint14)address);
      }

      if(address >= 0x4000 && address <= 0x7fff) {
        return rom.read((io.rom.base << 14) + (io.rom.bank << 14) + (uint14)address);
      }

      if(address >= 0xa000 && address <= 0xbfff) {
        if(!ram || !io.ram.enable) return 0xff;
        return ram.read(io.ram.bank << 13 | (uint13)address);
      }

      return data;
    }

    return data;
  }

  auto write(uint16 address, uint8 data) -> void override {
    if(io.mode == 0) {
      if(address >= 0x0000 && address <= 0x1fff) {
        io.mode = 1;
      }

      if(address >= 0x2000 && address <= 0x3fff) {
        io.rom.base = data.bit(0,5);
      }

      return;
    }

    if(io.mode == 1) {
      if(address >= 0x0000 && address <= 0x1fff) {
        io.ram.enable = data.bit(0,3) == 0x0a;
      }

      if(address >= 0x2000 && address <= 0x3fff) {
        io.rom.bank = data;
      }

      if(address >= 0x4000 && address <= 0x5fff) {
        io.ram.bank = data;
      }

      if(address >= 0xa000 && address <= 0xbfff) {
        if(!ram || !io.ram.enable) return;
        ram.write(io.ram.bank << 13 | (uint13)address, data);
      }

      return;
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.mode);
    s(io.rom.base);
    s(io.rom.bank);
    s(io.ram.enable);
    s(io.ram.bank);
  }

  struct IO {
    uint1 mode;
    struct ROM {
      uint6 base;
      uint8 bank = 0x01;
    } rom;
    struct RAM {
      uint1 enable;
      uint8 bank;
    } ram;
  } io;
};
