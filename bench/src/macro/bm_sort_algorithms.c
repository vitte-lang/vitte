#include "bench/bench.h"
#include <string.h>
#include <stdlib.h>

/*
  bm_sort_algorithms.c (macro)

  Goal:
  - Macro benchmark for sorting algorithms on varying data sizes.
  - Tests bubble sort, insertion sort, quick sort equivalent.
  - Measures algorithmic complexity with realistic datasets.

  Design:
  - Arrays of 100, 500, 1000 elements.
  - Sorted, reverse-sorted, and random data.
  - Multiple algorithm implementations.
*/

/* Simple bubble sort */
static void bubble_sort(int* arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        int tmp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = tmp;
      }
    }
  }
}

/* Insertion sort */
static void insertion_sort(int* arr, int n) {
  for (int i = 1; i < n; i++) {
    int key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = key;
  }
}

/* Partition helper for quicksort-like */
static int partition(int* arr, int low, int high) {
  int pivot = arr[high];
  int i = low - 1;
  for (int j = low; j < high; j++) {
    if (arr[j] < pivot) {
      i++;
      int tmp = arr[i];
      arr[i] = arr[j];
      arr[j] = tmp;
    }
  }
  int tmp = arr[i + 1];
  arr[i + 1] = arr[high];
  arr[high] = tmp;
  return i + 1;
}

/* Quicksort */
static void quicksort(int* arr, int low, int high) {
  if (low < high) {
    int pi = partition(arr, low, high);
    quicksort(arr, low, pi - 1);
    quicksort(arr, pi + 1, high);
  }
}

/* Initialize array with pattern */
static void init_array(int* arr, int n, int pattern) {
  switch (pattern) {
    case 0: /* ascending */
      for (int i = 0; i < n; i++) arr[i] = i;
      break;
    case 1: /* descending */
      for (int i = 0; i < n; i++) arr[i] = n - i;
      break;
    case 2: /* random-ish */
      for (int i = 0; i < n; i++) arr[i] = (i * 37 + 17) % (n * 2);
      break;
  }
}

static void bm_sort_bubble(void* ctx) {
  (void)ctx;
  
  int arr[256];
  for (int iter = 0; iter < 10; iter++) {
    for (int pattern = 0; pattern < 3; pattern++) {
      init_array(arr, 256, pattern);
      bubble_sort(arr, 256);
    }
  }
}

static void bm_sort_insertion(void* ctx) {
  (void)ctx;
  
  int arr[512];
  for (int iter = 0; iter < 10; iter++) {
    for (int pattern = 0; pattern < 3; pattern++) {
      init_array(arr, 512, pattern);
      insertion_sort(arr, 512);
    }
  }
}

static void bm_sort_quick(void* ctx) {
  (void)ctx;
  
  int arr[1024];
  for (int iter = 0; iter < 5; iter++) {
    for (int pattern = 0; pattern < 3; pattern++) {
      init_array(arr, 1024, pattern);
      quicksort(arr, 0, 1023);
    }
  }
}

void bench_register_macro_sorts(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:sort_bubble", BENCH_MACRO, bm_sort_bubble, NULL);
  bench_registry_add("macro:sort_insertion", BENCH_MACRO, bm_sort_insertion, NULL);
  bench_registry_add("macro:sort_quick", BENCH_MACRO, bm_sort_quick, NULL);
}
