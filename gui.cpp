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

            const char* title =
                "Operating Systems Calculator";

            XDrawString(
                display,
                window,
                graphicsContext,
                30,
                50,
                title,
                static_cast<int>(std::strlen(title)));
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