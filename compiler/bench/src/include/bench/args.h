#ifndef VITTE_ARGS_H_INCLUDED
#define VITTE_ARGS_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_args_kind {
    VITTE_ARGS_FLAG,
    VITTE_ARGS_STRING,
    VITTE_ARGS_U64,
    VITTE_ARGS_I64,
    VITTE_ARGS_F64,
} vitte_args_kind;

typedef struct vitte_args_opt {
    const char* long_name;
    char short_name;
    vitte_args_kind kind;
    bool required;

    /* Output pointers */
    bool* out_bool;
    char** out_str;
    size_t* out_list_count;
    size_t max_count;
    uint64_t* out_u64;
    int64_t* out_i64;
    double* out_f64;

    /* Description for usage */
    const char* description;
} vitte_args_opt;

typedef struct vitte_args_ctx {
    int argc;
    char** argv;

    const vitte_args_opt* opts;
    size_t opt_count;

    /* Errors */
    char err_msg[256];
    /*
      Option-seen bitmap (length opt_count). If NULL, required checking is best-effort.
      - If an arena is provided, vitte_args_parse() will lazily allocate this array.
    */
    uint8_t* seen;
    size_t   seen_cap;

    /* Whether to stop parsing options at the first '--' */
    bool stop_at_double_dash;

    /* Arena for allocations (optional) */
    void* arena;
} vitte_args_ctx;

void vitte_args_set_error(vitte_args_ctx* ctx, const char* fmt, ...);

static inline void vitte_args_ctx_init(vitte_args_ctx* ctx, int argc, char** argv, const vitte_args_opt* opts, size_t opt_count, void* arena)
{
    ctx->argc = argc;
    ctx->argv = argv;
    ctx->opts = opts;
    ctx->opt_count = opt_count;
    ctx->arena = arena;
    ctx->stop_at_double_dash = true;
    ctx->err_msg[0] = '\0';
    ctx->seen = NULL;
    ctx->seen_cap = 0;
}

/* Usage printing                                                              */
static inline void vitte_args__eprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vfprintf(stderr, fmt, ap);
    (void)fputc('\n', stderr);
    va_end(ap);
}

/*
  Print a usage string ...
*/

static inline const vitte_args_opt* vitte_args_find_long_idx(const vitte_args_ctx* ctx, const char* name, size_t name_len, size_t* out_idx)
{
    for (size_t i = 0; i < ctx->opt_count; i++) {
        const vitte_args_opt* o = &ctx->opts[i];
        if (!o->long_name) {
            continue;
        }
        /* compare lengths */
        size_t ln = 0;
        while (o->long_name[ln] != '\0') ln++;
        if (ln != name_len) {
            continue;
        }
        bool eq = true;
        for (size_t k = 0; k < name_len; k++) {
            if (o->long_name[k] != name[k]) {
                eq = false;
                break;
            }
        }
        if (eq) {
            if (out_idx) {
                *out_idx = i;
            }
            return o;
        }
    }
    return NULL;
}

static inline const vitte_args_opt* vitte_args_find_long(const vitte_args_ctx* ctx, const char* name, size_t name_len)
{
    return vitte_args_find_long_idx(ctx, name, name_len, NULL);
}

static inline const vitte_args_opt* vitte_args_find_short_idx(const vitte_args_ctx* ctx, char ch, size_t* out_idx)
{
    for (size_t i = 0; i < ctx->opt_count; i++) {
        const vitte_args_opt* o = &ctx->opts[i];
        if (o->short_name != 0 && o->short_name == ch) {
            if (out_idx) {
                *out_idx = i;
            }
            return o;
        }
    }
    return NULL;
}

static inline const vitte_args_opt* vitte_args_find_short(const vitte_args_ctx* ctx, char ch)
{
    return vitte_args_find_short_idx(ctx, ch, NULL);
}

static inline bool vitte_args_parse(vitte_args_ctx* ctx)
{
    if (!ctx || !ctx->argv || ctx->argc < 1) {
        return false;
    }

    /* Lazy allocate seen bitmap when an arena is available. */
    if (!ctx->seen && ctx->arena && ctx->opt_count > 0) {
        ctx->seen = (uint8_t*)vitte_arena_calloc_aligned(ctx->arena, ctx->opt_count, 1);
        ctx->seen_cap = ctx->seen ? ctx->opt_count : 0;
    }

    int i = 1;
    bool stop_parsing = false;
    while (i < ctx->argc) {
        char* arg = ctx->argv[i];
        if (stop_parsing) {
            i++;
            continue;
        }
        if (arg[0] != '-') {
            /* Positional argument */
            i++;
            continue;
        }
        if (arg[1] == '-' && arg[2] == '\0') {
            if (ctx->stop_at_double_dash) {
                stop_parsing = true;
            }
            i++;
            continue;
        }
        if (arg[1] == '-' && arg[2] != '\0') {
            /* Long option */
            const char* name = &arg[2];
            size_t name_len = 0;
            while (name[name_len] != '\0' && name[name_len] != '=') {
                name_len++;
            }

            size_t opt_idx = (size_t)-1;
            const vitte_args_opt* o = vitte_args_find_long_idx(ctx, name, name_len, &opt_idx);
            if (!o) {
                vitte_args_set_error(ctx, "invalid option: %s", arg);
                return false;
            }
            if (ctx->seen && opt_idx != (size_t)-1 && opt_idx < ctx->seen_cap) {
                ctx->seen[opt_idx] = 1;
            }

            const char* value = NULL;
            if (name[name_len] == '=') {
                value = &name[name_len + 1];
            }

            switch (o->kind) {
                case VITTE_ARGS_FLAG:
                    if (o->out_bool) {
                        *o->out_bool = true;
                    }
                    i++;
                    break;
                case VITTE_ARGS_STRING:
                    if (!value) {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: --%s", o->long_name);
                            return false;
                        }
                        value = ctx->argv[++i];
                    }
                    if (o->max_count > 1 && o->out_list_count && o->out_str) {
                        size_t count = *o->out_list_count;
                        if (count < o->max_count) {
                            o->out_str[count] = (char*)value;
                            *o->out_list_count = count + 1;
                        } else {
                            vitte_args_set_error(ctx, "too many values for option: --%s", o->long_name);
                            return false;
                        }
                    } else if (o->out_str) {
                        *o->out_str = (char*)value;
                    }
                    i++;
                    break;
                case VITTE_ARGS_U64:
                    if (!value) {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: --%s", o->long_name);
                            return false;
                        }
                        value = ctx->argv[++i];
                    }
                    if (o->out_u64) {
                        char* endptr = NULL;
                        unsigned long long val = strtoull(value, &endptr, 10);
                        if (endptr == value || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid uint64 value for option: --%s", o->long_name);
                            return false;
                        }
                        *o->out_u64 = (uint64_t)val;
                    }
                    i++;
                    break;
                case VITTE_ARGS_I64:
                    if (!value) {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: --%s", o->long_name);
                            return false;
                        }
                        value = ctx->argv[++i];
                    }
                    if (o->out_i64) {
                        char* endptr = NULL;
                        long long val = strtoll(value, &endptr, 10);
                        if (endptr == value || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid int64 value for option: --%s", o->long_name);
                            return false;
                        }
                        *o->out_i64 = (int64_t)val;
                    }
                    i++;
                    break;
                case VITTE_ARGS_F64:
                    if (!value) {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: --%s", o->long_name);
                            return false;
                        }
                        value = ctx->argv[++i];
                    }
                    if (o->out_f64) {
                        char* endptr = NULL;
                        double val = strtod(value, &endptr);
                        if (endptr == value || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid float64 value for option: --%s", o->long_name);
                            return false;
                        }
                        *o->out_f64 = val;
                    }
                    i++;
                    break;
                default:
                    i++;
                    break;
            }
            continue;
        }
        /* Short option(s) */
        for (size_t j = 1; arg[j] != '\0'; j++) {
            char ch = arg[j];
            size_t opt_idx = (size_t)-1;
            const vitte_args_opt* o = vitte_args_find_short_idx(ctx, ch, &opt_idx);
            if (!o) {
                vitte_args_set_error(ctx, "invalid option: -%c", ch);
                return false;
            }
            if (ctx->seen && opt_idx != (size_t)-1 && opt_idx < ctx->seen_cap) {
                ctx->seen[opt_idx] = 1;
            }
            switch (o->kind) {
                case VITTE_ARGS_FLAG:
                    if (o->out_bool) {
                        *o->out_bool = true;
                    }
                    break;
                case VITTE_ARGS_STRING:
                    if (arg[j + 1] != '\0') {
                        /* The rest of arg is the value */
                        if (o->max_count > 1 && o->out_list_count && o->out_str) {
                            size_t count = *o->out_list_count;
                            if (count < o->max_count) {
                                o->out_str[count] = &arg[j + 1];
                                *o->out_list_count = count + 1;
                            } else {
                                vitte_args_set_error(ctx, "too many values for option: -%c", ch);
                                return false;
                            }
                        } else if (o->out_str) {
                            *o->out_str = &arg[j + 1];
                        }
                        j = strlen(arg) - 1;
                    } else {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: -%c", ch);
                            return false;
                        }
                        if (o->max_count > 1 && o->out_list_count && o->out_str) {
                            size_t count = *o->out_list_count;
                            if (count < o->max_count) {
                                o->out_str[count] = ctx->argv[i + 1];
                                *o->out_list_count = count + 1;
                            } else {
                                vitte_args_set_error(ctx, "too many values for option: -%c", ch);
                                return false;
                            }
                        } else if (o->out_str) {
                            *o->out_str = ctx->argv[i + 1];
                        }
                        i++;
                    }
                    break;
                case VITTE_ARGS_U64:
                    if (arg[j + 1] != '\0') {
                        char* endptr = NULL;
                        unsigned long long val = strtoull(&arg[j + 1], &endptr, 10);
                        if (endptr == &arg[j + 1] || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid uint64 value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_u64) {
                            *o->out_u64 = (uint64_t)val;
                        }
                        j = strlen(arg) - 1;
                    } else {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_u64) {
                            char* endptr = NULL;
                            unsigned long long val = strtoull(ctx->argv[i + 1], &endptr, 10);
                            if (endptr == ctx->argv[i + 1] || *endptr != '\0') {
                                vitte_args_set_error(ctx, "invalid uint64 value for option: -%c", ch);
                                return false;
                            }
                            *o->out_u64 = (uint64_t)val;
                        }
                        i++;
                    }
                    break;
                case VITTE_ARGS_I64:
                    if (arg[j + 1] != '\0') {
                        char* endptr = NULL;
                        long long val = strtoll(&arg[j + 1], &endptr, 10);
                        if (endptr == &arg[j + 1] || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid int64 value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_i64) {
                            *o->out_i64 = (int64_t)val;
                        }
                        j = strlen(arg) - 1;
                    } else {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_i64) {
                            char* endptr = NULL;
                            long long val = strtoll(ctx->argv[i + 1], &endptr, 10);
                            if (endptr == ctx->argv[i + 1] || *endptr != '\0') {
                                vitte_args_set_error(ctx, "invalid int64 value for option: -%c", ch);
                                return false;
                            }
                            *o->out_i64 = (int64_t)val;
                        }
                        i++;
                    }
                    break;
                case VITTE_ARGS_F64:
                    if (arg[j + 1] != '\0') {
                        char* endptr = NULL;
                        double val = strtod(&arg[j + 1], &endptr);
                        if (endptr == &arg[j + 1] || *endptr != '\0') {
                            vitte_args_set_error(ctx, "invalid float64 value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_f64) {
                            *o->out_f64 = val;
                        }
                        j = strlen(arg) - 1;
                    } else {
                        if (i + 1 >= ctx->argc) {
                            vitte_args_set_error(ctx, "missing value for option: -%c", ch);
                            return false;
                        }
                        if (o->out_f64) {
                            char* endptr = NULL;
                            double val = strtod(ctx->argv[i + 1], &endptr);
                            if (endptr == ctx->argv[i + 1] || *endptr != '\0') {
                                vitte_args_set_error(ctx, "invalid float64 value for option: -%c", ch);
                                return false;
                            }
                            *o->out_f64 = val;
                        }
                        i++;
                    }
                    break;
                default:
                    break;
            }
        }
        i++;
    }

    /* required check (accurate when ctx->seen is available) */
    if (ctx->seen) {
        for (size_t i = 0; i < ctx->opt_count; i++) {
            const vitte_args_opt* o = &ctx->opts[i];
            if (!o->required) {
                continue;
            }
            if (i >= ctx->seen_cap || ctx->seen[i] == 0) {
                vitte_args_set_error(ctx, "missing required option: --%s", o->long_name);
                return false;
            }
        }
    }

    return true;
}

static inline void vitte_args_print_usage(const vitte_args_ctx* ctx, const char* program_name)
{
    vitte_args__eprintf("Usage: %s [options]", program_name);
    vitte_args__eprintf("Options:");
    for (size_t i = 0; i < ctx->opt_count; i++) {
        const vitte_args_opt* o = &ctx->opts[i];
        char short_opt[8] = "";
        if (o->short_name) {
            snprintf(short_opt, sizeof(short_opt), "-%c,", o->short_name);
        }
        if (o->kind == VITTE_ARGS_FLAG) {
            vitte_args__eprintf("  %s --%s\t%s", short_opt, o->long_name, o->description ? o->description : "");
        } else {
            vitte_args__eprintf("  %s --%s <value>\t%s", short_opt, o->long_name, o->description ? o->description : "");
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* VITTE_ARGS_H_INCLUDED */
