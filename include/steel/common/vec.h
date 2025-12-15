#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t* data;
  size_t len;
  size_t cap;
} steel_vec_u8;

void steel_vec_u8_init(steel_vec_u8* v);
void steel_vec_u8_free(steel_vec_u8* v);
int  steel_vec_u8_reserve(steel_vec_u8* v, size_t cap);
int  steel_vec_u8_push(steel_vec_u8* v, uint8_t x);

typedef struct {
  char* data;
  size_t len;
  size_t cap;
} steel_vec_char;

void steel_vec_char_init(steel_vec_char* v);
void steel_vec_char_free(steel_vec_char* v);
int  steel_vec_char_reserve(steel_vec_char* v, size_t cap);
int  steel_vec_char_push(steel_vec_char* v, char c);
int  steel_vec_char_append(steel_vec_char* v, const char* s);
