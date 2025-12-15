#ifndef STEEL_VALUE_H
#define STEEL_VALUE_H

typedef enum {
    VALUE_NIL,
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_OBJECT,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int boolean;
        double number;
        char *string;
        void *object;
    } as;
} Value;

Value value_number(double num);
Value value_string(const char *str);
Value value_bool(int b);
Value value_nil(void);

#endif
