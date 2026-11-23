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

    ///
    if (currentScreen == 1)
    {
        drawArithmeticControls();
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
    ///
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