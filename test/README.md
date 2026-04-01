# Test Directory

The baseline correctness test entrypoint is:

```bash
./build/cholesky_tests
```

The current tests cover:

- zero-size handling
- 1x1 factorization
- the coursework 2x2 example
- a known 3x3 factor with exact expected output
- reconstruction of a generated SPD correlation matrix via `L * L^T`
- invalid input rejection
