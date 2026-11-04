#include "calculator.h"

using namespace std;
int main()
{
    calculator calc;

    try
    {
        calc.runCLI();
    }
    catch (const exception& error)
    {
        calc.showError(error.what());
        return 1;
    }

    return 0;
}

