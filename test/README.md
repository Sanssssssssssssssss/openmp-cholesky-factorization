# Test Directory

The baseline correctness test entrypoint is:

```bash
./build/cholesky_tests
```

The current tests cover:

- zero-size handling
- 1x1 factorization
- the coursework 2x2 example
- exact log-determinant recovery for the coursework example
- diagonal SPD matrices
- a known 3x3 factor with exact expected output
- a denser 4x4 rebuild case
- reconstruction of a generated SPD correlation matrix via `L * L^T`
- positive diagonal checks after factorization
- invalid input rejection
- early failure on a negative diagonal
