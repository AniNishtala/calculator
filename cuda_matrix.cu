#include "cuda_matrix.h"
#include <cuda_runtime.h>
#include <chrono>

__global__ void matmultiplication(int r1, int c1, int c2,double* firstmat,double* secondmat,double* resultmat)
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

// CPU code calls this function to run the kernel above.
// Error handling can be added later.
void multiplyMatricesCUDA(int r1, int c1, int c2,const double* firstmat,const double* secondmat,double* resultmat,double& seconds)
{
    size_t firstBytes = static_cast<size_t>(r1) * c1 * sizeof(double);
    size_t secondBytes = static_cast<size_t>(c1) * c2 * sizeof(double);
    size_t resultBytes = static_cast<size_t>(r1) * c2 * sizeof(double);

    double* firstmat_d = nullptr;
    double* secondmat_d = nullptr;
    double* resultmat_d = nullptr;

    auto start = std::chrono::steady_clock::now();

    cudaMalloc((void**)&firstmat_d, firstBytes);
    cudaMalloc((void**)&secondmat_d, secondBytes);
    cudaMalloc((void**)&resultmat_d, resultBytes);

    cudaMemcpy(firstmat_d, firstmat, firstBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(secondmat_d, secondmat, secondBytes, cudaMemcpyHostToDevice);

    dim3 block(16, 16);
    dim3 grid((r1 - 1) / block.x + 1, (c2 - 1) / block.y + 1);

    matmultiplication<<<grid, block>>>(
        r1, c1, c2, firstmat_d, secondmat_d, resultmat_d);

    cudaDeviceSynchronize();

    cudaMemcpy(resultmat, resultmat_d, resultBytes, cudaMemcpyDeviceToHost);

    cudaFree(firstmat_d);
    cudaFree(secondmat_d);
    cudaFree(resultmat_d);

    auto end = std::chrono::steady_clock::now();
    seconds = std::chrono::duration<double>(end - start).count();
}

