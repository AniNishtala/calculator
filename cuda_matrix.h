#pragma once

void multiplyMatricesCUDA(int r1, int c1, int c2,
    const double* firstmat, const double* secondmat,
    double* resultmat, double& seconds);

void addMatricesCUDA(int rows, int cols,
    const double* firstmat, const double* secondmat,
    double* resultmat, double& seconds);

// Sum of matching element products; returns one number by reference.
void dotMatricesCUDA(int rows, int cols,
    const double* firstmat, const double* secondmat,
    double& result, double& seconds);
