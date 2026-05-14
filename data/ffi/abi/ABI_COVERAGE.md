# ABI Coverage

| Target | Convention | Arg Registers | Return | Preserved | Ptr | Align(ptr/float) |
|---|---|---|---|---|---:|---|
| linux-x86_64 | sysv_x64 | rdi|rsi|rdx|rcx|r8|r9 | rax | rbx|rbp|r12|r13|r14|r15 | 8 | 8/8 |
| linux-arm64 | aapcs_arm64 | x0|x1|x2|x3|x4|x5|x6|x7 | x0 | x19|x20|x21|x22|x23|x24|x25|x26|x27|x28 | 8 | 8/8 |
| linux-riscv64 | riscv64_lp64d | a0|a1|a2|a3|a4|a5|a6|a7 | a0 | s0|s1|s2|s3|s4|s5|s6|s7|s8|s9|s10|s11 | 8 | 8/8 |
| macos-x86_64 | sysv_x64 | rdi|rsi|rdx|rcx|r8|r9 | rax | rbx|rbp|r12|r13|r14|r15 | 8 | 8/8 |
| macos-arm64 | aapcs_arm64 | x0|x1|x2|x3|x4|x5|x6|x7 | x0 | x19|x20|x21|x22|x23|x24|x25|x26|x27|x28 | 8 | 8/8 |
| windows-x86_64 | ms_x64 | rcx|rdx|r8|r9 | rax | rbx|rbp|rsi|rdi|r12|r13|r14|r15 | 8 | 8/8 |

Total targets: 6
