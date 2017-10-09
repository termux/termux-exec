#ifndef TERMUX_REWRITE_EXECUTABLE__INCLUDED
#define TERMUX_REWRITE_EXECUTABLE__INCLUDED

#include <stddef.h>

const char* termux_rewrite_executable(
  char* rewritten,
  size_t rewritten_size,
  const char* original);

#endif // TERMUX_REWRITE_EXECUTABLE__INCLUDED
