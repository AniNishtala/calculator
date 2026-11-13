#include "calculator.h"

using namespace std;

void runPlotTest();

int main()
{
    calculator calc;

    try
    {
        calc.runCLI(calculator::PlotDisplay(), runPlotTest);
    }
    catch (const exception& error)
    {
        calc.showError(error.what());
        return 1;
    }

    return 0;
}
