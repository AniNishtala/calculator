#pragma once

#include <bits/stdc++.h>
#include <pthread.h>
#include <omp.h>

#ifdef CALCULATOR_USE_CUDA
#include "cuda_matrix.h"
#endif

using namespace std;

class calculator
{
public:
    calculator()
    {
    }

    ~calculator()
    {
        freeMatrices();
    }

    // Each object has its matrices, so DONT copy the object.
    calculator(const calculator&) = delete;
    calculator& operator=(const calculator&) = delete;

    // GUI: leave these functions for later.

    void createWindow()
    {
        // Implementation required.
    }

    void createInputBoxes()
    {
        // Implementation required.
    }

    void createCPUButton()
    {
        // Implementation required.
    }

    void createPthreadsButton()
    {
        // Implementation required.
    }

    void createOpenMPButton()
    {
        // Implementation required.
    }

    void createCUDAButton()
    {
        // Implementation required.
    }

    void createClearButton()
    {
        // Implementation required.
    }

    void createResultBox()
    {
        // Implementation required.
    }

    void runGUI()
    {
        // Implementation required.
    }

    void closeGUI()
    {
        // Implementation required.
    }

    // GUI button actions: leave these for later too.

    void cpuButtonClicked()
    {
        // Implementation required.
    }

    void pthreadsButtonClicked()
    {
        // Implementation required.
    }

    void openmpButtonClicked()
    {
        // Implementation required.
    }

    void cudaButtonClicked()
    {
        // Implementation required.
    }

    void clearButtonClicked()
    {
        // Implementation required.
    }

    // Read from the CLI for now. Later, read from GUI input boxes.
    void readInput(int& r1, int& c1, int& r2, int& c2, int& threads)
    {
        cout << "First matrix: enter rows and columns: ";
        if (!(cin >> r1 >> c1))
        {
            throw invalid_argument("Enter whole numbers for matrix sizes.");
        }

        cout << "Second matrix: enter rows and columns: ";
        if (!(cin >> r2 >> c2))
        {
            throw invalid_argument("Enter whole numbers for matrix sizes.");
        }

        cout << "CPU threads (1-16; 16 for 5800H. 9950X has 32): ";
        if (!(cin >> threads))
        {
            throw invalid_argument("Enter a whole number for threads.");
        }
    }

    void checkInput(bool& valid)
    {
        valid = r1 > 0 && c1 > 0 && r2 > 0 && c2 > 0 && c1 == r2 && threadCount >= 1 && threadCount <= 32;
    }

    // Matrix memory: same malloc/free approach as the C example.
    void allocateMatrices()
    {
        bool valid = false;
        checkInput(valid);

        if (!valid)
        {
            throw invalid_argument("Use positive sizes, c1 == r2, and 1-32 threads.");
        }

        // Check the sizes before multiplying them to get byte counts.
        size_t maxElements = numeric_limits<size_t>::max() / sizeof(double);

        if (static_cast<size_t>(r1) > maxElements / c1 || static_cast<size_t>(r2) > maxElements / c2 || static_cast<size_t>(r1) > maxElements / c2)
        {
            throw length_error("Matrix sizes are too large.");
        }

        freeMatrices();

        firstmat = static_cast<double*>(malloc(static_cast<size_t>(r1) * c1 * sizeof(double)));

        secondmat = static_cast<double*>(malloc(static_cast<size_t>(r2) * c2 * sizeof(double)));

        resultmat = static_cast<double*>(malloc(static_cast<size_t>(r1) * c2 * sizeof(double)));

        if (!firstmat || !secondmat || !resultmat)
        {
            freeMatrices();
            throw runtime_error("Matrix memory allocation failed.");
        }
    }

    void freeMatrices()
    {
        free(firstmat);
        free(secondmat);
        free(resultmat);

        firstmat = nullptr;
        secondmat = nullptr;
        resultmat = nullptr;
    }

    void fillmatrix(int rows, int cols, double* matrix)
    {
        if (rows <= 0 || cols <= 0 || matrix == nullptr)
        {
            throw invalid_argument("Invalid matrix passed to fillmatrix().");
        }

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                matrix[static_cast<size_t>(i) * cols + j] = rand() % 10;
            }
        }
    }

    // CPU: no threads. multiplyRows contains the naive i-j-k loops.
    void matrixmultiplication(double& seconds)
    {
        seconds = 0.0;
        checkMatrices();

        auto start = chrono::steady_clock::now();

        multiplyRows(0, r1);

        auto end = chrono::steady_clock::now();
        seconds = chrono::duration<double>(end - start).count();
    }

    // CPU: pthreads API.
    void matrixmultiplicationPthreads(int threads, double& seconds)
    {
        seconds = 0.0;
        checkMatrices();
        checkThreadCount(threads);

        // A thread needs at least one row to calculate.
        threads = min(threads, r1);

        vector<pthread_t> threadIDs(threads);
        vector<ThreadData> threadData(threads);

        int created = 0;
        int error = 0;
        int nextRow = 0;
        int rowsPerThread = r1 / threads;
        int extraRows = r1 % threads;

        auto start = chrono::steady_clock::now();

        for (int i = 0; i < threads; i++)
        {
            int rowCount = rowsPerThread;

            if (i < extraRows)
            {
                rowCount++;
            }

            threadData[i].object = this;
            threadData[i].firstRow = nextRow;
            threadData[i].endRow = nextRow + rowCount;
            nextRow += rowCount;

            error = pthread_create(&threadIDs[i], nullptr, pthreadWorker, &threadData[i]);

            if (error != 0)
            {
                break;
            }

            created++;
        }

        // Joining finishes all writes before results are read or freed.
        for (int i = 0; i < created; i++)
        {
            int joinError = pthread_join(threadIDs[i], nullptr);

            if (joinError != 0)
            {
                // Do not free memory if a worker might still be using it.
                cerr << "pthread_join failed: " << strerror(joinError) << '\n';
                terminate();
            }
        }

        auto end = chrono::steady_clock::now();

        if (error != 0)
        {
            throw runtime_error(string("pthread_create failed: ") + strerror(error));
        }

        seconds = chrono::duration<double>(end - start).count();
    }

    static void* pthreadWorker(void* data)
    {
        ThreadData* task = static_cast<ThreadData*>(data);
        task->object->multiplyRows(task->firstRow, task->endRow);

        return nullptr;
    }

    void multiplyRows(int firstRow, int endRow)
    {
        for (int i = firstRow; i < endRow; i++)
        {
            for (int j = 0; j < c2; j++)
            {
                // Each worker has its own sum.
                double sum = 0.0;

                for (int k = 0; k < c1; k++)
                {
                    sum += firstmat[static_cast<size_t>(i) * c1 + k] * secondmat[static_cast<size_t>(k) * c2 + j];
                }

                // Only the worker assigned this row writes here.
                resultmat[static_cast<size_t>(i) * c2 + j] = sum;
            }
        }
    }

    // CPU, OpenMP Version. Each loop iteration owns a different result row.
    void matrixmultiplicationOpenMP(int threads, double& seconds)
    {
        seconds = 0.0;
        checkMatrices();
        checkThreadCount(threads);
        threads = min(threads, r1);

        auto start = chrono::steady_clock::now();

        #pragma omp parallel for num_threads(threads) schedule(static)
        for (int i = 0; i < r1; i++)
        {
            multiplyRows(i, i + 1);
        }

        // The parallel loop waits for all workers before continuing.
        auto end = chrono::steady_clock::now();
        seconds = chrono::duration<double>(end - start).count();
    }

    // GPU: the normal C++ interface to the separate .cu implementation.
    void matrixmultiplicationCUDA(double& seconds)
    {
        seconds = 0.0;
        checkMatrices();

        #ifdef CALCULATOR_USE_CUDA
        multiplyMatricesCUDA(r1, c1, c2, firstmat, secondmat, resultmat, seconds);
        #else
        throw runtime_error("CUDA is not enabled in this build.");
        #endif
    }

    // CLI output for now; later this can write to the GUI.
    void showResult(int rows, int cols, const double* matrix, double seconds)
    {
        cout << fixed << setprecision(9);
        cout << "Time: " << seconds << " seconds\n";
        printmatrix("Result", rows, cols, matrix);
    }

    void showError(const char* message)
    {
        cerr << "Error: " << message << '\n';
    }

    void printmatrix(const char* name, int rows, int cols, const double* matrix)
    {
        int shownRows = min(rows, 8);
        int shownCols = min(cols, 8);

        cout << name << " (" << rows << " x " << cols << "):\n";

        for (int i = 0; i < shownRows; i++)
        {
            cout << "[ ";

            for (int j = 0; j < shownCols; j++)
            {
                cout << fixed << setprecision(0) << setw(8)
                     << matrix[static_cast<size_t>(i) * cols + j] << ' ';
            }

            if (cols > shownCols)
            {
                cout << "... ";
            }

            cout << "]\n";
        }

        if (rows > shownRows)
        {
            cout << "...\n";
        }

        cout << '\n';
    }

    void checkResult(const double* expected, bool& same)
    {
        same = true;
        size_t count = static_cast<size_t>(r1) * c2;

        for (size_t i = 0; i < count; i++)
        {
            double tolerance = 1e-9 * max(1.0, fabs(expected[i]));

            if (!isfinite(resultmat[i]) ||
                fabs(resultmat[i] - expected[i]) > tolerance)
            {
                same = false;
                break;
            }
        }
    }

    // Temporary CLI driver, before adding the GUI.
    void runCLI()
    {
        readInput(r1, c1, r2, c2, threadCount);
        allocateMatrices();

        srand(1); // Repeatable input for testing.
        fillmatrix(r1, c1, firstmat);
        fillmatrix(r2, c2, secondmat);

        cout << "\nCPU thread limit: " << threadCount << '\n';
        cout << "Threads are limited to the number of result rows.\n\n";

        printmatrix("First matrix", r1, c1, firstmat);
        printmatrix("Second matrix", r2, c2, secondmat);

        double serialSeconds = 0.0;
        double pthreadSeconds = 0.0;
        double openmpSeconds = 0.0;

        cout << "Serial CPU\n";
        matrixmultiplication(serialSeconds);
        showResult(r1, c2, resultmat, serialSeconds);

        // Keep the serial result so both parallel versions can be checked.
        size_t count = static_cast<size_t>(r1) * c2;
        vector<double> expected(resultmat, resultmat + count);

        // NaN makes skipped output cells fail verification.
        fill(resultmat, resultmat + count, numeric_limits<double>::quiet_NaN());

        cout << "Pthreads CPU\n";
        matrixmultiplicationPthreads(threadCount, pthreadSeconds);
        showResult(r1, c2, resultmat, pthreadSeconds);

        bool pthreadSame = false;
        checkResult(expected.data(), pthreadSame);
        cout << "Pthreads matches serial: " << (pthreadSame ? "YES" : "NO") << "\n\n";

        fill(resultmat, resultmat + count, numeric_limits<double>::quiet_NaN());

        cout << "OpenMP CPU\n";
        matrixmultiplicationOpenMP(threadCount, openmpSeconds);
        showResult(r1, c2, resultmat, openmpSeconds);

        bool openmpSame = false;
        checkResult(expected.data(), openmpSame);
        cout << "OpenMP matches serial: " << (openmpSame ? "YES" : "NO") << "\n\n";

        if (!pthreadSame || !openmpSame)
        {
            throw runtime_error("Parallel result verification failed.");
        }

        #ifdef CALCULATOR_USE_CUDA
        fill(resultmat, resultmat + count, numeric_limits<double>::quiet_NaN());

        double cudaSeconds = 0.0;
        cout << "CUDA GPU (includes allocation and data transfers)\n";
        matrixmultiplicationCUDA(cudaSeconds);
        showResult(r1, c2, resultmat, cudaSeconds);

        bool cudaSame = false;
        checkResult(expected.data(), cudaSame);
        cout << "CUDA matches serial: " << (cudaSame ? "YES" : "NO") << "\n\n";

        if (!cudaSame)
        {
            throw runtime_error("CUDA result verification failed.");
        }
        #endif

        cout << fixed << setprecision(3);

        if (pthreadSeconds > 0.0)
        {
            cout << "Pthreads speedup: " << serialSeconds / pthreadSeconds << "x\n";
        }

        if (openmpSeconds > 0.0)
        {
            cout << "OpenMP speedup: " << serialSeconds / openmpSeconds << "x\n";
        }

        #ifdef CALCULATOR_USE_CUDA
        if (cudaSeconds > 0.0)
        {
            cout << "CUDA speedup: " << serialSeconds / cudaSeconds << "x\n";
        }
        #endif
    }

private:
    struct ThreadData
    {
        calculator* object;
        int firstRow;
        int endRow;
    };

    void checkMatrices()
    {
        if (r1 <= 0 || c1 <= 0 || r2 <= 0 || c2 <= 0 || c1 != r2 ||
            firstmat == nullptr || secondmat == nullptr || resultmat == nullptr)
        {
            throw invalid_argument("Allocate valid matrices before calculating.");
        }
    }

    void checkThreadCount(int threads)
    {
        if (threads < 1 || threads > 32)
        {
            throw invalid_argument("Use between 1 and 32 CPU threads.");
        }
    }

    int r1 = 0;
    int c1 = 0;
    int r2 = 0;
    int c2 = 0;
    int threadCount = 16;

    double* firstmat = nullptr;
    double* secondmat = nullptr;
    double* resultmat = nullptr;
};

