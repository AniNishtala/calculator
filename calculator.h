#pragma once

class calculator
{
public:
    // GUI

    void createWindow()
    {
        // Implementation required.
    }

    void createInputBoxes()
    {
        // Implementation required. matrix sizes + thread count
    }

    void createCPUButton()
    {
        // Implementation required. connect to cpuButtonClicked().
    }

    void createPthreadsButton()
    {
        // Implementation required. connect to pthreadsButtonClicked().
    }

    void createOpenMPButton()
    {
        // Implementation required. connect to openmpButtonClicked().
    }

    void createCUDAButton()
    {
        // Implementation required. connect to cudaButtonClicked().
    }

    void createClearButton()
    {
        // Implementation required. connect to clearButtonClicked().
    }

    void createResultBox()
    {
        // Implementation required. show the matrix and execution time.
    }

    void runGUI()
    {
        // Implementation required. show the window and handle clicks.
    }

    void closeGUI()
    {
        // Implementation required.
    }

    // Button actions

    void cpuButtonClicked()
    {
        // Implementation required. run matrixmultiplication().
    }

    void pthreadsButtonClicked()
    {
        // Implementation required. run matrixmultiplicationPthreads().
    }

    void openmpButtonClicked()
    {
        // Implementation required. run matrixmultiplicationOpenMP().
    }

    void cudaButtonClicked()
    {
        // Implementation required. run matrixmultiplicationCUDA().
    }

    void clearButtonClicked()
    {
        // Implementation required. clear inputs and displayed results.
    }

    // Read and check input

    void readInput(int& r1,int& c1,int& r2,int& c2,int& threads)
    {
        // Implementation required. read values from the GUI.
    }

    void checkInput(bool& valid)
    {
        // Implementation required. check sizes, c1 == r2, and a positive thread count.
    }

    // Matrix memory

    void allocateMatrices()
    {
        // Implementation required. allocate firstmat, secondmat, and resultmat.
    }

    void freeMatrices()
    {
        // Implementation required. release allocated memory.
    }

    void fillmatrix(int rows, int cols, double* matrix)
    {
        // Implementation required. same purpose as your C function.
    }

    // CPU: no threads

    void matrixmultiplication(double& seconds)
    {
        // Implementation required. multiply the matrices with regular loops and record elapsed time.
    }

    // CPU: pthreads API

    void matrixmultiplicationPthreads(int threads,double& seconds)
    {
        // Implementation required. use pthread_create() and pthread_join(), then record elapsed time.
    }

    static void* pthreadWorker(void* data)
    {
        // Implementation required. each thread calculates its assigned rows. A return value is also required.
    }

    void multiplyRows(int firstRow, int endRow)
    {
        // Implementation required. calculate rows from firstRow up to, but not including, endRow.
    }

    // CPU: OpenMP

    void matrixmultiplicationOpenMP(int threads,double& seconds)
    {
        // Implementation required. multiply using OpenMP and record elapsed time.
    }

    // GPU: CUDA code. Refer to code from CMPE214

    void matrixmultiplicationCUDA(double& seconds)
    {
        // Implementation required. allocate GPU memory, copy inputs, launch your CUDA kernel, copy the result back, and release GPU memory.
    }

    // Display

    void showResult(int rows,int cols, const double* matrix, double seconds)
    {
        // Implementation required. display in the GUI.
    }

    void showError(const char* message)
    {
        // Implementation required.
    }

private:
    int r1 = 0;
    int c1 = 0;
    int r2 = 0;
    int c2 = 0;
    int threadCount = 16;

    double* firstmat = nullptr;
    double* secondmat = nullptr;
    double* resultmat = nullptr;
};
