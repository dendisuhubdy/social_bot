#pragma once
#include <stdarg.h>

int str2netstr(const char *inStr, char *outStr);
std::string base64_encode(unsigned char const* bytes_to_encode,
                                  unsigned int in_len);
const std::string currentDateTime();

void LOG_create(const char *filename);
void LOG_close();
int LOG_printf(const char *fmt, ...);

