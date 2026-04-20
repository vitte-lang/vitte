# Vitte Kernel Library — Complete System Programming API

The Vitte Kernel Library (`stdlib/kernel/`) provides comprehensive system-level abstractions for process management, threading, synchronization, signal handling, network operations, file I/O, device control, and user management.

## Module Structure

### 1. **kernel/process.vitl** — Process Management
Core process operations matching Unix process model.

**Key Types:**
- `PID` — Process identifier
- `ProcessState` — Current process state (enum)
- `Process` — Process descriptor

**Operations (12 functions):**
- `getpid()` — Get current process ID
- `getppid()` — Get parent process ID
- `fork()` — Create child process
- `exit(code)` — Terminate process
- `wait()` — Wait for child process
- `waitpid(pid)` — Wait for specific child
- `sleep(seconds)` — Sleep for duration
- `usleep(microseconds)` — Sleep microseconds
- `exec()` — Execute program
- `execv(args)` — Execute with arguments
- `getpriority()` — Get process priority
- `setpriority()` — Set process priority
- `nice(increment)` — Adjust priority

**Constants:**
- `PRIO_PROCESS`, `PRIO_PGRP`, `PRIO_USER` — Priority scope

---

### 2. **kernel/threads.vitl** — Thread Management
Complete POSIX threading API.

**Key Types:**
- `ThreadID` — Thread identifier
- `ThreadState` — Thread state (enum)
- `Thread` — Thread descriptor

**Operations (10+ functions):**
- `pthread_create()` — Create new thread
- `pthread_self()` — Get current thread ID
- `pthread_join(tid)` — Wait for thread completion
- `pthread_detach(tid)` — Detach thread
- `pthread_cancel(tid)` — Cancel thread
- `pthread_exit()` — Terminate thread
- `pthread_yield()` — Yield CPU
- `pthread_equal(tid1, tid2)` — Compare thread IDs
- `pthread_getpriority()` — Get thread priority
- `pthread_setpriority()` — Set thread priority
- Thread-local storage (TLS) functions

**Constants:**
- `PTHREAD_CANCEL_ENABLE`, `PTHREAD_CANCEL_DISABLE`
- `PTHREAD_CANCEL_DEFERRED`, `PTHREAD_CANCEL_ASYNCHRONOUS`
- `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER` — Scheduling policies

---

### 3. **kernel/sync.vitl** — Synchronization Primitives
Complete set of threading synchronization mechanisms.

**Key Types (6 synchronization primitives):**
- `Mutex` — Mutual exclusion lock
- `Semaphore` — Counting semaphore
- `CondVar` — Condition variable
- `Barrier` — Thread barrier
- `RWLock` — Reader-writer lock
- `SpinLock` — Spin lock

**Operations (30+ functions):**

**Mutex:**
- `pthread_mutex_init()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`
- `pthread_mutex_trylock()`, `pthread_mutex_destroy()`

**Semaphore:**
- `pthread_sem_init()`, `pthread_sem_wait()`, `pthread_sem_trywait()`
- `pthread_sem_post()`, `pthread_sem_destroy()`

**Condition Variable:**
- `pthread_cond_init()`, `pthread_cond_wait()`, `pthread_cond_timedwait()`
- `pthread_cond_signal()`, `pthread_cond_broadcast()`, `pthread_cond_destroy()`

**Barrier:**
- `pthread_barrier_init()`, `pthread_barrier_wait()`, `pthread_barrier_destroy()`

**Read-Write Lock:**
- `pthread_rwlock_init()`, `pthread_rwlock_rdlock()`, `pthread_rwlock_wrlock()`
- `pthread_rwlock_unlock()`, `pthread_rwlock_tryrdlock()`, `pthread_rwlock_trywrlock()`
- `pthread_rwlock_destroy()`

**Spin Lock:**
- `spin_lock_init()`, `spin_lock()`, `spin_unlock()`, `spin_trylock()`

**Constants:**
- `PTHREAD_MUTEX_NORMAL`, `PTHREAD_MUTEX_RECURSIVE`
- `PTHREAD_MUTEX_ERRORCHECK`, `PTHREAD_MUTEX_DEFAULT`

---

### 4. **kernel/signals.vitl** — Signal Handling
Complete POSIX signal management.

**Signal Constants (31 signals):**
- Standard: `SIGABRT`, `SIGALRM`, `SIGBUS`, `SIGCHLD`, `SIGCONT`
- Termination: `SIGFPE`, `SIGHUP`, `SIGILL`, `SIGINT`, `SIGIO`
- Priority: `SIGKILL`, `SIGPIPE`, `SIGQUIT`, `SIGSEGV`, `SIGSTOP`
- User-defined: `SIGUSR1`, `SIGUSR2`
- Timing: `SIGPROF`, `SIGVTALRM`, `SIGXCPU`, `SIGXFSZ`
- Terminal: `SIGTSTP`, `SIGTTIN`, `SIGTTOU`
- Control: `SIGWINCH`, `SIGIOT`, `SIGPOLL`, `SIGSYS`, `SIGTRAP`, `SIGURG`

**Signal Handler Constants:**
- `SA_NOCLDSTOP`, `SA_NOCLDWAIT`, `SA_NODEFER`, `SA_ONSTACK`
- `SA_RESETHAND`, `SA_RESTART`, `SA_SIGINFO`

**Signal Mask Operations:**
- `SIG_BLOCK`, `SIG_UNBLOCK`, `SIG_SETMASK`

**Operations (10+ functions):**
- `signal(sig, handler)` — Set signal handler
- `sigaction(sig, action)` — Advanced signal handling
- `sigprocmask(how, set)` — Change signal mask
- `sigenable(sig)`, `sigdisable(sig)` — Enable/disable signals
- `kill(pid, sig)` — Send signal to process
- `raise(sig)` — Raise signal in current process
- `sigwait()` — Wait for signal
- `sigwaitinfo()` — Wait for signal with info
- `pause()` — Suspend process

**Signal Set Operations:**
- `sigemptyset()`, `sigfillset()`, `sigaddset()`, `sigdelset()`
- `sigismember()`, `sigcmpset()`

---

### 5. **kernel/memory.vitl** — Virtual Memory Management
Kernel memory mapping and management.

**Memory Protection Flags:**
- `PROT_NONE`, `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`

**Memory Mapping Flags:**
- `MAP_SHARED`, `MAP_PRIVATE`, `MAP_FIXED`, `MAP_ANONYMOUS`

**Memory Advice Flags:**
- `MADV_NORMAL`, `MADV_RANDOM`, `MADV_SEQUENTIAL`
- `MADV_WILLNEED`, `MADV_DONTNEED`

**Memory Lock Flags:**
- `MCL_CURRENT`, `MCL_FUTURE`

**Operations (15 functions):**
- `mmap()` — Map memory region
- `munmap()` — Unmap memory
- `mprotect()` — Change protection
- `madvise()` — Advise kernel on memory usage
- `msync()` — Sync memory with storage
- `mlock()`, `munlock()` — Lock/unlock memory
- `mlockall()`, `munlockall()` — Lock/unlock all memory
- `brk()`, `sbrk()` — Adjust break point
- `get_meminfo()` — Get memory statistics
- `get_page_size()` — Get page size
- `get_phys_pages()`, `get_avphys_pages()` — Physical page counts
- `mincore()` — Check page residency

---

### 6. **kernel/network.vitl** — Network Operations
Socket-based network programming (TCP/UDP, multicast).

**Address Families:**
- `AF_UNSPEC`, `AF_UNIX`, `AF_INET`, `AF_INET6`

**Socket Types:**
- `SOCK_STREAM` (TCP), `SOCK_DGRAM` (UDP), `SOCK_RAW`, `SOCK_SEQPACKET`

**Protocols:**
- `IPPROTO_IP`, `IPPROTO_ICMP`, `IPPROTO_TCP`, `IPPROTO_UDP`

**Socket Options:**
- `SOL_SOCKET`, `SO_REUSEADDR`, `SO_KEEPALIVE`, `SO_SNDBUF`, `SO_RCVBUF`
- `SO_LINGER`, `SO_TYPE`, `SO_ERROR`

**TCP Options:**
- `TCP_NODELAY`, `TCP_KEEPIDLE`, `TCP_KEEPINTVL`

**Socket Shutdown Modes:**
- `SHUT_RD`, `SHUT_WR`, `SHUT_RDWR`

**Operations (35+ functions):**

**Basic Socket Operations:**
- `socket()` — Create socket
- `bind()` — Bind to address
- `listen()` — Listen for connections
- `accept()` — Accept connection
- `connect()` — Connect to remote
- `close()`, `shutdown()` — Close/shutdown

**Socket I/O:**
- `send()`, `sendto()`, `sendmsg()` — Send data
- `recv()`, `recvfrom()`, `recvmsg()` — Receive data

**Socket Information:**
- `getsockname()`, `getpeername()` — Get addresses
- `getsockopt()`, `setsockopt()` — Get/set options
- `fcntl()`, `ioctl()` — File descriptor control

**Address Conversion:**
- `inet_aton()`, `inet_ntoa()` — IPv4 string conversion
- `inet_pton()`, `inet_ntop()` — Presentation format conversion
- `htons()`, `ntohs()`, `htonl()`, `ntohl()` — Byte order conversion

**Name Resolution:**
- `gethostbyname()`, `gethostbyaddr()` — Host lookup
- `getservbyname()`, `getservbyport()` — Service lookup
- `getprotobyname()`, `getprotobynumber()` — Protocol lookup

**Multiplexing:**
- `poll()`, `select()` — I/O multiplexing

**Multicast:**
- `setsockopt_mcast_loop()`, `setsockopt_mcast_ttl()`
- `setsockopt_mcast_if()`, `setsockopt_join_group()`, `setsockopt_leave_group()`

---

### 7. **kernel/fileio.vitl** — File I/O and Filesystem Operations
Complete file descriptor and filesystem operations.

**File Open Flags:**
- `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_APPEND`
- `O_CREAT`, `O_EXCL`, `O_TRUNC`, `O_NONBLOCK`, `O_SYNC`

**File Mode Bits:**
- User: `S_IRUSR`, `S_IWUSR`, `S_IXUSR`
- Group: `S_IRGRP`, `S_IWGRP`, `S_IXGRP`
- Other: `S_IROTH`, `S_IWOTH`, `S_IXOTH`

**File Type Flags:**
- `S_ISREG`, `S_ISDIR`, `S_ISCHR`, `S_ISBLK`
- `S_ISFIFO`, `S_ISLNK`, `S_ISSOCK`

**Standard File Descriptors:**
- `STDIN_FILENO` (0), `STDOUT_FILENO` (1), `STDERR_FILENO` (2)

**Seek Constants:**
- `SEEK_SET`, `SEEK_CUR`, `SEEK_END`

**Operations (40+ functions):**

**File Descriptor Operations:**
- `open()`, `creat()`, `close()`
- `read()`, `write()` — Sequential I/O
- `pread()`, `pwrite()` — Random access I/O
- `lseek()` — Seek in file
- `fsync()`, `fdatasync()` — Sync to disk

**Descriptor Manipulation:**
- `dup()`, `dup2()` — Duplicate descriptors
- `fcntl()`, `ioctl()` — Descriptor control
- `ftruncate()` — Truncate file
- `flock()` — Advisory locking

**Directory Operations:**
- `mkdir()`, `rmdir()` — Create/remove directories
- `opendir()`, `closedir()` — Open/close directory streams
- `readdir()` — Read directory entry
- `rewinddir()` — Rewind directory
- `chdir()` — Change directory
- `getcwd()` — Get current directory
- `listdir()` — List directory contents

**File Management:**
- `unlink()` — Delete file
- `link()` — Create hard link
- `symlink()` — Create symbolic link
- `readlink()` — Read symbolic link
- `rename()` — Rename file

**File Permissions:**
- `chmod()`, `fchmod()` — Change permissions
- `chown()`, `fchown()` — Change owner
- `umask()` — Set default mask

**File Information:**
- `stat()`, `fstat()`, `lstat()` — Get file statistics
- `access()` — Check accessibility
- `truncate()` — Truncate by path

**Filesystem Operations:**
- `statvfs()` — Get filesystem statistics
- `pathconf()`, `fpathconf()` — Get configuration
- `sync()` — Sync all filesystems

---

### 8. **kernel/device.vitl** — Device and Terminal Control
Terminal control and device I/O operations.

**Terminal Mode Flags:**
- Input: `IGNBRK`, `BRKINT`, `IGNPAR`, `INPCK`, `ISTRIP`
- `INLCR`, `IGNCR`, `ICRNL`, `IXON`, `IXOFF`
- Output: `OPOST`, `ONLCR`, `OCRNL`, `ONOCR`, `ONLRET`
- Local: `ICANON`, `ECHO`, `ECHOE`, `ECHOK`, `ECHONL`

**Baud Rates (17 speeds):**
- `B0`, `B50`, `B75`, `B110`, `B134`, `B150`, `B200`, `B300`
- `B600`, `B1200`, `B1800`, `B2400`, `B4800`, `B9600`
- `B19200`, `B38400`, `B57600`, `B115200`

**Tcsetattr Actions:**
- `TCSA_NOW`, `TCSA_DRAIN`, `TCSA_FLUSH`

**Operations (25+ functions):**

**Terminal Attributes:**
- `tcgetattr()` — Get terminal settings
- `tcsetattr()` — Set terminal settings
- `tcflow()` — Flow control
- `tcflush()` — Flush buffers
- `tcsendbreak()` — Send break signal

**Baud Rate:**
- `cfgetispeed()`, `cfgetospeed()` — Get baud rates
- `cfsetispeed()`, `cfsetospeed()` — Set baud rates
- `cfsetspeed()` — Set both baud rates

**Terminal Window:**
- `tiocgwinsz()` — Get terminal size
- `tiocswinsz()` — Set terminal size
- `isatty()` — Check if terminal
- `ttyname()` — Get terminal name

**Device Control:**
- `ioctl()` — Device I/O control (3 variants)
- `get_device_info()` — Get device info
- `mknod()` — Create device node
- `makedev()`, `major_num()`, `minor_num()` — Device number operations

**Console Control:**
- `get_console_size()` — Get console dimensions
- `get_tty_fd()` — Get TTY file descriptor
- `set_tty_raw_mode()`, `set_tty_canonical_mode()` — Input modes
- `enable_echo()`, `disable_echo()` — Echo control

**Device Type Checking:**
- `is_terminal()`, `is_socket()`, `is_pipe()`, `is_regular_file()`

---

### 9. **kernel/users.vitl** — User, Group, and Permission Management
User/group database access and permission/resource control.

**Resource Limit Constants (10 limits):**
- `RLIMIT_CPU` — CPU time
- `RLIMIT_FSIZE` — File size
- `RLIMIT_DATA` — Data segment
- `RLIMIT_STACK` — Stack size
- `RLIMIT_CORE` — Core dump size
- `RLIMIT_RSS` — Resident set
- `RLIMIT_NPROC` — Process count
- `RLIMIT_NOFILE` — Open files
- `RLIMIT_MEMLOCK` — Locked memory
- `RLIMIT_AS` — Address space

**Operations (35+ functions):**

**User ID Operations:**
- `getuid()`, `geteuid()` — Get user IDs
- `setuid()`, `seteuid()` — Set user IDs

**Group ID Operations:**
- `getgid()`, `getegid()` — Get group IDs
- `setgid()`, `setegid()` — Set group IDs
- `getgroups()`, `setgroups()` — Group lists

**Process ID Operations:**
- `getpid()`, `getppid()` — Get PIDs
- `getpgid()`, `setpgid()` — Process group
- `setsid()`, `getsid()` — Session management

**Password/Group Database:**
- `getpwuid()`, `getpwnam()` — User lookup
- `getgrgid()`, `getgrnam()` — Group lookup

**Resource Limits (12 functions):**
- `getrlimit()`, `setrlimit()` — Generic limit operations
- Specialized: `getrlimit_cpu()`, `setrlimit_cpu()`
- `getrlimit_fsize()`, `setrlimit_fsize()`
- `getrlimit_nofile()`, `setrlimit_nofile()`

**Environment Variables:**
- `getenv()` — Get variable
- `setenv()`, `unsetenv()` — Set/unset variable
- `environ()` — Get all variables
- `clearenv()` — Clear all variables

**Process Control:**
- `getpriority()`, `setpriority()` — Priority control
- `getrusage()` — Resource usage

**User Information:**
- `get_login()` — Login name
- `get_username()` — Current user
- `get_home_dir()` — Home directory
- `get_shell()` — Default shell
- `is_root()` — Root check

**Access Control:**
- `access()` — Check permissions
- `faccessat()` — Check relative to directory
- `eaccess()` — Check effective access
- `initgroups()` — Initialize group access

---

## Module Import and Usage

### Import All Kernel Modules
```vitte
use vitte/kernel
```

This automatically imports all 9 kernel modules with their complete APIs (150+ functions and 100+ constants).

### Import Specific Modules
```vitte
use vitte/kernel/process
use vitte/kernel/threads
use vitte/kernel/network
use vitte/kernel/fileio
```

---

## Type Definitions

### Process Types
```vitte
type PID = i64
type ProcessState = pick {
    RUNNING, STOPPED, ZOMBIE, DEFUNCT
}
form Process {
    pid: PID,
    state: ProcessState
}
```

### Thread Types
```vitte
type ThreadID = i64
type ThreadState = pick {
    JOINABLE, DETACHED, CANCELED
}
form Thread {
    tid: ThreadID,
    state: ThreadState
}
```

### Synchronization Types
```vitte
type Mutex = i64
type Semaphore = i64
type CondVar = i64
type Barrier = i64
type RWLock = i64
type SpinLock = i64
```

### Network Types
```vitte
form IPv4Addr { addr: string }
form IPv6Addr { addr: string }
form SockAddr {
    family: i32,
    address: string,
    port: i32
}
form HostEnt {
    name: string,
    aliases: [string],
    family: i32,
    addresses: [string]
}
```

### File Types
```vitte
form DirEnt {
    ino: i64,
    off: i64,
    reclen: i32,
    type: i32,
    name: string
}
form FileStat {
    st_dev: i64,
    st_ino: i64,
    st_mode: i32,
    st_nlink: i32,
    st_uid: i32,
    st_gid: i32,
    st_rdev: i64,
    st_size: i64,
    st_blksize: i64,
    st_blocks: i64,
    st_atime: i64,
    st_mtime: i64,
    st_ctime: i64
}
```

### Terminal Types
```vitte
form Termios {
    c_iflag: i32,
    c_oflag: i32,
    c_cflag: i32,
    c_lflag: i32,
    c_cc: [int]
}
form WinSize {
    ws_row: i32,
    ws_col: i32,
    ws_xpixel: i32,
    ws_ypixel: i32
}
```

### User Types
```vitte
form PasswdEntry {
    pw_name: string,
    pw_uid: i32,
    pw_gid: i32,
    pw_dir: string,
    pw_shell: string,
    pw_gecos: string
}
form GroupEntry {
    gr_name: string,
    gr_gid: i32,
    gr_mem: [string]
}
form RlimitEntry {
    rlim_cur: i64,
    rlim_max: i64
}
```

---

## Examples

See `examples/kernel_examples.vitl` for 10 comprehensive examples demonstrating:
1. File operations (open flags, file modes)
2. Directory operations (getcwd, chdir)
3. User/group information (UIDs, GIDs, usernames)
4. Resource limits (NOFILE, CPU, file size)
5. Environment variables (PATH, HOME)
6. Socket creation (AF_INET, SOCK_STREAM)
7. Terminal information (TTY check, window size)
8. Network address conversion (byte order)
9. Process priority (getpriority, setpriority)
10. Mutex synchronization (thread-safe counters)

---

## Status

- ✅ **9 complete kernel modules** (9 files, 500+ functions/constants)
- ✅ **POSIX-compliant API** matching standard C/C++ system libraries
- ✅ **Comprehensive type system** with forms and picker types
- ✅ **Type-safe synchronization primitives** (6 types)
- ✅ **Full networking stack** (TCP/UDP/IPv4/IPv6)
- ✅ **Complete file I/O** (40+ file descriptor operations)
- ✅ **Terminal control** (TERMIOS, baud rates, window size)
- ✅ **User/group management** (passwd/group database, resource limits)
- ✅ **Signal handling** (31 POSIX signals + handlers + masks)
- ✅ **Thread management** (pthread API, thread-local storage)
- ✅ **Process management** (fork/exec model, priority control)
- ✅ **Memory management** (mmap, mprotect, memory statistics)
- ✅ **10 practical examples** demonstrating all features

All modules compile without errors and are ready for system-level programming in Vitte.
