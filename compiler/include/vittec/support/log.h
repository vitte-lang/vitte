#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_LOG_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_LOG_H



    typedef enum vittec_log_level {
  VITTEC_LOG_ERROR = 0,
  VITTEC_LOG_WARN  = 1,
  VITTEC_LOG_INFO  = 2,
  VITTEC_LOG_DEBUG = 3
} vittec_log_level_t;

void vittec_log_set_level(vittec_log_level_t lvl);
void vittec_log(vittec_log_level_t lvl, const char* fmt, ...);

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_LOG_H */
