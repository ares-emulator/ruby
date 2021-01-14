auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.wram = parent->append<Node::Debugger::Memory>("CPU WRAM");
  memory.wram->setSize(128_KiB);
  memory.wram->setRead([&](u32 address) -> u8 {
    return cpu.wram[uint17(address)];
  });
  memory.wram->setWrite([&](u32 address, u8 data) -> void {
    cpu.wram[uint17(address)] = data;
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");

  tracer.dma = parent->append<Node::Debugger::Tracer::Notification>("DMA", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(tracer.instruction->enabled() && tracer.instruction->address(cpu.r.pc.d)) {
    tracer.instruction->notify(cpu.disassembleInstruction(), cpu.disassembleContext(), {
      "V:", pad(cpu.vcounter(), 3L), " ", "H:", pad(cpu.hcounter(), 4L), " I:", (uint)cpu.field()
    });
  }
}

auto CPU::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}

auto CPU::Debugger::dma(uint8 channelID, uint24 addressA, uint8 addressB, uint8 data) -> void {
  if(tracer.dma->enabled()) {
    string output;
    output.append("Channel ", channelID, ": ");
    if(cpu.channels[channelID].direction == 0) {
      output.append("$", hex(addressA, 6L), " ");
      output.append("($", hex(data, 2L), ") => ");
      output.append("$21", hex(addressB, 2L));
    } else {
      output.append("$21", hex(addressB, 2L), " ");
      output.append("($", hex(data, 2L), ") => ");
      output.append("$", hex(addressA, 6L));
    }
    tracer.dma->notify(output);
  }
}
