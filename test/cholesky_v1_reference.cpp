#include "cholesky_v1_reference.h"

#define cholesky cholesky_v1_reference
#include "../history/versions/cholesky_v1_serial_opt/src/cholesky.cpp"
#undef cholesky
