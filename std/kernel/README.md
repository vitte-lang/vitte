# Kernel Standard Library

The kernel standard library provides core system functionality for bare-metal and embedded environments.

## Modules

### Core Module (`kernel`)
Main kernel initialization and system utilities.

**Key Functions:**
- `init()` - Initialize kernel
- `start_scheduler()` - Start task scheduler
- `print(message)` - Print to console
- `printf(format)` - Formatted output
- `uptime_ms()` - Get kernel uptime
- `version()` - Get kernel version
- `panic(message)` - Panic and stop
- `assert(condition, message)` - Assert condition

### Task Module (`kernel.task`)
Task scheduling and process management.

**Key Types:**
- `Priority` - Task priority levels
- `TaskState` - Task state enumeration
- `TaskStatus` - Result type
- `TaskId` - Task identifier

**Key Functions:**
- `create(entry, priority)` - Create new task
- `yield()` - Yield to scheduler
- `delay(ms)` - Delay task
- `current_task()` - Get current task ID
- `terminate(task_id)` - Terminate task
- `set_priority(task_id, priority)` - Change priority
- `wait_event()` - Wait for event
- `send_event(task_id)` - Send event to task

### Time Module (`kernel.time`)
Timer and clock operations.

**Key Types:**
- `Time` - Seconds and milliseconds structure
- `TimerId` - Timer identifier
- `TimerCallback` - Timer callback function

**Key Functions:**
- `now()` - Get current time
- `millis()` - Milliseconds since boot
- `micros()` - Microseconds since boot
- `sleep_ms(ms)` - Sleep milliseconds
- `sleep_s(seconds)` - Sleep seconds
- `timer_once(delay_ms, callback)` - One-shot timer
- `timer_repeat(interval_ms, callback)` - Repeating timer
- `timer_cancel(timer_id)` - Cancel timer
- `elapsed(start)` - Get elapsed time
- `frequency()` - Get tick frequency

### Memory Module (`kernel.memory`)
Memory allocation and management.

**Key Types:**
- `MemStats` - Memory statistics structure

**Key Functions:**
- `malloc(size)` - Allocate memory
- `free(ptr)` - Free memory
- `calloc(count, size)` - Allocate zero-initialized
- `realloc(ptr, size)` - Reallocate memory
- `stats()` - Get memory statistics
- `memset(ptr, value, size)` - Set memory
- `memcpy(dest, src, size)` - Copy memory
- `memcmp(a, b, size)` - Compare memory
- `check_heap()` - Check heap integrity
- `print_stats()` - Print statistics

### Console Module (`kernel.console`)
Standard input/output and terminal operations.

**Key Types:**
- `OutputMode` - Blocking or non-blocking I/O
- `Color` - Text colors
- `CursorPos` - Cursor position structure

**Key Functions:**
- `write(message)` - Write to stdout
- `println(message)` - Print with newline
- `printf(format)` - Formatted output
- `readline()` - Read line from stdin
- `read_char()` - Read single character
- `clear()` - Clear console
- `set_color(color)` - Set text color
- `reset_color()` - Reset text color
- `get_cursor()` - Get cursor position
- `set_cursor(pos)` - Set cursor position

## Example Usage

```vitte
use std/kernel
use std/kernel.{time, console, task}

entry main at core/app {
  kernel.init()
  console.println("Vitte Kernel")
  
  let uptime = kernel.uptime_ms()
  console.printf("Uptime: {uptime}ms\n")
  
  kernel.start_scheduler()
  
  return 0
}
```
