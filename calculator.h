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
        stopArithmeticPool();
        freeMatrices();
    }

    calculator(const calculator&) = delete;
    calculator& operator=(const calculator&) = delete;

    // GUI functions are still placeholders.
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

    // CLI input for now. Parameters use call by reference.
    void readInput(int& rows1, int& cols1, int& rows2, int& cols2, int& threads)
    {
        cout << "First matrix: enter rows and columns: ";
        if (!(cin >> rows1 >> cols1))
        {
            throw invalid_argument("Enter whole numbers for matrix sizes.");
        }

        cout << "Second matrix: enter rows and columns: ";
        if (!(cin >> rows2 >> cols2))
        {
            throw invalid_argument("Enter whole numbers for matrix sizes.");
        }

        cout << "CPU threads (1-32): ";
        if (!(cin >> threads))
        {
            throw invalid_argument("Enter a whole number for threads.");
        }
    }

    void checkInput(bool& valid)
    {
        valid = r1 > 0 && c1 > 0 && r2 > 0 && c2 > 0 &&
                threadCount >= 1 && threadCount <= 32;

        if (operation == 2)
        {
            valid = valid && c1 == r2;
        }
        else if (operation == 3 || operation == 4)
        {
            valid = valid && r1 == r2 && c1 == c2;
        }
        else
        {
            valid = false;
        }
    }

    void allocateMatrices()
    {
        bool valid = false;
        checkInput(valid);

        if (!valid)
        {
            throw invalid_argument(
                "Use positive sizes and 1-32 threads. Multiplication needs c1 == r2; "
                "addition and dot product need matching matrix shapes.");
        }

        resultRows = operation == 4 ? 1 : r1;
        resultCols = operation == 4 ? 1 : (operation == 2 ? c2 : c1);

        size_t maxElements = numeric_limits<size_t>::max() / sizeof(double);
        if (static_cast<size_t>(r1) > maxElements / c1 ||
            static_cast<size_t>(r2) > maxElements / c2 ||
            static_cast<size_t>(resultRows) > maxElements / resultCols)
        {
            throw length_error("Matrix sizes are too large.");
        }

        freeMatrices();
        firstmat = static_cast<double*>(malloc(static_cast<size_t>(r1) * c1 * sizeof(double)));
        secondmat = static_cast<double*>(malloc(static_cast<size_t>(r2) * c2 * sizeof(double)));
        resultmat = static_cast<double*>(malloc(resultCount() * sizeof(double)));

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
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                matrix[static_cast<size_t>(i) * cols + j] = rand() % 10;
            }
        }
    }

    void readmatrix(const char* name, int rows, int cols, double* matrix)
    {
        cout << "Enter " << name << " values row by row ("
             << rows << " x " << cols << "):\n";

        size_t count = static_cast<size_t>(rows) * cols;
        for (size_t i = 0; i < count; i++)
        {
            if (!(cin >> matrix[i]) || !isfinite(matrix[i]))
            {
                throw invalid_argument("Enter finite numbers for matrix values.");
            }
        }
    }

    // Naive matrix multiplication: i-j-k loops, as in your C code.
    void multiplyRows(int firstRow, int endRow)
    {
        for (int i = firstRow; i < endRow; i++)
        {
            for (int j = 0; j < c2; j++)
            {
                double sum = 0.0;
                for (int k = 0; k < c1; k++)
                {
                    sum += firstmat[static_cast<size_t>(i) * c1 + k] *
                           secondmat[static_cast<size_t>(k) * c2 + j];
                }
                resultmat[static_cast<size_t>(i) * c2 + j] = sum;
            }
        }
    }

    void addRows(int firstRow, int endRow)
    {
        for (int i = firstRow; i < endRow; i++)
        {
            for (int j = 0; j < c1; j++)
            {
                size_t index = static_cast<size_t>(i) * c1 + j;
                resultmat[index] = firstmat[index] + secondmat[index];
            }
        }
    }

    // Matrix dot product means sum(A[i][j] * B[i][j]), producing one number.
    double dotRows(int firstRow, int endRow)
    {
        double sum = 0.0;
        for (int i = firstRow; i < endRow; i++)
        {
            for (int j = 0; j < c1; j++)
            {
                size_t index = static_cast<size_t>(i) * c1 + j;
                sum += firstmat[index] * secondmat[index];
            }
        }
        return sum;
    }

    void computeSerial(double& seconds)
    {
        checkMatrices();
        auto start = chrono::steady_clock::now();

        if (operation == 2)
        {
            multiplyRows(0, r1);
        }
        else if (operation == 3)
        {
            addRows(0, r1);
        }
        else
        {
            resultmat[0] = dotRows(0, r1);
        }

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    void computePthreads(int threads, double& seconds)
    {
        checkMatrices();
        checkThreadCount(threads);
        threads = min(threads, r1);

        vector<pthread_t> threadIDs(threads);
        vector<ThreadData> data(threads);
        int created = 0;
        int error = 0;
        int nextRow = 0;
        auto start = chrono::steady_clock::now();

        for (int i = 0; i < threads; i++)
        {
            int rows = r1 / threads + (i < r1 % threads ? 1 : 0);
            data[i].object = this;
            data[i].firstRow = nextRow;
            data[i].endRow = nextRow + rows;
            data[i].partialSum = 0.0;
            nextRow += rows;

            error = pthread_create(&threadIDs[i], nullptr, pthreadWorker, &data[i]);
            if (error != 0)
            {
                break;
            }
            created++;
        }

        for (int i = 0; i < created; i++)
        {
            if (pthread_join(threadIDs[i], nullptr) != 0)
            {
                terminate();
            }
        }

        if (error != 0)
        {
            throw runtime_error(string("pthread_create failed: ") + strerror(error));
        }

        if (operation == 4)
        {
            // Workers write separate partial sums. Only this thread combines them.
            resultmat[0] = 0.0;
            for (int i = 0; i < threads; i++)
            {
                resultmat[0] += data[i].partialSum;
            }
        }

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    static void* pthreadWorker(void* argument)
    {
        ThreadData* data = static_cast<ThreadData*>(argument);
        calculator* object = data->object;

        if (object->operation == 2)
        {
            object->multiplyRows(data->firstRow, data->endRow);
        }
        else if (object->operation == 3)
        {
            object->addRows(data->firstRow, data->endRow);
        }
        else
        {
            data->partialSum = object->dotRows(data->firstRow, data->endRow);
        }
        return nullptr;
    }

    void computeOpenMP(int threads, double& seconds)
    {
        checkMatrices();
        checkThreadCount(threads);
        threads = min(threads, r1);
        auto start = chrono::steady_clock::now();

        if (operation == 4)
        {
            double sum = 0.0;

            // Each thread gets a private sum; OpenMP combines them safely.
            #pragma omp parallel for num_threads(threads) schedule(static) reduction(+:sum)
            for (int i = 0; i < r1; i++)
            {
                sum += dotRows(i, i + 1);
            }
            resultmat[0] = sum;
        }
        else
        {
            #pragma omp parallel for num_threads(threads) schedule(static)
            for (int i = 0; i < r1; i++)
            {
                if (operation == 2)
                {
                    multiplyRows(i, i + 1);
                }
                else
                {
                    addRows(i, i + 1);
                }
            }
        }

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    void computeCUDA(double& seconds)
    {
        checkMatrices();

        #ifdef CALCULATOR_USE_CUDA
        if (operation == 2)
        {
            multiplyMatricesCUDA(r1, c1, c2, firstmat, secondmat, resultmat, seconds);
        }
        else if (operation == 3)
        {
            addMatricesCUDA(r1, c1, firstmat, secondmat, resultmat, seconds);
        }
        else
        {
            dotMatricesCUDA(r1, c1, firstmat, secondmat, resultmat[0], seconds);
        }
        #else
        seconds = 0.0;
        throw runtime_error("CUDA is not enabled in this build.");
        #endif
    }

    // Keep the original matrix multiplication function names.
    void matrixmultiplication(double& seconds)
    {
        requireOperation(2);
        computeSerial(seconds);
    }

    void matrixmultiplicationPthreads(int threads, double& seconds)
    {
        requireOperation(2);
        computePthreads(threads, seconds);
    }

    void matrixmultiplicationOpenMP(int threads, double& seconds)
    {
        requireOperation(2);
        computeOpenMP(threads, seconds);
    }

    void matrixmultiplicationCUDA(double& seconds)
    {
        requireOperation(2);
        computeCUDA(seconds);
    }

    // Scalar expressions run on the CPU; starting threads would add overhead.
    void calculateExpression(const string& text, double& result)
    {
        expression = text;
        position = 0;
        result = parseExpression();
        skipSpaces();

        if (position != expression.size())
        {
            throw invalid_argument("Unexpected text in expression.");
        }
        if (!isfinite(result))
        {
            throw invalid_argument("Result is outside the finite real-number range.");
        }
    }

    // Split one input line into independent expressions.
    void splitExpressions(const string& line, vector<string>& inputs)
    {
        inputs.clear();
        size_t first = 0;

        while (true)
        {
            size_t end = line.find(';', first);
            string part = line.substr(first, end == string::npos ? string::npos : end - first);
            size_t beginText = part.find_first_not_of(" \t\r\n");

            if (beginText == string::npos)
            {
                // An optional trailing semicolon is allowed.
                if (end == string::npos && !inputs.empty())
                {
                    break;
                }
                throw invalid_argument("Enter complete expressions separated by ; without empty entries.");
            }

            size_t endText = part.find_last_not_of(" \t\r\n");
            inputs.push_back(part.substr(beginText, endText - beginText + 1));

            if (end == string::npos)
            {
                break;
            }
            first = end + 1;
        }
    }

    // Evaluate independent expressions one after another.
    void arithmeticSerial(const vector<string>& inputs,
        vector<double>& results, double& seconds)
    {
        auto start = chrono::steady_clock::now();
        results.assign(inputs.size(), 0.0);

        for (size_t i = 0; i < inputs.size(); i++)
        {
            calculateExpression(inputs[i], results[i]);
        }

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    // Reuse a fixed team for successive batches with the same worker count.
    // Submit one batch at a time per calculator object.
    void arithmeticPthreads(const vector<string>& inputs, int threads,
        vector<double>& results, double& seconds)
    {
        checkThreadCount(threads);
        auto start = chrono::steady_clock::now();
        results.assign(inputs.size(), 0.0);
        seconds = 0.0;
        if (inputs.empty())
        {
            return;
        }

        threads = static_cast<int>(min(static_cast<size_t>(threads), inputs.size()));
        ensureArithmeticPool(threads);
        ArithmeticPool* pool = arithmeticPool;
        // The previous batch is complete. Allocate before acquiring the mutex,
        // so an allocation failure cannot leave the pool locked.
        pool->errors.assign(inputs.size(), exception_ptr());

        pthread_mutex_lock(&pool->mutex);
        pool->inputs = &inputs;
        pool->results = &results;
        pool->finished = 0;
        pool->generation++;

        // Publish the whole batch, then release the mutex while waiting.
        pthread_cond_broadcast(&pool->jobReady);
        while (pool->finished != pool->workers.size())
        {
            pthread_cond_wait(&pool->jobDone, &pool->mutex);
        }
        pool->inputs = nullptr;
        pool->results = nullptr;
        pthread_mutex_unlock(&pool->mutex);

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
        checkArithmeticErrors(pool->errors);
    }

    void arithmeticOpenMP(const vector<string>& inputs, int threads,
        vector<double>& results, double& seconds)
    {
        checkThreadCount(threads);
        auto start = chrono::steady_clock::now();
        results.assign(inputs.size(), 0.0);
        seconds = 0.0;
        if (inputs.empty())
        {
            return;
        }

        threads = static_cast<int>(min(static_cast<size_t>(threads), inputs.size()));
        openmpArithmeticErrors.assign(inputs.size(), exception_ptr());

        #pragma omp parallel num_threads(threads)
        {
            // Retain each worker's parser and string capacity between batches.
            static thread_local calculator parser;

            // The parallel-region end already waits for all results.
            #pragma omp for schedule(static) nowait
            for (size_t i = 0; i < inputs.size(); i++)
            {
                try
                {
                    parser.calculateExpression(inputs[i], results[i]);
                }
                catch (...)
                {
                    // Exceptions cannot escape the OpenMP worker region.
                    openmpArithmeticErrors[i] = current_exception();
                }
            }
        }

        seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
        checkArithmeticErrors(openmpArithmeticErrors);
    }

    void showResult(int rows, int cols, const double* matrix, double seconds)
    {
        cout << fixed << setprecision(9) << "Time: " << seconds << " seconds\n";
        printmatrix("Result", rows, cols, matrix);
    }

    void showError(const char* message)
    {
        cerr << "Error: " << message << '\n';
    }

    void printmatrix(const char* name, int rows, int cols, const double* matrix)
    {
        cout << name << " (" << rows << " x " << cols << "):\n";
        cout << defaultfloat << setprecision(10);

        for (int i = 0; i < min(rows, 8); i++)
        {
            cout << "[ ";
            for (int j = 0; j < min(cols, 8); j++)
            {
                cout << matrix[static_cast<size_t>(i) * cols + j] << ' ';
            }
            if (cols > 8)
            {
                cout << "... ";
            }
            cout << "]\n";
        }
        if (rows > 8)
        {
            cout << "...\n";
        }
        cout << '\n';
    }

    void checkResult(const double* expected, bool& same)
    {
        same = true;
        for (size_t i = 0; i < resultCount(); i++)
        {
            double tolerance = 1e-9 * max(1.0, fabs(expected[i]));
            if (!isfinite(expected[i]) || !isfinite(resultmat[i]) ||
                fabs(resultmat[i] - expected[i]) > tolerance)
            {
                same = false;
                break;
            }
        }
    }

    void runCLI()
    {
        srand(1);

        while (true)
        {
            cout << "\n1. Arithmetic expression\n"
                 << "2. Matrix multiplication\n"
                 << "3. Matrix addition\n"
                 << "4. Matrix dot product (sum of matching element products)\n"
                 << "5. One-line batch arithmetic (separate expressions with ;)\n"
                 << "0. Exit\nChoice: ";

            if (!(cin >> operation))
            {
                if (cin.eof())
                {
                    return;
                }
                throw invalid_argument("Enter a menu number.");
            }
            if (operation == 0)
            {
                return;
            }
            if (operation == 1)
            {
                cout << "Expression (trig functions use radians): ";
                string text;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (!getline(cin, text))
                {
                    throw invalid_argument("Enter an expression.");
                }
                double result = 0.0;
                calculateExpression(text, result);
                cout << defaultfloat << setprecision(12) << "Answer: " << result << '\n';
            }
            else if (operation >= 2 && operation <= 4)
            {
                runMatrixCLI();
            }
            else if (operation == 5)
            {
                runArithmeticCLI();
            }
            else
            {
                throw invalid_argument("Choose a menu number from 0 to 5.");
            }
        }
    }

private:
    struct ArithmeticPool;

    struct ArithmeticWorkerData
    {
        ArithmeticPool* pool;
        size_t worker;
    };

    struct ArithmeticPool
    {
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t jobReady = PTHREAD_COND_INITIALIZER;
        pthread_cond_t jobDone = PTHREAD_COND_INITIALIZER;
        vector<pthread_t> workers;
        vector<ArithmeticWorkerData> data;
        vector<exception_ptr> errors;
        const vector<string>* inputs = nullptr;
        vector<double>* results = nullptr;
        size_t generation = 0;
        size_t ready = 0;
        size_t finished = 0;
        bool stopping = false;

        ~ArithmeticPool()
        {
            pthread_cond_destroy(&jobReady);
            pthread_cond_destroy(&jobDone);
            pthread_mutex_destroy(&mutex);
        }
    };

    void ensureArithmeticPool(int threads)
    {
        if (arithmeticPool && arithmeticPool->workers.size() == static_cast<size_t>(threads))
        {
            return;
        }
        stopArithmeticPool();

        unique_ptr<ArithmeticPool> pool(new ArithmeticPool);
        pool->workers.resize(threads);
        pool->data.resize(threads);
        int created = 0;
        int error = 0;

        for (int i = 0; i < threads; i++)
        {
            pool->data[i].pool = pool.get();
            pool->data[i].worker = static_cast<size_t>(i);
            error = pthread_create(&pool->workers[i], nullptr,
                arithmeticPoolWorker, &pool->data[i]);
            if (error != 0)
            {
                break;
            }
            created++;
        }

        if (error != 0)
        {
            pthread_mutex_lock(&pool->mutex);
            pool->stopping = true;
            pthread_cond_broadcast(&pool->jobReady);
            pthread_mutex_unlock(&pool->mutex);
            for (int i = 0; i < created; i++)
            {
                if (pthread_join(pool->workers[i], nullptr) != 0)
                {
                    terminate();
                }
            }
            throw runtime_error(string("pthread_create failed: ") + strerror(error));
        }

        // Ensure all parsers are initialized before publishing the first job.
        pthread_mutex_lock(&pool->mutex);
        while (pool->ready != pool->workers.size())
        {
            pthread_cond_wait(&pool->jobDone, &pool->mutex);
        }
        pthread_mutex_unlock(&pool->mutex);
        arithmeticPool = pool.release();
    }

    void stopArithmeticPool()
    {
        if (!arithmeticPool)
        {
            return;
        }
        pthread_mutex_lock(&arithmeticPool->mutex);
        arithmeticPool->stopping = true;
        pthread_cond_broadcast(&arithmeticPool->jobReady);
        pthread_mutex_unlock(&arithmeticPool->mutex);

        for (size_t i = 0; i < arithmeticPool->workers.size(); i++)
        {
            if (pthread_join(arithmeticPool->workers[i], nullptr) != 0)
            {
                terminate();
            }
        }
        delete arithmeticPool;
        arithmeticPool = nullptr;
    }

    static void* arithmeticPoolWorker(void* argument)
    {
        ArithmeticWorkerData* data = static_cast<ArithmeticWorkerData*>(argument);
        ArithmeticPool* pool = data->pool;
        calculator parser;
        size_t seen = 0;

        pthread_mutex_lock(&pool->mutex);
        pool->ready++;
        pthread_cond_signal(&pool->jobDone);

        while (true)
        {
            // Predicate loops handle early signals and spurious wakeups.
            while (!pool->stopping && pool->generation == seen)
            {
                pthread_cond_wait(&pool->jobReady, &pool->mutex);
            }
            if (pool->stopping)
            {
                break;
            }
            seen = pool->generation;
            const vector<string>* inputs = pool->inputs;
            vector<double>* results = pool->results;
            size_t workers = pool->workers.size();
            size_t each = inputs->size() / workers;
            size_t extra = inputs->size() % workers;
            size_t first = data->worker * each + min(data->worker, extra);
            size_t end = first + each + (data->worker < extra ? 1 : 0);
            pthread_mutex_unlock(&pool->mutex);

            // No locking inside the calculation loop: each slot has one owner.
            for (size_t i = first; i < end; i++)
            {
                try
                {
                    parser.calculateExpression((*inputs)[i], (*results)[i]);
                }
                catch (...)
                {
                    pool->errors[i] = current_exception();
                }
            }

            pthread_mutex_lock(&pool->mutex);
            pool->finished++;
            if (pool->finished == workers)
            {
                pthread_cond_signal(&pool->jobDone);
            }
        }

        pthread_mutex_unlock(&pool->mutex);
        return nullptr;
    }

    void checkArithmeticErrors(const vector<exception_ptr>& errors)
    {
        // Run after all workers have finished; report the first failed expression.
        for (size_t i = 0; i < errors.size(); i++)
        {
            if (errors[i])
            {
                rethrow_exception(errors[i]);
            }
        }
    }

    bool sameArithmeticResults(const vector<double>& expected,
        const vector<double>& actual)
    {
        if (expected.size() != actual.size())
        {
            return false;
        }
        for (size_t i = 0; i < expected.size(); i++)
        {
            double tolerance = 1e-9 * max(1.0, fabs(expected[i]));
            if (!isfinite(actual[i]) || fabs(expected[i] - actual[i]) > tolerance)
            {
                return false;
            }
        }
        return true;
    }

    double medianTime(vector<double> samples)
    {
        sort(samples.begin(), samples.end());
        return samples[samples.size() / 2];
    }

    void runArithmeticCLI()
    {
        cout << "CPU threads (1-32): ";
        if (!(cin >> threadCount))
        {
            throw invalid_argument("Enter a whole number for threads.");
        }
        checkThreadCount(threadCount);

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter expressions on ONE line, separated by ; (trig functions use radians):\n";
        string line;
        if (!getline(cin, line))
        {
            throw invalid_argument("Enter a line of expressions.");
        }
        vector<string> inputs;
        splitExpressions(line, inputs);
        size_t count = inputs.size();
        cout << "Expressions parsed: " << count << '\n';

        vector<double> serialResults;
        vector<double> pthreadResults;
        vector<double> openmpResults;
        double serialSeconds = 0.0;
        double pthreadSeconds = 0.0;
        double openmpSeconds = 0.0;

        arithmeticSerial(inputs, serialResults, serialSeconds);
        arithmeticPthreads(inputs, threadCount, pthreadResults, pthreadSeconds);
        arithmeticOpenMP(inputs, threadCount, openmpResults, openmpSeconds);

        double initialSerial = serialSeconds;
        double initialPthreads = pthreadSeconds;
        double initialOpenMP = openmpSeconds;
        const vector<double> expected = serialResults;
        if (!sameArithmeticResults(expected, pthreadResults) ||
            !sameArithmeticResults(expected, openmpResults))
        {
            throw runtime_error("Initial batch result verification failed.");
        }

        // These are explicitly repeated benchmark runs of the same input.
        // Keep the initial pass visible; do not hide worker startup in a speedup.
        const int repeats = 7;
        vector<double> serialTimes;
        vector<double> pthreadTimes;
        vector<double> openmpTimes;
        serialTimes.reserve(repeats);
        pthreadTimes.reserve(repeats);
        openmpTimes.reserve(repeats);

        for (int run = 0; run < repeats; run++)
        {
            // Rotate method order to reduce consistent first/last-run bias.
            for (int turn = 0; turn < 3; turn++)
            {
                int method = (run + turn) % 3;
                if (method == 0)
                {
                    arithmeticSerial(inputs, serialResults, serialSeconds);
                    serialTimes.push_back(serialSeconds);
                    if (!sameArithmeticResults(expected, serialResults))
                    {
                        throw runtime_error("Serial benchmark result verification failed.");
                    }
                }
                else if (method == 1)
                {
                    arithmeticPthreads(inputs, threadCount, pthreadResults, pthreadSeconds);
                    pthreadTimes.push_back(pthreadSeconds);
                    if (!sameArithmeticResults(expected, pthreadResults))
                    {
                        throw runtime_error("Pthreads benchmark result verification failed.");
                    }
                }
                else
                {
                    arithmeticOpenMP(inputs, threadCount, openmpResults, openmpSeconds);
                    openmpTimes.push_back(openmpSeconds);
                    if (!sameArithmeticResults(expected, openmpResults))
                    {
                        throw runtime_error("OpenMP benchmark result verification failed.");
                    }
                }
            }
        }

        serialSeconds = medianTime(serialTimes);
        pthreadSeconds = medianTime(pthreadTimes);
        openmpSeconds = medianTime(openmpTimes);

        cout << "\nBatch arithmetic results (up to 8 shown):\n";
        cout << defaultfloat << setprecision(12);
        for (size_t i = 0; i < min(count, static_cast<size_t>(8)); i++)
        {
            cout << inputs[i] << " = " << serialResults[i] << '\n';
        }

        bool pthreadSame = sameArithmeticResults(serialResults, pthreadResults);
        bool openmpSame = sameArithmeticResults(serialResults, openmpResults);
        cout << "Pthreads matches serial: " << (pthreadSame ? "YES" : "NO") << '\n';
        cout << "OpenMP matches serial: " << (openmpSame ? "YES" : "NO") << '\n';
        if (!pthreadSame || !openmpSame)
        {
            throw runtime_error("Batch arithmetic result verification failed.");
        }

        cout << fixed << setprecision(9);
        cout << "Workers used: " << min(static_cast<size_t>(threadCount), count) << '\n';
        cout << "Initial batch pass (includes any required worker setup):\n";
        cout << "Initial serial time: " << initialSerial << " seconds\n";
        cout << "Initial pthreads time: " << initialPthreads << " seconds\n";
        cout << "Initial OpenMP time: " << initialOpenMP << " seconds\n";
        cout << "Repeated-run medians (" << repeats << " runs; workers reused):\n";
        cout << "Serial arithmetic time: " << serialSeconds << " seconds\n";
        cout << "Pthreads arithmetic time: " << pthreadSeconds << " seconds\n";
        cout << "OpenMP arithmetic time: " << openmpSeconds << " seconds\n";
        cout << "Speedups below compare the repeated-run medians.\n";
        cout << fixed << setprecision(3);
        if (pthreadSeconds > 0.0)
        {
            cout << "Pthreads speedup: " << serialSeconds / pthreadSeconds << "x\n";
        }
        if (openmpSeconds > 0.0)
        {
            cout << "OpenMP speedup: " << serialSeconds / openmpSeconds << "x\n";
        }
    }

    struct ThreadData
    {
        calculator* object;
        int firstRow;
        int endRow;
        double partialSum;
    };

    void runMatrixCLI()
    {
        readInput(r1, c1, r2, c2, threadCount);
        allocateMatrices();

        cout << "Matrix values: 1 = enter your own, 2 = random: ";
        int mode = 0;
        if (!(cin >> mode) || (mode != 1 && mode != 2))
        {
            throw invalid_argument("Choose 1 or 2 for matrix values.");
        }
        if (mode == 1)
        {
            readmatrix("first matrix", r1, c1, firstmat);
            readmatrix("second matrix", r2, c2, secondmat);
        }
        else
        {
            fillmatrix(r1, c1, firstmat);
            fillmatrix(r2, c2, secondmat);
        }

        cout << "\nCPU thread limit: " << min(threadCount, r1) << "\n\n";
        printmatrix("First matrix", r1, c1, firstmat);
        printmatrix("Second matrix", r2, c2, secondmat);

        double serialSeconds = 0.0;
        double pthreadSeconds = 0.0;
        double openmpSeconds = 0.0;

        cout << "Serial CPU\n";
        computeSerial(serialSeconds);
        showResult(resultRows, resultCols, resultmat, serialSeconds);
        vector<double> expected(resultmat, resultmat + resultCount());

        fill(resultmat, resultmat + resultCount(), numeric_limits<double>::quiet_NaN());
        cout << "Pthreads CPU\n";
        computePthreads(threadCount, pthreadSeconds);
        showResult(resultRows, resultCols, resultmat, pthreadSeconds);
        bool same = false;
        checkResult(expected.data(), same);
        cout << "Pthreads matches serial: " << (same ? "YES" : "NO") << "\n\n";
        if (!same)
        {
            throw runtime_error("Pthreads result verification failed.");
        }

        fill(resultmat, resultmat + resultCount(), numeric_limits<double>::quiet_NaN());
        cout << "OpenMP CPU\n";
        computeOpenMP(threadCount, openmpSeconds);
        showResult(resultRows, resultCols, resultmat, openmpSeconds);
        checkResult(expected.data(), same);
        cout << "OpenMP matches serial: " << (same ? "YES" : "NO") << "\n\n";
        if (!same)
        {
            throw runtime_error("OpenMP result verification failed.");
        }

        #ifdef CALCULATOR_USE_CUDA
        double cudaSeconds = 0.0;
        fill(resultmat, resultmat + resultCount(), numeric_limits<double>::quiet_NaN());
        cout << "CUDA GPU (includes allocation and data transfers)\n";
        computeCUDA(cudaSeconds);
        showResult(resultRows, resultCols, resultmat, cudaSeconds);
        checkResult(expected.data(), same);
        cout << "CUDA matches serial: " << (same ? "YES" : "NO") << "\n\n";
        if (!same)
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
        freeMatrices();
    }

    size_t resultCount() const
    {
        return static_cast<size_t>(resultRows) * resultCols;
    }

    void checkMatrices()
    {
        bool valid = false;
        checkInput(valid);
        if (!valid || !firstmat || !secondmat || !resultmat)
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

    void requireOperation(int requested)
    {
        if (operation != requested)
        {
            throw invalid_argument("Select matrix multiplication first.");
        }
    }

    // Small expression parser: follows normal arithmetic precedence.
    void skipSpaces()
    {
        while (position < expression.size() &&
               isspace(static_cast<unsigned char>(expression[position])))
        {
            position++;
        }
    }

    bool take(char character)
    {
        skipSpaces();
        if (position < expression.size() && expression[position] == character)
        {
            position++;
            return true;
        }
        return false;
    }

    double parseExpression()
    {
        double value = parseTerm();
        while (true)
        {
            if (take('+'))
            {
                value += parseTerm();
            }
            else if (take('-'))
            {
                value -= parseTerm();
            }
            else
            {
                return value;
            }
        }
    }

    double parseTerm()
    {
        double value = parseUnary();
        while (true)
        {
            if (take('*'))
            {
                value *= parseUnary();
            }
            else if (take('/'))
            {
                double divisor = parseUnary();
                if (divisor == 0.0)
                {
                    throw invalid_argument("Cannot divide by zero.");
                }
                value /= divisor;
            }
            else if (take('%'))
            {
                double divisor = parseUnary();
                if (divisor == 0.0)
                {
                    throw invalid_argument("Cannot take a remainder with zero.");
                }
                value = fmod(value, divisor);
            }
            else
            {
                return value;
            }
        }
    }

    double parseUnary()
    {
        if (take('+'))
        {
            return parseUnary();
        }
        if (take('-'))
        {
            return -parseUnary();
        }
        return parsePower();
    }

    double parsePower()
    {
        double value = parsePrimary();
        if (!isfinite(value))
        {
            throw invalid_argument("Invalid function input or numeric overflow.");
        }
        if (take('^'))
        {
            value = pow(value, parseUnary());
        }
        if (!isfinite(value))
        {
            throw invalid_argument("Invalid function input or numeric overflow.");
        }
        return value;
    }

    double parsePrimary()
    {
        if (take('('))
        {
            double value = parseExpression();
            if (!take(')'))
            {
                throw invalid_argument("Missing closing parenthesis.");
            }
            return value;
        }

        skipSpaces();
        if (position == expression.size())
        {
            throw invalid_argument("Expected a number or function.");
        }

        if (isalpha(static_cast<unsigned char>(expression[position])))
        {
            string name;
            while (position < expression.size() &&
                   isalpha(static_cast<unsigned char>(expression[position])))
            {
                name += expression[position++];
            }
            if (name == "pi")
            {
                return acos(-1.0);
            }
            if (name == "e")
            {
                return exp(1.0);
            }
            if (!take('('))
            {
                throw invalid_argument("Functions need parentheses, for example sqrt(9).");
            }
            double value = parseExpression();
            if (!take(')'))
            {
                throw invalid_argument("Missing closing parenthesis.");
            }
            if (name == "sqrt")
            {
                return sqrt(value);
            }
            if (name == "sin")
            {
                return sin(value);
            }
            if (name == "cos")
            {
                return cos(value);
            }
            if (name == "tan")
            {
                return tan(value);
            }
            if (name == "ln")
            {
                return log(value);
            }
            if (name == "log")
            {
                return log10(value);
            }
            if (name == "exp")
            {
                return exp(value);
            }
            if (name == "abs")
            {
                return fabs(value);
            }
            throw invalid_argument("Unknown function name.");
        }

        const char* start = expression.c_str() + position;
        char* end = nullptr;
        double value = strtod(start, &end);
        if (end == start || !isfinite(value))
        {
            throw invalid_argument("Expected a finite number.");
        }
        position += static_cast<size_t>(end - start);
        return value;
    }

    // Menu values: 2 = multiply, 3 = add, 4 = dot product.
    int operation = 2;
    int r1 = 0;
    int c1 = 0;
    int r2 = 0;
    int c2 = 0;
    int resultRows = 0;
    int resultCols = 0;
    int threadCount = 16;

    double* firstmat = nullptr;
    double* secondmat = nullptr;
    double* resultmat = nullptr;

    string expression;
    size_t position = 0;
    ArithmeticPool* arithmeticPool = nullptr;
    vector<exception_ptr> openmpArithmeticErrors;
};
