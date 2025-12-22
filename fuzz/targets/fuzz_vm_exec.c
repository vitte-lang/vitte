// fuzz_vm_exec.c
// Fuzz target: VM exec placeholder (no real VM wired here).

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"

typedef struct {
  uint32_t regs[8];
  uint32_t pc;
} vm_state;

static void vm_step(vm_state* st, uint8_t op, uint32_t imm) {
  uint32_t r = op & 7u;
  switch (op >> 3) {
    case 0:
      st->regs[r] += imm;
      break;
    case 1:
      st->regs[r] ^= imm;
      break;
    case 2:
      st->regs[r] = (uint32_t)((uint64_t)st->regs[r] * (uint64_t)(imm | 1u));
      break;
    case 3:
      st->regs[r] = imm;
      break;
    case 4:
      st->pc += (imm & 0xFu);
      break;
    default:
      break;
  }
}

FUZZ_TARGET(fuzz_vm_exec_target) {
  fuzz_reader r = fuzz_reader_from(data, size);
  uint8_t mode = 0;
  (void)fuzz_reader_read_u8(&r, &mode);

  vm_state st;
  for (size_t i = 0; i < 8; ++i)
    st.regs[i] = (uint32_t)(i * 0x9E3779B9u);
  st.pc = 0;

  uint64_t steps = 0;
  if (!fuzz_reader_read_uvar(&r, &steps, 4))
    steps = 64;
  if (steps > 100000)
    steps = 100000;

  for (uint64_t i = 0; i < steps && !fuzz_reader_eof(&r); ++i) {
    uint8_t op = 0;
    uint64_t imm64 = 0;
    if (!fuzz_reader_read_u8(&r, &op))
      break;
    if (!fuzz_reader_read_uvar(&r, &imm64, 5))
      break;
    uint32_t imm = (uint32_t)imm64;
    if ((uint64_t)imm != imm64)
      break;
    vm_step(&st, op, imm);
    st.pc++;
    if ((mode & 1u) && (op == 0xFFu))
      break;
  }

  FUZZ_TRACE_TAG_U32("vmexec.r0", st.regs[0]);
  FUZZ_TRACE_TAG_U32("vmexec.r1", st.regs[1]);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_vm_exec_target
#include "fuzz/fuzz_driver.h"
