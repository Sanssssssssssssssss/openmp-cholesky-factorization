#ifndef CHOLESKY_H
#define CHOLESKY_H

// Computes an in-place Cholesky factorization of a row-major matrix.
// Returns elapsed wall clock time in seconds, or -1.0 on invalid input.
double cholesky(double* c, int n);

#endif
