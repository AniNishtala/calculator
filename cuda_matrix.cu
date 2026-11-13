#include "cuda_matrix.h"
#include <cuda_runtime.h>
#include <chrono>

// Your original naive matrix multiplication kernel.
__global__ void matmultiplication(int r1, int c1, int c2, double* firstmat, double* secondmat, double* resultmat)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    double sum = 0;

    if (x >= r1 || y >= c2)
    {
        return;
    }

    for (int k = 0; k < c1; k++)
    {
        sum += firstmat[x * c1 + k] * secondmat[k * c2 + y];
    }

    resultmat[x * c2 + y] = sum;
}

__global__ void mataddition(int rows, int cols, double* firstmat, double* secondmat, double* resultmat)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= rows || y >= cols)
    {
        return;
    }

    size_t index = static_cast<size_t>(x) * cols + y;
    resultmat[index] = firstmat[index] + secondmat[index];
}

// Each GPU thread writes one row's partial dot product.
__global__ void matdotproduct(int rows, int cols, double* firstmat, double* secondmat, double* partial)
{
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= rows)
    {
        return;
    }

    double sum = 0.0;
    for (int j = 0; j < cols; j++)
    {
        size_t index = static_cast<size_t>(row) * cols + j;
        sum += firstmat[index] * secondmat[index];
    }
    partial[row] = sum;
}

// One final GPU thread combines the partial sums. No shared updates or atomics.
__global__ void finishDotProduct(int rows, double* partial, double* result)
{
    double sum = 0.0;
    for (int i = 0; i < rows; i++)
    {
        sum += partial[i];
    }
    result[0] = sum;
}

// Common allocation/copy/launch code. CUDA error handling can be added later.
// operation: 2 = multiplication, 3 = addition, 4 = dot product.
static void runMatricesCUDA(int operation, int r1, int c1, int c2, const double* firstmat, const double* secondmat, double* resultmat, double& seconds)
{
    size_t firstBytes = static_cast<size_t>(r1) * c1 * sizeof(double);
    size_t secondBytes = operation == 2
        ? static_cast<size_t>(c1) * c2 * sizeof(double) : firstBytes;
    size_t resultBytes = operation == 4 ? sizeof(double)
        : static_cast<size_t>(r1) * (operation == 2 ? c2 : c1) * sizeof(double);

    double* firstmat_d = nullptr;
    double* secondmat_d = nullptr;
    double* resultmat_d = nullptr;
    double* partial_d = nullptr;

    auto start = std::chrono::steady_clock::now();

    cudaMalloc((void**)&firstmat_d, firstBytes);
    cudaMalloc((void**)&secondmat_d, secondBytes);
    cudaMalloc((void**)&resultmat_d, resultBytes);
    cudaMemcpy(firstmat_d, firstmat, firstBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(secondmat_d, secondmat, secondBytes, cudaMemcpyHostToDevice);

    if (operation == 2)
    {
        dim3 block(16, 16);
        dim3 grid((r1 - 1) / block.x + 1, (c2 - 1) / block.y + 1);
        matmultiplication<<<grid, block>>>(
            r1, c1, c2, firstmat_d, secondmat_d, resultmat_d);
    }
    else if (operation == 3)
    {
        dim3 block(16, 16);
        dim3 grid((r1 - 1) / block.x + 1, (c1 - 1) / block.y + 1);
        mataddition<<<grid, block>>>(
            r1, c1, firstmat_d, secondmat_d, resultmat_d);
    }
    else
    {
        cudaMalloc((void**)&partial_d, static_cast<size_t>(r1) * sizeof(double));
        int block = 256;
        int grid = (r1 - 1) / block + 1;
        matdotproduct<<<grid, block>>>(r1, c1, firstmat_d, secondmat_d, partial_d);
        // Same-stream kernel launches execute in order.
        finishDotProduct<<<1, 1>>>(r1, partial_d, resultmat_d);
    }

    cudaDeviceSynchronize();
    cudaMemcpy(resultmat, resultmat_d, resultBytes, cudaMemcpyDeviceToHost);

    cudaFree(firstmat_d);
    cudaFree(secondmat_d);
    cudaFree(resultmat_d);
    cudaFree(partial_d);

    auto end = std::chrono::steady_clock::now();
    seconds = std::chrono::duration<double>(end - start).count();
}

void multiplyMatricesCUDA(int r1, int c1, int c2, const double* firstmat, const double* secondmat, double* resultmat, double& seconds)
{
    runMatricesCUDA(2, r1, c1, c2, firstmat, secondmat, resultmat, seconds);
}

void addMatricesCUDA(int rows, int cols, const double* firstmat, const double* secondmat, double* resultmat, double& seconds)
{
    runMatricesCUDA(3, rows, cols, cols, firstmat, secondmat, resultmat, seconds);
}

void dotMatricesCUDA(int rows, int cols, const double* firstmat, const double* secondmat, double& result, double& seconds)
{
    runMatricesCUDA(4, rows, cols, cols, firstmat, secondmat, &result, seconds);
}
