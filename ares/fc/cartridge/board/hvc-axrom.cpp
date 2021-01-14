struct HVC_AxROM : Interface {
  static auto create(string id) -> Interface* {
    if(id == "HVC-AMROM" ) return new HVC_AxROM(Revision::AMROM);
    if(id == "HVC-ANROM" ) return new HVC_AxROM(Revision::ANROM);
    if(id == "HVC-AN1ROM") return new HVC_AxROM(Revision::AN1ROM);
    if(id == "HVC-AOROM" ) return new HVC_AxROM(Revision::AOROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;

  enum class Revision : u32 {
    AMROM,
    ANROM,
    AN1ROM,
    AOROM,
  } revision;

  HVC_AxROM(Revision revision) : revision(revision) {}

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data;
    return programROM.read(programBank << 15 | (n15)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    programBank = data.bit(0,3);
    mirror = data.bit(4);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return ppu.readCIRAM(mirror << 10 | (n10)address);
    if(characterROM) return characterROM.read(address);
    if(characterRAM) return characterRAM.read(address);
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return ppu.writeCIRAM(mirror << 10 | (n10)address, data);
    if(characterRAM) return characterRAM.write(address, data);
  }

  auto power() -> void {
    programBank = 0x0f;
  }

  auto serialize(serializer& s) -> void {
    s(characterRAM);
    s(programBank);
    s(mirror);
  }

  n4 programBank;
  n1 mirror;
};
