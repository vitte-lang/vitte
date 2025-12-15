#ifndef STEEL_ERROR_H
#define STEEL_ERROR_H

void error_report(const char *filename, int line, int column, const char *message);
void error_warning(const char *filename, int line, int column, const char *message);

#endif
