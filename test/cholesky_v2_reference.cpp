#include "cholesky_v2_reference.h"

#define cholesky cholesky_v2_reference
#include "../history/versions/cholesky_v2_serial_blocked/src/cholesky.cpp"
#undef cholesky
