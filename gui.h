#pragma once

#include "calculator.h"

#include <X11/Xlib.h>

class CalculatorGUI
{
public:
    ~CalculatorGUI();

    void createWindow();
    void createInputBoxes();
    void createCPUButton();
    void createPthreadsButton();
    void createOpenMPButton();
    void createCUDAButton();
    void createClearButton();
    void createResultBox();

    void createPlotCPUButton();
    void createPlotPthreadsButton();
    void createPlotOpenMPButton();
    void createPerformancePlotButton();

    void cpuButtonClicked();
    void pthreadsButtonClicked();
    void openmpButtonClicked();
    void cudaButtonClicked();
    void clearButtonClicked();

    void plotCPUButtonClicked();
    void plotPthreadsButtonClicked();
    void plotOpenMPButtonClicked();
    void performancePlotButtonClicked();

    void drawFunctionPlot(
        const std::vector<double>& xValues,
        const std::vector<double>& yValues);

    void drawPerformancePlot(
        const std::vector<std::string>& methodNames,
        const std::vector<double>& medianSeconds,
        const std::vector<double>& speedups);

    void run();
    void closeWindow();

private:
    calculator calc;
    
    void drawMenu();
    void handleMenuClick(int mouseX, int mouseY);

    void drawOperationScreen();
    void handleOperationClick(int mouseX, int mouseY);

    void drawArithmeticControls();
    void handleArithmeticKey(XKeyEvent& keyEvent);
    void calculateArithmetic();

    void drawMatrixControls();
    void handleMatrixKey(XKeyEvent& keyEvent);
    void calculateMatrix(int executionMethod);

    

    Display* display = nullptr;                // Connection to the X11 display server
    Window window = 0;                         // Identifier for the calculator window
    GC graphicsContext = nullptr;              // X11 drawing context
    Atom deleteWindowMessage = 0;              // Handles the window's close button

    int currentScreen = 0;                     // 0 = main menu, 1–6 = operation screens
    
    std::string arithmeticInput;
    std::string arithmeticResult;

    int activeMatrixField = 0;                   // 1=dimensions, 2=threads, 3=A, 4=B

    std::string matrixDimensionsInput =
        "2 2 2 2";                               // rowsA colsA rowsB colsB

    std::string matrixThreadsInput = "4";

    std::string matrixAInput =
        "1 2 3 4";

    std::string matrixBInput =
        "5 6 7 8";

    std::string matrixStatus;

    std::vector<double> matrixOutput;
    int matrixOutputRows = 0;
    int matrixOutputColumns = 0;

    bool running = false;                      // Controls the GUI event loop
    int windowWidth = 1000;                    // Initial window width
    int windowHeight = 700;                    // Initial window height

    std::string functionTitle = "f(x)";
};