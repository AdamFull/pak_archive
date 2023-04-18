#ifndef LIBFMEMOPEN_H
#define LIBFMEMOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* https://github.com/Arryboom/fmemopen_windows  */
FILE* fmemopen(void* buf, size_t len, const char* type);

#ifdef __cplusplus
}
#endif

#endif // LIBFMEMOPEN_H