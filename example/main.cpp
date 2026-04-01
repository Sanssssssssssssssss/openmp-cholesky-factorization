#include "mphil_dis_cholesky.h"

#include <iomanip>
#include <iostream>

int main() {

    int matrix_size = 2;

    double matrix[] = {
        4.0, 2.0,
        2.0, 26.0,
    };

    // double matrix[] = {
    //     4.0, 2.0,  4.0, 2.0, 2.0, 26.0, 4.0, 2.0, 2.0, 26.0,
    //     2.0, 26.0, 4.0, 2.0, 2.0, 26.0, 4.0, 2.0, 2.0, 26.0,
    //     2.0, 26.0, 4.0, 2.0, 2.0, 26.0, 4.0, 2.0, 2.0, 26.0,
    //     2.0, 26.0, 4.0, 2.0, 2.0, 26.0, 4.0, 2.0, 2.0, 26.0,
    //     2.0, 26.0, 2.0, 26.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0,
    //     2.0, 26.0, 4.0, 2.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0,
    //     2.0, 26.0, 4.0, 2.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0,
    //     2.0, 26.0, 4.0, 2.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0,
    //     2.0, 26.0, 4.0, 2.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0,
    //     2.0, 26.0, 4.0, 2.0, 2.0, 26.0, 2.0, 2.0, 2.0, 26.0,
    // };

    const double elapsed = cholesky(matrix, matrix_size);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "elapsed_seconds=" << elapsed << '\n';
    std::cout << "matrix_after=" << '\n';

    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j) {
            std::cout << matrix[i * matrix_size + j];
            if (j + 1 < matrix_size) {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }

    return 0;
}
