#include "gui.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>


CalculatorGUI::~CalculatorGUI()
{
    closeWindow();
}


void CalculatorGUI::createWindow()
{
    display = XOpenDisplay(nullptr);                       // Connects to the Linux display server

    if (display == nullptr)
    {
        throw std::runtime_error(
            "Could not connect to the X11 display server.");
    }

    int screen = DefaultScreen(display);

    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100,                                               // Starting x position
        100,                                               // Starting y position
        windowWidth,
        windowHeight,
        1,                                                 // Border width
        BlackPixel(display, screen),
        WhitePixel(display, screen));

    if (window == 0)
    {
        closeWindow();
        throw std::runtime_error(
            "Could not create the calculator window.");
    }

    XStoreName(
        display,
        window,
        "Operating Systems Calculator");

    XSelectInput(
        display,
        window,
        ExposureMask |
        KeyPressMask |
        ButtonPressMask |
        StructureNotifyMask);

    deleteWindowMessage =
        XInternAtom(display, "WM_DELETE_WINDOW", False);

    XSetWMProtocols(
        display,
        window,
        &deleteWindowMessage,
        1);

    graphicsContext =
        XCreateGC(display, window, 0, nullptr);

    if (graphicsContext == nullptr)
    {
        closeWindow();
        throw std::runtime_error(
            "Could not create the X11 drawing context.");
    }

    XSetForeground(
        display,
        graphicsContext,
        BlackPixel(display, screen));

    XMapRaised(display, window);                           // Makes the window visible
    XFlush(display);
}


// Draw Menu
void CalculatorGUI::drawMenu()
{
    const char* title = "Operating Systems Calculator";

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        50,
        title,
        static_cast<int>(std::strlen(title)));

    const char* buttonLabels[] =
    {
        "1. Arithmetic Expression",
        "2. Matrix Multiplication",
        "3. Matrix Addition",
        "4. Matrix Dot Product",
        "5. Batch Arithmetic",
        "6. Function Plotter",
        "0. Exit"
    };

    const int buttonCount = 7;
    const int buttonX = 80;
    const int firstButtonY = 100;
    const int buttonWidth = 500;
    const int buttonHeight = 55;
    const int buttonSpacing = 15;

    for (int i = 0; i < buttonCount; i++)
    {
        int buttonY =
            firstButtonY +
            i * (buttonHeight + buttonSpacing);

        XDrawRectangle(
            display,
            window,
            graphicsContext,
            buttonX,
            buttonY,
            buttonWidth,
            buttonHeight);

        XDrawString(
            display,
            window,
            graphicsContext,
            buttonX + 20,
            buttonY + 34,
            buttonLabels[i],
            static_cast<int>(
                std::strlen(buttonLabels[i])));
    }
}


void CalculatorGUI::handleMenuClick(
    int mouseX,
    int mouseY)
{
    const int buttonX = 80;
    const int firstButtonY = 100;
    const int buttonWidth = 500;
    const int buttonHeight = 55;
    const int buttonSpacing = 15;
    const int buttonCount = 7;

    if (mouseX < buttonX ||
        mouseX > buttonX + buttonWidth)
    {
        return;
    }

    int relativeY = mouseY - firstButtonY;

    if (relativeY < 0)
    {
        return;
    }

    int buttonAreaHeight =
        buttonHeight + buttonSpacing;

    int buttonIndex =
        relativeY / buttonAreaHeight;

    int positionInsideButton =
        relativeY % buttonAreaHeight;

    if (buttonIndex < 0 ||
        buttonIndex >= buttonCount ||
        positionInsideButton > buttonHeight)
    {
        return;
    }

    if (buttonIndex == 6)
    {
        running = false;
    }
    else
    {
        currentScreen = buttonIndex + 1;

        XClearWindow(display, window);
        drawOperationScreen();
        XFlush(display);
    }
}

void CalculatorGUI::drawArithmeticControls()
{
    const char* inputLabel =
        "Enter an arithmetic expression:";

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        100,
        inputLabel,
        static_cast<int>(std::strlen(inputLabel)));

    const int inputX = 30;
    const int inputY = 120;
    const int inputWidth = 700;
    const int inputHeight = 45;

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        inputX,
        inputY,
        inputWidth,
        inputHeight);

    XDrawString(
        display,
        window,
        graphicsContext,
        inputX + 15,
        inputY + 28,
        arithmeticInput.c_str(),
        static_cast<int>(arithmeticInput.size()));

    const int calculateX = 30;
    const int calculateY = 190;
    const int calculateWidth = 150;
    const int calculateHeight = 45;

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        calculateX,
        calculateY,
        calculateWidth,
        calculateHeight);

    const char* calculateLabel = "Calculate";

    XDrawString(
        display,
        window,
        graphicsContext,
        calculateX + 40,
        calculateY + 28,
        calculateLabel,
        static_cast<int>(std::strlen(calculateLabel)));

    const int clearX = 200;
    const int clearY = 190;
    const int clearWidth = 120;
    const int clearHeight = 45;

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        clearX,
        clearY,
        clearWidth,
        clearHeight);

    const char* clearLabel = "Clear";

    XDrawString(
        display,
        window,
        graphicsContext,
        clearX + 40,
        clearY + 28,
        clearLabel,
        static_cast<int>(std::strlen(clearLabel)));

    if (!arithmeticResult.empty())
    {
        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            285,
            arithmeticResult.c_str(),
            static_cast<int>(arithmeticResult.size()));
    }

    const char* instruction =
        "Type an expression and press Enter or click Calculate.";

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        330,
        instruction,
        static_cast<int>(std::strlen(instruction)));
}


void CalculatorGUI::calculateArithmetic()
{
    if (arithmeticInput.empty())
    {
        arithmeticResult =
            "Error: Enter an arithmetic expression.";
    }
    else
    {
        try
        {
            double answer = 0.0;

            calc.calculateExpression(
                arithmeticInput,
                answer);

            std::ostringstream output;
            output << std::setprecision(12) << answer;

            arithmeticResult =
                "Result: " + output.str();
        }
        catch (const std::exception& error)
        {
            arithmeticResult =
                std::string("Error: ") + error.what();
        }
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}


void CalculatorGUI::handleArithmeticKey(
    XKeyEvent& keyEvent)
{
    char textBuffer[32];
    KeySym keySymbol;

    int characterCount =
        XLookupString(
            &keyEvent,
            textBuffer,
            sizeof(textBuffer),
            &keySymbol,
            nullptr);

    if (keySymbol == XK_BackSpace)
    {
        if (!arithmeticInput.empty())
        {
            arithmeticInput.pop_back();
            arithmeticResult.clear();
        }
    }
    else if (keySymbol == XK_Return ||
             keySymbol == XK_KP_Enter)
    {
        calculateArithmetic();
        return;
    }
    else
    {
        for (int i = 0; i < characterCount; i++)
        {
            unsigned char character =
                static_cast<unsigned char>(
                    textBuffer[i]);

            if (std::isprint(character) &&
                arithmeticInput.size() < 100)
            {
                arithmeticInput.push_back(
                    static_cast<char>(character));

                arithmeticResult.clear();
            }
        }
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}

void CalculatorGUI::drawMatrixControls()
{
    auto drawInputField =
        [&](int fieldNumber,
            const char* label,
            int x,
            int y,
            int width,
            const std::string& value)
        {
            std::string displayedLabel;

            if (activeMatrixField == fieldNumber)
            {
                displayedLabel = "> ";
            }

            displayedLabel += label;

            XDrawString(
                display,
                window,
                graphicsContext,
                x,
                y,
                displayedLabel.c_str(),
                static_cast<int>(
                    displayedLabel.size()));

            XDrawRectangle(
                display,
                window,
                graphicsContext,
                x,
                y + 10,
                width,
                35);

            XDrawString(
                display,
                window,
                graphicsContext,
                x + 10,
                y + 33,
                value.c_str(),
                static_cast<int>(value.size()));
        };

    drawInputField(
        1,
        "Dimensions: rowsA colsA rowsB colsB",
        30,
        90,
        300,
        matrixDimensionsInput);

    drawInputField(
        2,
        "Thread count",
        360,
        90,
        120,
        matrixThreadsInput);

    drawInputField(
        3,
        "Matrix A values (row by row)",
        30,
        160,
        800,
        matrixAInput);

    drawInputField(
        4,
        "Matrix B values (row by row)",
        30,
        230,
        800,
        matrixBInput);

    auto drawButton =
        [&](int x,
            int width,
            const char* label)
        {
            const int buttonY = 310;
            const int buttonHeight = 40;

            XDrawRectangle(
                display,
                window,
                graphicsContext,
                x,
                buttonY,
                width,
                buttonHeight);

            XDrawString(
                display,
                window,
                graphicsContext,
                x + 15,
                buttonY + 25,
                label,
                static_cast<int>(
                    std::strlen(label)));
        };

    drawButton(30, 90, "CPU");
    drawButton(135, 120, "Pthreads");
    drawButton(270, 120, "OpenMP");
    drawButton(405, 90, "CUDA");
    drawButton(510, 90, "Clear");

    if (!matrixStatus.empty())
    {
        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            390,
            matrixStatus.c_str(),
            static_cast<int>(matrixStatus.size()));
    }

    if (!matrixOutput.empty())
    {
        const char* resultLabel = "Result:";

        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            425,
            resultLabel,
            static_cast<int>(
                std::strlen(resultLabel)));

        int rowsToDisplay =
            std::min(matrixOutputRows, 6);

        int columnsToDisplay =
            std::min(matrixOutputColumns, 6);

        for (int row = 0;
             row < rowsToDisplay;
             row++)
        {
            std::ostringstream outputRow;
            outputRow << "[ ";

            for (int column = 0;
                 column < columnsToDisplay;
                 column++)
            {
                outputRow
                    << std::setprecision(8)
                    << matrixOutput[
                        static_cast<size_t>(row) *
                        matrixOutputColumns +
                        column]
                    << " ";
            }

            if (matrixOutputColumns > columnsToDisplay)
            {
                outputRow << "... ";
            }

            outputRow << "]";

            std::string rowText =
                outputRow.str();

            XDrawString(
                display,
                window,
                graphicsContext,
                30,
                450 + row * 22,
                rowText.c_str(),
                static_cast<int>(rowText.size()));
        }

        if (matrixOutputRows > rowsToDisplay)
        {
            const char* moreRows = "...";

            XDrawString(
                display,
                window,
                graphicsContext,
                30,
                450 + rowsToDisplay * 22,
                moreRows,
                3);
        }
    }
}

void CalculatorGUI::handleMatrixKey(
    XKeyEvent& keyEvent)
{
    char textBuffer[32];
    KeySym keySymbol;

    int characterCount =
        XLookupString(
            &keyEvent,
            textBuffer,
            sizeof(textBuffer),
            &keySymbol,
            nullptr);

    if (keySymbol == XK_Tab)
    {
        activeMatrixField =
            activeMatrixField % 4 + 1;
    }
    else
    {
        std::string* activeInput = nullptr;

        if (activeMatrixField == 1)
        {
            activeInput = &matrixDimensionsInput;
        }
        else if (activeMatrixField == 2)
        {
            activeInput = &matrixThreadsInput;
        }
        else if (activeMatrixField == 3)
        {
            activeInput = &matrixAInput;
        }
        else if (activeMatrixField == 4)
        {
            activeInput = &matrixBInput;
        }

        if (activeInput != nullptr)
        {
            if (keySymbol == XK_BackSpace)
            {
                if (!activeInput->empty())
                {
                    activeInput->pop_back();
                }
            }
            else
            {
                for (int i = 0;
                     i < characterCount;
                     i++)
                {
                    unsigned char character =
                        static_cast<unsigned char>(
                            textBuffer[i]);

                    if (std::isprint(character) &&
                        activeInput->size() < 2000)
                    {
                        activeInput->push_back(
                            static_cast<char>(
                                character));
                    }
                }
            }

            matrixStatus.clear();
            matrixOutput.clear();
        }
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}


void CalculatorGUI::calculateMatrix(
    int executionMethod)
{
    try
    {
        int rowsA;
        int columnsA;
        int rowsB;
        int columnsB;
        int threads;

        std::istringstream dimensions(
            matrixDimensionsInput);

        std::string extraText;

        if (!(dimensions >>
              rowsA >>
              columnsA >>
              rowsB >>
              columnsB) ||
            (dimensions >> extraText))
        {
            throw std::invalid_argument(
                "Enter four whole-number dimensions.");
        }

        std::istringstream threadInput(
            matrixThreadsInput);

        if (!(threadInput >> threads) ||
            (threadInput >> extraText))
        {
            throw std::invalid_argument(
                "Enter one whole-number thread count.");
        }

        auto parseValues =
            [](const std::string& text)
            {
                std::vector<double> values;
                std::istringstream input(text);
                double value;

                while (input >> value)
                {
                    values.push_back(value);
                }

                if (!input.eof())
                {
                    throw std::invalid_argument(
                        "Matrix values must be numbers separated by spaces.");
                }

                return values;
            };

        std::vector<double> firstValues =
            parseValues(matrixAInput);

        std::vector<double> secondValues =
            parseValues(matrixBInput);

        calc.configureMatrixData(
            currentScreen,
            rowsA,
            columnsA,
            rowsB,
            columnsB,
            threads,
            firstValues,
            secondValues);

        double seconds = 0.0;
        std::string methodName;

        if (executionMethod == 1)
        {
            methodName = "CPU";
            calc.computeSerial(seconds);
        }
        else if (executionMethod == 2)
        {
            methodName = "Pthreads";
            calc.computePthreads(
                threads,
                seconds);
        }
        else if (executionMethod == 3)
        {
            methodName = "OpenMP";
            calc.computeOpenMP(
                threads,
                seconds);
        }
        else if (executionMethod == 4)
        {
            methodName = "CUDA";
            calc.computeCUDA(seconds);
        }
        else
        {
            throw std::invalid_argument(
                "Unknown execution method.");
        }

        matrixOutput =
            calc.getMatrixResult();

        matrixOutputRows =
            calc.getMatrixResultRows();

        matrixOutputColumns =
            calc.getMatrixResultColumns();

        std::ostringstream status;
        status
            << methodName
            << " time: "
            << std::fixed
            << std::setprecision(9)
            << seconds
            << " seconds";

        matrixStatus = status.str();
    }
    catch (const std::exception& error)
    {
        matrixOutput.clear();
        matrixOutputRows = 0;
        matrixOutputColumns = 0;

        matrixStatus =
            std::string("Error: ") +
            error.what();
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}

void CalculatorGUI::drawBatchControls()
{
    std::string expressionLabel;

    if (activeBatchField == 1)
    {
        expressionLabel = "> ";
    }

    expressionLabel +=
        "Expressions separated by semicolons";

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        90,
        expressionLabel.c_str(),
        static_cast<int>(
            expressionLabel.size()));

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        30,
        100,
        800,
        45);

    XDrawString(
        display,
        window,
        graphicsContext,
        40,
        128,
        batchInput.c_str(),
        static_cast<int>(batchInput.size()));

    std::string threadLabel;

    if (activeBatchField == 2)
    {
        threadLabel = "> ";
    }

    threadLabel += "Thread count";

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        175,
        threadLabel.c_str(),
        static_cast<int>(
            threadLabel.size()));

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        30,
        185,
        120,
        40);

    XDrawString(
        display,
        window,
        graphicsContext,
        40,
        211,
        batchThreadsInput.c_str(),
        static_cast<int>(
            batchThreadsInput.size()));

    auto drawButton =
        [&](int x,
            int width,
            const char* label)
        {
            const int buttonY = 250;
            const int buttonHeight = 40;

            XDrawRectangle(
                display,
                window,
                graphicsContext,
                x,
                buttonY,
                width,
                buttonHeight);

            XDrawString(
                display,
                window,
                graphicsContext,
                x + 15,
                buttonY + 25,
                label,
                static_cast<int>(
                    std::strlen(label)));
        };

    drawButton(30, 90, "CPU");
    drawButton(135, 120, "Pthreads");
    drawButton(270, 120, "OpenMP");
    drawButton(405, 90, "Clear");

    if (!batchStatus.empty())
    {
        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            330,
            batchStatus.c_str(),
            static_cast<int>(
                batchStatus.size()));
    }

    int resultsToDisplay =
        std::min(
            static_cast<int>(
                batchResults.size()),
            8);

    for (int i = 0;
         i < resultsToDisplay;
         i++)
    {
        std::ostringstream resultLine;

        resultLine
            << (i + 1)
            << ". "
            << batchExpressions[i]
            << " = "
            << std::setprecision(12)
            << batchResults[i];

        std::string resultText =
            resultLine.str();

        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            370 + i * 25,
            resultText.c_str(),
            static_cast<int>(
                resultText.size()));
    }

    if (batchResults.size() > 8)
    {
        const char* additionalResults =
            "... additional results not displayed";

        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            370 + resultsToDisplay * 25,
            additionalResults,
            static_cast<int>(
                std::strlen(additionalResults)));
    }
}


void CalculatorGUI::handleBatchKey(
    XKeyEvent& keyEvent)
{
    char textBuffer[32];
    KeySym keySymbol;

    int characterCount =
        XLookupString(
            &keyEvent,
            textBuffer,
            sizeof(textBuffer),
            &keySymbol,
            nullptr);

    if (keySymbol == XK_Tab)
    {
        activeBatchField =
            activeBatchField % 2 + 1;
    }
    else
    {
        std::string* activeInput = nullptr;

        if (activeBatchField == 1)
        {
            activeInput = &batchInput;
        }
        else if (activeBatchField == 2)
        {
            activeInput = &batchThreadsInput;
        }

        if (activeInput != nullptr)
        {
            if (keySymbol == XK_BackSpace)
            {
                if (!activeInput->empty())
                {
                    activeInput->pop_back();
                }
            }
            else
            {
                for (int i = 0;
                     i < characterCount;
                     i++)
                {
                    unsigned char character =
                        static_cast<unsigned char>(
                            textBuffer[i]);

                    if (std::isprint(character) &&
                        activeInput->size() < 2000)
                    {
                        activeInput->push_back(
                            static_cast<char>(
                                character));
                    }
                }
            }

            batchStatus.clear();
            batchResults.clear();
            batchExpressions.clear();
        }
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}


void CalculatorGUI::calculateBatch(
    int executionMethod)
{
    try
    {
        int threads;
        std::string extraText;

        std::istringstream threadInput(
            batchThreadsInput);

        if (!(threadInput >> threads) ||
            (threadInput >> extraText))
        {
            throw std::invalid_argument(
                "Enter one whole-number thread count.");
        }

        calc.splitExpressions(
            batchInput,
            batchExpressions);

        double seconds = 0.0;
        std::string methodName;

        if (executionMethod == 1)
        {
            methodName = "CPU";

            calc.arithmeticSerial(
                batchExpressions,
                batchResults,
                seconds);
        }
        else if (executionMethod == 2)
        {
            methodName = "Pthreads";

            calc.arithmeticPthreads(
                batchExpressions,
                threads,
                batchResults,
                seconds);
        }
        else if (executionMethod == 3)
        {
            methodName = "OpenMP";

            calc.arithmeticOpenMP(
                batchExpressions,
                threads,
                batchResults,
                seconds);
        }
        else
        {
            throw std::invalid_argument(
                "Unknown batch execution method.");
        }

        std::ostringstream status;
        status
            << methodName
            << " time: "
            << std::fixed
            << std::setprecision(9)
            << seconds
            << " seconds";

        batchStatus = status.str();
    }
    catch (const std::exception& error)
    {
        batchResults.clear();
        batchExpressions.clear();

        batchStatus =
            std::string("Error: ") +
            error.what();
    }

    XClearWindow(display, window);
    drawOperationScreen();
    XFlush(display);
}



void CalculatorGUI::drawOperationScreen()
{
    const char* screenTitles[] =
    {
        "",
        "Arithmetic Expression",
        "Matrix Multiplication",
        "Matrix Addition",
        "Matrix Dot Product",
        "Batch Arithmetic",
        "Function Plotter"
    };

    if (currentScreen < 1 ||
        currentScreen > 6)
    {
        return;
    }

    const char* title =
        screenTitles[currentScreen];

    XDrawString(
        display,
        window,
        graphicsContext,
        30,
        50,
        title,
        static_cast<int>(std::strlen(title)));

    if (currentScreen == 1)
    {
        drawArithmeticControls();
    }
    
    else if (currentScreen >= 2 &&
         currentScreen <= 4)
    {
        drawMatrixControls();
    }
    else if (currentScreen == 5)
    {
        drawBatchControls();
    }
    else

    {
        const char* message =
            "Input controls will be added next.";

        XDrawString(
            display,
            window,
            graphicsContext,
            30,
            100,
            message,
            static_cast<int>(std::strlen(message)));
    }
    const int backButtonX = 30;
    const int backButtonY = 620;
    const int backButtonWidth = 180;
    const int backButtonHeight = 45;

    XDrawRectangle(
        display,
        window,
        graphicsContext,
        backButtonX,
        backButtonY,
        backButtonWidth,
        backButtonHeight);

    const char* backLabel =
        "Back to Main Menu";

    XDrawString(
        display,
        window,
        graphicsContext,
        backButtonX + 20,
        backButtonY + 28,
        backLabel,
        static_cast<int>(std::strlen(backLabel)));
}


void CalculatorGUI::handleOperationClick(
    int mouseX,
    int mouseY)
{

    
    if (currentScreen == 1)
    {
        bool insideCalculateButton =
            mouseX >= 30 &&
            mouseX <= 180 &&
            mouseY >= 190 &&
            mouseY <= 235;

        bool insideClearButton =
            mouseX >= 200 &&
            mouseX <= 320 &&
            mouseY >= 190 &&
            mouseY <= 235;

        if (insideCalculateButton)
        {
            calculateArithmetic();
            return;
        }

        if (insideClearButton)
        {
            arithmeticInput.clear();
            arithmeticResult.clear();

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }
    }
    
    if (currentScreen >= 2 &&
        currentScreen <= 4)
    {
        if (mouseY >= 100 &&
            mouseY <= 135)
        {
            if (mouseX >= 30 &&
                mouseX <= 330)
            {
                activeMatrixField = 1;
            }
            else if (mouseX >= 360 &&
                     mouseX <= 480)
            {
                activeMatrixField = 2;
            }
            else
            {
                return;
            }

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }

        if (mouseX >= 30 &&
            mouseX <= 830 &&
            mouseY >= 170 &&
            mouseY <= 205)
        {
            activeMatrixField = 3;

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }

        if (mouseX >= 30 &&
            mouseX <= 830 &&
            mouseY >= 240 &&
            mouseY <= 275)
        {
            activeMatrixField = 4;

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }

        if (mouseY >= 310 &&
            mouseY <= 350)
        {
            if (mouseX >= 30 &&
                mouseX <= 120)
            {
                calculateMatrix(1);
                return;
            }

            if (mouseX >= 135 &&
                mouseX <= 255)
            {
                calculateMatrix(2);
                return;
            }

            if (mouseX >= 270 &&
                mouseX <= 390)
            {
                calculateMatrix(3);
                return;
            }

            if (mouseX >= 405 &&
                mouseX <= 495)
            {
                calculateMatrix(4);
                return;
            }

            if (mouseX >= 510 &&
                mouseX <= 600)
            {
                matrixDimensionsInput.clear();
                matrixThreadsInput.clear();
                matrixAInput.clear();
                matrixBInput.clear();

                matrixStatus.clear();
                matrixOutput.clear();
                matrixOutputRows = 0;
                matrixOutputColumns = 0;

                activeMatrixField = 1;

                XClearWindow(display, window);
                drawOperationScreen();
                XFlush(display);
                return;
            }
        }
    }

    ///
    if (currentScreen == 5)
    {
        if (mouseX >= 30 &&
            mouseX <= 830 &&
            mouseY >= 100 &&
            mouseY <= 145)
        {
            activeBatchField = 1;

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }

        if (mouseX >= 30 &&
            mouseX <= 150 &&
            mouseY >= 185 &&
            mouseY <= 225)
        {
            activeBatchField = 2;

            XClearWindow(display, window);
            drawOperationScreen();
            XFlush(display);
            return;
        }

        if (mouseY >= 250 &&
            mouseY <= 290)
        {
            if (mouseX >= 30 &&
                mouseX <= 120)
            {
                calculateBatch(1);
                return;
            }

            if (mouseX >= 135 &&
                mouseX <= 255)
            {
                calculateBatch(2);
                return;
            }

            if (mouseX >= 270 &&
                mouseX <= 390)
            {
                calculateBatch(3);
                return;
            }

            if (mouseX >= 405 &&
                mouseX <= 495)
            {
                batchInput.clear();
                batchThreadsInput.clear();
                batchStatus.clear();
                batchResults.clear();
                batchExpressions.clear();

                activeBatchField = 1;

                XClearWindow(display, window);
                drawOperationScreen();
                XFlush(display);
                return;
            }
        }
    }
    ///
    

    const int backButtonX = 30;
    const int backButtonY = 620;
    const int backButtonWidth = 180;
    const int backButtonHeight = 45;

    bool insideBackButton =
        mouseX >= backButtonX &&
        mouseX <= backButtonX + backButtonWidth &&
        mouseY >= backButtonY &&
        mouseY <= backButtonY + backButtonHeight;

    if (insideBackButton)
    {
        currentScreen = 0;

        XClearWindow(display, window);
        drawMenu();
        XFlush(display);
    }
}

void CalculatorGUI::run()
{
    createWindow();
    running = true;

    while (running)
    {
        XEvent event;
        XNextEvent(display, &event);                       // Waits for the next GUI event

        if (event.type == Expose &&
            event.xexpose.count == 0)
        {
            XClearWindow(display, window);

            if (currentScreen == 0)
            {
                drawMenu();
            }
            else
            {
                drawOperationScreen();
            }
        }
        else if (event.type == ButtonPress)
        {
            if (event.xbutton.button == Button1)
            {
                if (currentScreen == 0)
                {
                    handleMenuClick(
                        event.xbutton.x,
                        event.xbutton.y);
                }
                else
                {
                    handleOperationClick(
                        event.xbutton.x,
                        event.xbutton.y);
                }
            }
        }
        else if (event.type == ConfigureNotify)
        {
            windowWidth = event.xconfigure.width;
            windowHeight = event.xconfigure.height;
        }
        else if (event.type == KeyPress)
        {
            KeySym key =
                XLookupKeysym(&event.xkey, 0);

            if (key == XK_Escape)
            {
                if (currentScreen == 0)
                {
                    running = false;
                }
                else
                {
                    currentScreen = 0;

                    XClearWindow(display, window);
                    drawMenu();
                    XFlush(display);
                }
            }
            else if (currentScreen == 1)
            {
                handleArithmeticKey(event.xkey);
            }
            else if (currentScreen >= 2 &&
                    currentScreen <= 4)
            {
                handleMatrixKey(event.xkey);
            }
            else if (currentScreen == 5)
            {
                handleBatchKey(event.xkey);
            }

        }
        else if (
            event.type == ClientMessage &&
            static_cast<Atom>(
                event.xclient.data.l[0]) ==
                deleteWindowMessage)
        {
            running = false;
        }
        else if (event.type == DestroyNotify)
        {
            window = 0;
            running = false;
        }
    }

    closeWindow();
}


void CalculatorGUI::closeWindow()
{
    if (display == nullptr)
    {
        return;
    }

    if (graphicsContext != nullptr)
    {
        XFreeGC(display, graphicsContext);
        graphicsContext = nullptr;
    }

    if (window != 0)
    {
        XDestroyWindow(display, window);
        window = 0;
    }

    XCloseDisplay(display);
    display = nullptr;
    running = false;
}