#include "calculator.h"
#include "gui.h"

using namespace std;

void runPlotTest();

int main(int argc, char* argv[])
{
    try
    {
        if (argc > 1 &&
            string(argv[1]) == "--gui")
        {
            CalculatorGUI gui;
            gui.run();
        }
        else
        {
            calculator calc;
            calc.runCLI(
                calculator::PlotDisplay(),
                runPlotTest);
        }
    }
    catch (const exception& error)
    {
        cerr << "Error: "
             << error.what()
             << '\n';

        return 1;
    }

    return 0;
}