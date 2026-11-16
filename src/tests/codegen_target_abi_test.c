#include "codegen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static vitte_codegen_target_t
make_target(
    vitte_codegen_arch_t architecture,
    vitte_codegen_os_t operating_system,
    vitte_codegen_abi_t abi,
    bool hosted)
{
    vitte_codegen_target_t target = {0};

    target.architecture = architecture;
    target.operating_system = operating_system;
    target.abi = abi;
    target.endian = VITTE_CODEGEN_ENDIAN_LITTLE;
    target.pointer_bits = 64u;
    target.size_bits = 64u;
    target.char_bits = 8u;
    target.hosted = hosted;
    return target;
}

static void
expect_valid(
    vitte_codegen_target_t target)
{
    assert(vitte_codegen_target_validate(&target));
}

static void
expect_invalid(
    vitte_codegen_target_t target)
{
    assert(!vitte_codegen_target_validate(&target));
}

int
main(void)
{
    vitte_codegen_target_t target;

    /* The enum values must stay aligned with compiler/codegen/codegen.vitte. */
    _Static_assert(VITTE_CODEGEN_ARCH_HOST == 5, "host architecture ordinal changed");
    _Static_assert(VITTE_CODEGEN_OS_HOST == 6, "host OS ordinal changed");
    _Static_assert(VITTE_CODEGEN_ABI_HOST == 6, "host ABI ordinal changed");
    _Static_assert(VITTE_CODEGEN_ABI_RISCV64 == 7, "RISC-V ABI ordinal changed");
    _Static_assert(VITTE_CODEGEN_ABI_WIN_ARM64 == 8, "Windows ARM64 ABI ordinal changed");

    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_WINDOWS) == VITTE_CODEGEN_ABI_WIN64);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_LINUX) == VITTE_CODEGEN_ABI_SYSV);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_NONE) == VITTE_CODEGEN_ABI_SYSV);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_WINDOWS) == VITTE_CODEGEN_ABI_WIN_ARM64);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_LINUX) == VITTE_CODEGEN_ABI_AAPCS64);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_NONE) == VITTE_CODEGEN_ABI_AAPCS64);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_RISCV64,
        VITTE_CODEGEN_OS_LINUX) == VITTE_CODEGEN_ABI_RISCV64);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_RISCV64,
        VITTE_CODEGEN_OS_WINDOWS) == VITTE_CODEGEN_ABI_INVALID);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_WASM32,
        VITTE_CODEGEN_OS_NONE) == VITTE_CODEGEN_ABI_WASM);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_WASM32,
        VITTE_CODEGEN_OS_WINDOWS) == VITTE_CODEGEN_ABI_INVALID);
    assert(vitte_codegen_abi_for_target(
        VITTE_CODEGEN_ARCH_HOST,
        VITTE_CODEGEN_OS_HOST) == VITTE_CODEGEN_ABI_HOST);

    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_SYSV,
        true));
    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_MACOS,
        VITTE_CODEGEN_ABI_SYSV,
        true));
    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_WINDOWS,
        VITTE_CODEGEN_ABI_WIN64,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_WINDOWS,
        VITTE_CODEGEN_ABI_SYSV,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_WIN64,
        true));

    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_AAPCS64,
        true));
    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_WINDOWS,
        VITTE_CODEGEN_ABI_WIN_ARM64,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_WINDOWS,
        VITTE_CODEGEN_ABI_AAPCS64,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_AARCH64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_WIN_ARM64,
        true));

    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_RISCV64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_RISCV64,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_RISCV64,
        VITTE_CODEGEN_OS_LINUX,
        VITTE_CODEGEN_ABI_SYSV,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_RISCV64,
        VITTE_CODEGEN_OS_WINDOWS,
        VITTE_CODEGEN_ABI_RISCV64,
        true));
    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_WASM32,
        VITTE_CODEGEN_OS_NONE,
        VITTE_CODEGEN_ABI_WASM,
        false));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_WASM32,
        VITTE_CODEGEN_OS_NONE,
        VITTE_CODEGEN_ABI_SYSV,
        false));

    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_NONE,
        VITTE_CODEGEN_ABI_FREESTANDING,
        false));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_X86_64,
        VITTE_CODEGEN_OS_NONE,
        VITTE_CODEGEN_ABI_FREESTANDING,
        true));
    expect_valid(make_target(
        VITTE_CODEGEN_ARCH_HOST,
        VITTE_CODEGEN_OS_HOST,
        VITTE_CODEGEN_ABI_HOST,
        true));
    expect_invalid(make_target(
        VITTE_CODEGEN_ARCH_HOST,
        VITTE_CODEGEN_OS_HOST,
        VITTE_CODEGEN_ABI_SYSV,
        true));

    target = vitte_codegen_target_default();
    if (target.architecture != VITTE_CODEGEN_ARCH_INVALID) {
        expect_valid(target);
#if defined(_WIN32) && (defined(__x86_64__) || defined(_M_X64))
        assert(target.abi == VITTE_CODEGEN_ABI_WIN64);
#elif defined(_WIN32) && (defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64))
        assert(target.abi == VITTE_CODEGEN_ABI_WIN_ARM64);
#elif defined(__aarch64__) || defined(__arm64__)
        assert(target.abi == VITTE_CODEGEN_ABI_AAPCS64);
#elif defined(__riscv) && (__riscv_xlen == 64)
        assert(target.abi == VITTE_CODEGEN_ABI_RISCV64);
#elif defined(__wasm32__)
        assert(target.abi == VITTE_CODEGEN_ABI_WASM);
#elif defined(__x86_64__) || defined(_M_X64)
        assert(target.abi == VITTE_CODEGEN_ABI_SYSV);
#endif
    }

    assert(strcmp(vitte_codegen_abi_name(VITTE_CODEGEN_ABI_RISCV64), "riscv64") == 0);
    assert(strcmp(vitte_codegen_abi_name(VITTE_CODEGEN_ABI_WIN_ARM64), "win-arm64") == 0);
    puts("target ABI validation passed");
    return 0;
}
