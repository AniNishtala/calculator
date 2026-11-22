#include "gui.h"

#include <X11/keysym.h>

#include <cstring>
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
            drawMenu();
        }
        else if (event.type == ButtonPress)
        {
            if (event.xbutton.button == Button1)
            {
                handleMenuClick(
                    event.xbutton.x,
                    event.xbutton.y);
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
                running = false;
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