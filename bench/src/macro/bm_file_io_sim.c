#include "bench/bench.h"
#include <string.h>
#include <stdio.h>

/*
  bm_file_io_sim.c (macro)

  Goal:
  - Macro benchmark simulating file I/O patterns.
  - Tests sequential reads, random access, buffering.
  - Measures I/O overhead without actual disk access.

  Design:
  - In-memory buffer simulating file data.
  - Sequential and random access patterns.
  - Buffered vs unbuffered simulation.
*/

#define SIMULATED_FILE_SIZE (1024 * 1024)  /* 1 MB */
#define BUFFER_SIZE 4096

static unsigned char g_file_data[SIMULATED_FILE_SIZE];

static void init_file_sim(void) {
  static int initialized = 0;
  if (initialized) return;
  
  for (size_t i = 0; i < sizeof(g_file_data); i++) {
    g_file_data[i] = (unsigned char)((i * 37 + 11) % 256);
  }
  initialized = 1;
}

/* Sequential read simulation */
static void bm_file_sequential_read(void* ctx) {
  (void)ctx;
  init_file_sim();
  
  unsigned char buf[BUFFER_SIZE];
  uint64_t total = 0;
  
  for (int iter = 0; iter < 10; iter++) {
    for (size_t pos = 0; pos < SIMULATED_FILE_SIZE; pos += BUFFER_SIZE) {
      size_t to_read = (pos + BUFFER_SIZE < SIMULATED_FILE_SIZE) ?
                       BUFFER_SIZE : (SIMULATED_FILE_SIZE - pos);
      memcpy(buf, &g_file_data[pos], to_read);
      
      for (size_t i = 0; i < to_read; i++) {
        total += buf[i];
      }
    }
  }
  (void)total;
}

/* Random access simulation */
static void bm_file_random_access(void* ctx) {
  (void)ctx;
  init_file_sim();
  
  unsigned char buf[BUFFER_SIZE];
  uint64_t total = 0;
  uint32_t seed = 0xDEADBEEF;
  
  for (int iter = 0; iter < 100; iter++) {
    for (int access = 0; access < 1000; access++) {
      seed = seed * 1103515245 + 12345;
      size_t pos = (seed % (SIMULATED_FILE_SIZE - BUFFER_SIZE));
      
      memcpy(buf, &g_file_data[pos], BUFFER_SIZE);
      for (size_t i = 0; i < BUFFER_SIZE; i++) {
        total += buf[i];
      }
    }
  }
  (void)total;
}

/* Buffered read with cache simulation */
static void bm_file_buffered_read(void* ctx) {
  (void)ctx;
  init_file_sim();
  
  unsigned char cache[BUFFER_SIZE];
  size_t cache_pos = 0;
  size_t cache_valid = 0;
  uint64_t total = 0;
  
  for (int iter = 0; iter < 10; iter++) {
    for (size_t pos = 0; pos < SIMULATED_FILE_SIZE; pos++) {
      if (pos < cache_pos || pos >= cache_pos + cache_valid) {
        /* Cache miss - reload */
        cache_pos = pos;
        cache_valid = (pos + BUFFER_SIZE < SIMULATED_FILE_SIZE) ?
                      BUFFER_SIZE : (SIMULATED_FILE_SIZE - pos);
        memcpy(cache, &g_file_data[cache_pos], cache_valid);
      }
      
      total += cache[pos - cache_pos];
    }
  }
  (void)total;
}

/* Line-by-line read simulation */
static void bm_file_line_read(void* ctx) {
  (void)ctx;
  init_file_sim();
  
  const unsigned char* pos = g_file_data;
  const unsigned char* end = g_file_data + SIMULATED_FILE_SIZE;
  uint64_t line_count = 0;
  
  for (int iter = 0; iter < 10; iter++) {
    pos = g_file_data;
    while (pos < end) {
      /* Simulate line reading */
      const unsigned char* line_start = pos;
      while (pos < end && *pos != '\n') {
        pos++;
      }
      if (pos < end) pos++;  /* skip newline */
      
      line_count++;
      
      /* Simulate processing */
      for (const unsigned char* p = line_start; p < pos && p < end; p++) {
        line_count += *p;
      }
    }
  }
  (void)line_count;
}

void bench_register_macro_file_io(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:file_seq_read", BENCH_MACRO, bm_file_sequential_read, NULL);
  bench_registry_add("macro:file_rand_access", BENCH_MACRO, bm_file_random_access, NULL);
  bench_registry_add("macro:file_buffered", BENCH_MACRO, bm_file_buffered_read, NULL);
  bench_registry_add("macro:file_line_read", BENCH_MACRO, bm_file_line_read, NULL);
}
