#pragma once

#include "calculator.h"

class CalculatorGUI
{
public:
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

    void drawFunctionPlot(const std::vector<double>& xValues, const std::vector<double>& yValues);

    void drawPerformancePlot(const std::vector<std::string>& methodNames, const std::vector<double>& medianSeconds, const std::vector<double>& speedups);

    void run();
    void closeWindow();

private:
    calculator calc;
    std::string functionTitle = "f(x)";
};
