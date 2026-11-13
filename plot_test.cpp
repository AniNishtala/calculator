#include "calculator.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

using namespace std;

namespace
{
    const int maximumPlots = 16;
    const char* palette[maximumPlots] = {"#1f77b4", "#d62728", "#2ca02c", "#9467bd", "#ff7f0e", "#8c564b", "#c51b7d", "#555555", "#8a8900", "#008b9a", "#000080", "#006400", "#b03060", "#663399", "#a05200", "#000000"};

    enum class InputMode
    {
        Idle,
        Coordinate,
        Integration,
        EditFunction,
        WindowRange,
        ThreadCount
    };

    struct Curve
    {
        string function;
        vector<double> x;
        vector<double> y;
        unsigned long color = 0;
        bool visible = true;
        double seconds = 0.0;
    };

    struct View
    {
        long double xMin;
        long double xMax;
        long double yMin;
        long double yMax;
    };

    struct PlotApplication
    {
        calculator calc;
        vector<Curve> curves;
        double xMin = -10.0;
        double xMax = 10.0;
        int points = 1001;
        int method = 1;
        int threads = 4;
        double lastSamplingSeconds = 0.0;
        string lastTimingScope = "all visible curves";
        bool quietSampling = false;
        int selected = 0;
        int width = 1250;
        int height = 780;
        View home;
        View view;
        bool grid = true;
        bool running = true;
        InputMode inputMode = InputMode::Idle;
        string input;
        string status = "Select a y slot and press E to enter a function. Sampling is automatic.";
        bool marker = false;
        double markerX = 0.0;
        double markerY = 0.0;
        bool integral = false;
        double integralLower = 0.0;
        double integralUpper = 0.0;
        Display* display = nullptr;
        Window window = 0;
        GC gc = nullptr;
        unsigned long black = 0;
        unsigned long panel = 0;
        unsigned long gridColor = 0;

        ~PlotApplication()
        {
            if (display)
            {
                if (gc)
                {
                    XFreeGC(display, gc);
                }
                if (window)
                {
                    XDestroyWindow(display, window);
                }
                XCloseDisplay(display);
            }
        }
    };

    string number(long double value, int precision = 8)
    {
        ostringstream text;
        text << defaultfloat << setprecision(precision) << value;
        return text.str();
    }

    int integerInput(const string& text, int minimum, int maximum)
    {
        istringstream input(text);
        int value = 0;
        string extra;
        if (!(input >> value) || (input >> extra) || value < minimum || value > maximum)
        {
            throw invalid_argument("Enter a whole number from " + to_string(minimum) + " to " + to_string(maximum) + ".");
        }
        return value;
    }

    string methodName(int method)
    {
        return method == 1 ? "Serial CPU" : (method == 2 ? "Pthreads" : "OpenMP");
    }

    void printSamplingTimes(const PlotApplication& app)
    {
        cout << "\nPlot sampling: " << methodName(app.method);
        if (app.method != 1)
        {
            cout << " (" << app.threads << " threads requested)";
        }
        cout << fixed << setprecision(9) << '\n';
        for (size_t i = 0; i < app.curves.size(); i++)
        {
            if (app.curves[i].visible && !app.curves[i].function.empty())
            {
                cout << "  y" << i << ": " << app.curves[i].seconds << " seconds\n";
            }
        }
        cout << "Total sampling time: " << app.lastSamplingSeconds << " seconds\n";
        cout << "Includes expression preparation, evaluation, and adaptive refinement; excludes X11 drawing.\n";
    }

    double sampleValue(PlotApplication& app, const string& function, double x)
    {
        double y = numeric_limits<double>::quiet_NaN();
        try
        {
            app.calc.evaluateFunctionAtX(function, x, y);
        }
        catch (const invalid_argument&)
        {
            // The formula was already validated. This x is outside its domain.
        }
        return y;
    }

    void appendSample(Curve& curve, double x, double y)
    {
        curve.x.push_back(x);
        curve.y.push_back(y);
    }

    void refineSegment(PlotApplication& app, Curve& curve, double xa, double ya, double xm, double ym, double xb, double yb, int depth)
    {
        if (!isfinite(ya) || !isfinite(ym) || !isfinite(yb))
        {
            appendSample(curve, xm, isfinite(ym) ? ym : numeric_limits<double>::quiet_NaN());
            appendSample(curve, xb, yb);
            return;
        }
        bool allAbove = ya > app.view.yMax && ym > app.view.yMax && yb > app.view.yMax;
        bool allBelow = ya < app.view.yMin && ym < app.view.yMin && yb < app.view.yMin;
        long double straightMiddle = (static_cast<long double>(ya) + yb) / 2.0L;
        long double errorPixels = fabs(static_cast<long double>(ym) - straightMiddle) * max(1, app.height - 225) / (app.view.yMax - app.view.yMin);
        if (allAbove || allBelow || errorPixels <= 0.5L)
        {
            appendSample(curve, xb, yb);
            return;
        }
        bool crossesWindow = (ya > app.view.yMax && yb < app.view.yMin) || (yb > app.view.yMax && ya < app.view.yMin);
        if (depth >= 12 || curve.x.size() >= 100000 || xm == xa || xm == xb)
        {
            if (crossesWindow)
            {
                // Persistent curvature across a pole must not become a vertical line.
                appendSample(curve, xm, numeric_limits<double>::quiet_NaN());
            }
            else
            {
                appendSample(curve, xm, ym);
            }
            appendSample(curve, xb, yb);
            return;
        }
        double leftMiddle = xa * 0.5 + xm * 0.5;
        double rightMiddle = xm * 0.5 + xb * 0.5;
        double leftValue = sampleValue(app, curve.function, leftMiddle);
        double rightValue = sampleValue(app, curve.function, rightMiddle);
        refineSegment(app, curve, xa, ya, leftMiddle, leftValue, xm, ym, depth + 1);
        refineSegment(app, curve, xm, ym, rightMiddle, rightValue, xb, yb, depth + 1);
    }

    void sampleCurve(PlotApplication& app, Curve& curve)
    {
        curve.x.clear();
        curve.y.clear();
        if (curve.function.empty())
        {
            curve.seconds = 0.0;
            return;
        }
        auto start = chrono::steady_clock::now();
        vector<double> x;
        vector<double> y;
        // Four samples per horizontal pixel, with an odd total for midpoint checks.
        app.points = min(20001, max(2049, max(1, app.width - 390) * 4 + 1));
        app.xMin = static_cast<double>(app.view.xMin);
        app.xMax = static_cast<double>(app.view.xMax);
        if (app.method == 1)
        {
            app.calc.generateFunctionPlotCPU(curve.function, app.xMin, app.xMax, app.points, x, y, curve.seconds);
        }
        else if (app.method == 2)
        {
            app.calc.generateFunctionPlotPthreads(curve.function, app.xMin, app.xMax, app.points, app.threads, x, y, curve.seconds);
        }
        else
        {
            app.calc.generateFunctionPlotOpenMP(curve.function, app.xMin, app.xMax, app.points, app.threads, x, y, curve.seconds);
        }
        curve.x.reserve(x.size() * 2);
        curve.y.reserve(y.size() * 2);
        appendSample(curve, x[0], y[0]);
        for (size_t i = 0; i + 2 < x.size(); i += 2)
        {
            refineSegment(app, curve, x[i], y[i], x[i + 1], y[i + 1], x[i + 2], y[i + 2], 0);
        }
        curve.seconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    void resampleAll(PlotApplication& app)
    {
        auto start = chrono::steady_clock::now();
        for (Curve& curve : app.curves)
        {
            if (curve.visible || curve.function.empty())
            {
                sampleCurve(app, curve);
            }
        }
        app.lastSamplingSeconds = chrono::duration<double>(chrono::steady_clock::now() - start).count();
        app.lastTimingScope = "all visible curves";
        if (!app.quietSampling)
        {
            printSamplingTimes(app);
        }
    }

    void reportCurveUpdate(PlotApplication& app, int index)
    {
        app.lastSamplingSeconds = app.curves[index].seconds;
        app.lastTimingScope = "y" + to_string(index);
        if (!app.quietSampling)
        {
            cout << fixed << setprecision(9) << "Plot update " << app.lastTimingScope << " [" << methodName(app.method) << "]: " << app.lastSamplingSeconds << " seconds (drawing excluded)\n";
        }
    }

    void checkBenchmarkCurves(const vector<Curve>& expected, const vector<Curve>& actual)
    {
        if (expected.size() != actual.size())
        {
            throw runtime_error("Benchmark methods produced different curve counts.");
        }
        for (size_t i = 0; i < expected.size(); i++)
        {
            if (!expected[i].visible || expected[i].function.empty())
            {
                continue;
            }
            if (expected[i].x.size() != actual[i].x.size() || expected[i].y.size() != actual[i].y.size())
            {
                throw runtime_error("Benchmark methods produced different curve sample counts.");
            }
            for (size_t point = 0; point < expected[i].x.size(); point++)
            {
                double x = expected[i].x[point];
                double y = expected[i].y[point];
                double otherX = actual[i].x[point];
                double otherY = actual[i].y[point];
                if (fabs(x - otherX) > 1e-12 * max(1.0, fabs(x)) || isfinite(y) != isfinite(otherY) || (isfinite(y) && fabs(y - otherY) > 1e-9 * max(1.0, fabs(y))))
                {
                    throw runtime_error("Benchmark methods produced different plot values.");
                }
            }
        }
    }

    bool benchmarkCancelled(PlotApplication& app)
    {
        if (!app.display)
        {
            return false;
        }
        XEvent event;
        while (XCheckTypedWindowEvent(app.display, app.window, KeyPress, &event))
        {
            if (XLookupKeysym(&event.xkey, 0) == XK_Escape)
            {
                app.running = false;
                return true;
            }
        }
        return false;
    }

    void benchmarkPlots(PlotApplication& app)
    {
        bool hasWork = false;
        for (const Curve& curve : app.curves)
        {
            hasWork = hasWork || (curve.visible && !curve.function.empty());
        }
        if (!hasWork)
        {
            throw invalid_argument("Show at least one function before benchmarking.");
        }
        int savedMethod = app.method;
        bool savedQuiet = app.quietSampling;
        double savedSeconds = app.lastSamplingSeconds;
        string savedScope = app.lastTimingScope;
        vector<Curve> savedCurves = app.curves;
        auto restore = [&]()
        {
            app.method = savedMethod;
            app.quietSampling = savedQuiet;
            app.lastSamplingSeconds = savedSeconds;
            app.lastTimingScope = savedScope;
            app.curves.swap(savedCurves);
        };
        const int repeats = 5;
        vector<double> samples[3];
        double warmup[3] = {0.0, 0.0, 0.0};
        double medians[3] = {0.0, 0.0, 0.0};
        cout << "\nPlot benchmark: same visible functions, window, resolution, and refinement tolerance.\n";
        cout << "One warm-up per method, then " << repeats << " measured runs per method. Esc cancels between batches.\n" << flush;

        try
        {
            app.quietSampling = true;
            vector<Curve> reference;
            for (int method = 1; method <= 3; method++)
            {
                app.method = method;
                resampleAll(app);
                warmup[method - 1] = app.lastSamplingSeconds;
                if (method == 1)
                {
                    reference = app.curves;
                }
                else
                {
                    checkBenchmarkCurves(reference, app.curves);
                }
                if (benchmarkCancelled(app))
                {
                    restore();
                    app.status = "Benchmark cancelled.";
                    return;
                }
            }
            for (int run = 0; run < repeats; run++)
            {
                for (int turn = 0; turn < 3; turn++)
                {
                    // Rotate order so no method is always first or last.
                    int method = (run + turn) % 3 + 1;
                    app.method = method;
                    resampleAll(app);
                    samples[method - 1].push_back(app.lastSamplingSeconds);
                    checkBenchmarkCurves(reference, app.curves);
                    if (benchmarkCancelled(app))
                    {
                        restore();
                        app.status = "Benchmark cancelled.";
                        return;
                    }
                }
            }
            for (int method = 0; method < 3; method++)
            {
                sort(samples[method].begin(), samples[method].end());
                medians[method] = samples[method][repeats / 2];
            }
        }
        catch (...)
        {
            restore();
            throw;
        }
        restore();
        cout << fixed << setprecision(9);
        cout << "Warm-up batch times (include any needed worker startup):\n";
        for (int method = 0; method < 3; method++)
        {
            cout << "  " << methodName(method + 1) << ": " << warmup[method] << " seconds\n";
        }
        cout << "Median plot sampling times (" << repeats << " runs; workers reused):\n";
        for (int method = 0; method < 3; method++)
        {
            cout << "  " << methodName(method + 1) << ": " << medians[method] << " seconds";
            if (medians[method] > 0.0)
            {
                cout << " | speedup: " << setprecision(3) << medians[0] / medians[method] << "x" << setprecision(9);
            }
            cout << '\n';
        }
        cout << "Results match. Speedup below 1x means slower. X11 drawing and result checking are excluded.\n";
        app.status = "Benchmark medians: CPU " + number(medians[0] * 1000, 5) + " ms; Pthreads " + number(medians[1] * 1000, 5) + " ms; OpenMP " + number(medians[2] * 1000, 5) + " ms. See terminal for speedups.";
    }

    void applyView(PlotApplication& app, const View& view)
    {
        if (!isfinite(view.xMin) || !isfinite(view.xMax) || !isfinite(view.yMin) || !isfinite(view.yMax) || view.xMin >= view.xMax || view.yMin >= view.yMax || !isfinite(static_cast<double>(view.xMin)) || !isfinite(static_cast<double>(view.xMax)) || static_cast<double>(view.xMin) >= static_cast<double>(view.xMax))
        {
            throw invalid_argument("Use finite increasing x and y ranges.");
        }
        app.view = view;
        resampleAll(app);
    }

    string checkFunctionEntry(calculator& calc, const string& entry, int slot)
    {
        if (entry.size() > 1024)
        {
            throw invalid_argument("Keep the function within 1024 characters.");
        }
        string formula = entry;
        size_t equals = formula.find('=');
        if (equals != string::npos)
        {
            string label;
            for (size_t i = 0; i < equals; i++)
            {
                if (!isspace(static_cast<unsigned char>(formula[i])))
                {
                    label += formula[i];
                }
            }
            if (label != "y" + to_string(slot))
            {
                throw invalid_argument("Enter a formula for y" + to_string(slot) + ", for example sin(x).");
            }
            formula = formula.substr(equals + 1);
        }
        size_t first = formula.find_first_not_of(" \t\r\n");
        if (first == string::npos)
        {
            throw invalid_argument("The function after = cannot be empty.");
        }
        size_t last = formula.find_last_not_of(" \t\r\n");
        formula = formula.substr(first, last - first + 1);
        calc.validateFunction(formula);
        return formula;
    }

    bool readInitialFunctions(PlotApplication& app)
    {
        app.curves.resize(maximumPlots);
        cout << "\nEnter one function per line, using x and explicit multiplication (*).\n";
        cout << "Press Enter on a blank line to open the graph. Blank y0 cancels.\n";
        cout << "Example: sin(x), or y0 = sin(x). Trigonometry uses radians.\n";

        for (int slot = 0; slot < maximumPlots; slot++)
        {
            while (true)
            {
                cout << "y" << slot << " = ";
                string entry;
                if (!getline(cin, entry))
                {
                    return false;
                }
                if (entry.find_first_not_of(" \t\r\n") == string::npos)
                {
                    return slot != 0;
                }
                try
                {
                    app.curves[slot].function = checkFunctionEntry(app.calc, entry, slot);
                    break;
                }
                catch (const exception& error)
                {
                    cout << "Invalid y" << slot << ": " << error.what() << " Try again.\n";
                }
            }
        }
        return true;
    }

    void initializePlot(PlotApplication& app)
    {
        // Fixed TI-style y0-y15 slots. No count or resolution prompts.
        app.curves.resize(maximumPlots);
        if (app.curves[0].function.empty())
        {
            app.curves[0].function = "sin(x)";
        }
        app.home.xMin = -10.0L;
        app.home.xMax = 10.0L;
        app.home.yMin = -10.0L;
        app.home.yMax = 10.0L;
        app.view = app.home;
        resampleAll(app);
    }

    int graphLeft()
    {
        return 85;
    }

    int graphTop()
    {
        return 100;
    }

    int graphRight(const PlotApplication& app)
    {
        return app.width - 305;
    }

    int graphBottom(const PlotApplication& app)
    {
        return app.height - 125;
    }

    int pixelX(const PlotApplication& app, long double x)
    {
        long double pixel = graphLeft() + (x - app.view.xMin) / (app.view.xMax - app.view.xMin) * (graphRight(app) - graphLeft());
        return static_cast<int>(max(-32000.0L, min(32000.0L, pixel)));
    }

    int pixelY(const PlotApplication& app, long double y)
    {
        long double pixel = graphBottom(app) - (y - app.view.yMin) / (app.view.yMax - app.view.yMin) * (graphBottom(app) - graphTop());
        return static_cast<int>(max(-32000.0L, min(32000.0L, pixel)));
    }

    long double tickStep(long double span, int target)
    {
        long double raw = span / max(1, target);
        long double scale = pow(10.0L, floor(log10(raw)));
        long double fraction = raw / scale;
        long double step = fraction <= 1.0L ? 1.0L : (fraction <= 2.0L ? 2.0L : (fraction <= 5.0L ? 5.0L : 10.0L));
        return step * scale;
    }

    void textAt(PlotApplication& app, int x, int y, const string& text, int maximum = 160)
    {
        string shown = text;
        if (shown.size() > static_cast<size_t>(maximum))
        {
            shown = shown.substr(0, maximum - 3) + "...";
        }
        XDrawString(app.display, app.window, app.gc, x, y, shown.c_str(), static_cast<int>(shown.size()));
    }

    void selectCurve(PlotApplication& app, int index)
    {
        app.selected = index;
        app.marker = false;
        app.integral = false;
        app.status = "Selected y" + to_string(index) + ": Shift+2 = y at x; Shift+1 = integral and area.";
    }

    void zoomView(PlotApplication& app, long double factor)
    {
        long double centerX = (app.view.xMin + app.view.xMax) / 2.0L;
        long double centerY = (app.view.yMin + app.view.yMax) / 2.0L;
        long double halfX = (app.view.xMax - app.view.xMin) * factor / 2.0L;
        long double halfY = (app.view.yMax - app.view.yMin) * factor / 2.0L;
        if (centerX - halfX == centerX + halfX || centerY - halfY == centerY + halfY)
        {
            return;
        }
        View view;
        view.xMin = centerX - halfX;
        view.xMax = centerX + halfX;
        view.yMin = centerY - halfY;
        view.yMax = centerY + halfY;
        applyView(app, view);
    }

    void queryCoordinate(PlotApplication& app, const string& input)
    {
        if (app.curves[app.selected].function.empty())
        {
            throw invalid_argument("This y slot is empty. Press Ctrl+Q, then E to enter a function.");
        }
        double x = 0.0;
        double y = 0.0;
        app.calc.calculateExpression(input, x);
        app.calc.evaluateFunctionAtX(app.curves[app.selected].function, x, y);
        app.marker = true;
        app.markerX = x;
        app.markerY = y;
        app.status = "y" + to_string(app.selected) + "(" + number(x, 12) + ") = " + number(y, 12);
        cout << app.status << '\n';
    }

    void queryIntegral(PlotApplication& app, const string& input)
    {
        if (app.curves[app.selected].function.empty())
        {
            throw invalid_argument("This y slot is empty. Press Ctrl+Q, then E to enter a function.");
        }
        vector<string> parts;
        app.calc.splitExpressions(input, parts);
        if (parts.size() < 2 || parts.size() > 3)
        {
            throw invalid_argument("Enter lower ; upper ; even intervals (intervals optional).");
        }
        double lower = 0.0;
        double upper = 0.0;
        app.calc.calculateExpression(parts[0], lower);
        app.calc.calculateExpression(parts[1], upper);
        int intervals = 1000;
        if (parts.size() == 3)
        {
            double entered = 0.0;
            app.calc.calculateExpression(parts[2], entered);
            if (entered < 2.0 || entered > 1000000.0 || floor(entered) != entered)
            {
                throw invalid_argument("Use an even interval count from 2 to 1000000.");
            }
            intervals = static_cast<int>(entered);
        }
        double signedIntegral = 0.0;
        double geometricArea = 0.0;
        double seconds = 0.0;
        app.calc.integrateFunction(app.curves[app.selected].function, lower, upper, intervals, app.method, app.threads, signedIntegral, geometricArea, seconds);
        app.integral = true;
        app.integralLower = lower;
        app.integralUpper = upper;
        app.status = "y" + to_string(app.selected) + " [" + number(lower) + ", " + number(upper) + "]: signed integral = " + number(signedIntegral, 10) + "; geometric area = " + number(geometricArea, 10);
        cout << app.status << " (Simpson approximation, " << intervals << " intervals, " << seconds << " seconds)\n";
    }

    void submitInput(PlotApplication& app)
    {
        try
        {
            if (app.inputMode == InputMode::Coordinate)
            {
                queryCoordinate(app, app.input);
            }
            else if (app.inputMode == InputMode::Integration)
            {
                queryIntegral(app, app.input);
            }
            else if (app.inputMode == InputMode::EditFunction)
            {
                Curve replacement;
                replacement.function = app.input;
                if (replacement.function.find_first_not_of(" \t\r\n") == string::npos)
                {
                    replacement.function.clear();
                }
                replacement.color = app.curves[app.selected].color;
                replacement.visible = !replacement.function.empty();
                sampleCurve(app, replacement);
                app.curves[app.selected] = std::move(replacement);
                app.marker = false;
                app.integral = false;
                app.status = "Updated y" + to_string(app.selected) + ".";
                reportCurveUpdate(app, app.selected);
            }
            else if (app.inputMode == InputMode::WindowRange)
            {
                vector<string> parts;
                app.calc.splitExpressions(app.input, parts);
                if (parts.size() != 2 && parts.size() != 4)
                {
                    throw invalid_argument("Enter xmin ; xmax, or xmin ; xmax ; ymin ; ymax.");
                }
                View view = app.view;
                double values[4];
                for (size_t i = 0; i < parts.size(); i++)
                {
                    app.calc.calculateExpression(parts[i], values[i]);
                }
                view.xMin = values[0];
                view.xMax = values[1];
                if (parts.size() == 4)
                {
                    view.yMin = values[2];
                    view.yMax = values[3];
                }
                applyView(app, view);
                app.status = "Window updated; curves resampled automatically.";
            }
            else if (app.inputMode == InputMode::ThreadCount)
            {
                app.threads = integerInput(app.input, 1, 32);
                resampleAll(app);
                app.status = "Thread count set to " + to_string(app.threads) + ".";
            }
            app.inputMode = InputMode::Idle;
            app.input.clear();
        }
        catch (const exception& error)
        {
            app.status = string("Error: ") + error.what();
        }
    }

    void drawCurves(PlotApplication& app)
    {
        XRectangle clip;
        clip.x = static_cast<short>(graphLeft());
        clip.y = static_cast<short>(graphTop());
        clip.width = static_cast<unsigned short>(graphRight(app) - graphLeft());
        clip.height = static_cast<unsigned short>(graphBottom(app) - graphTop());
        XSetClipRectangles(app.display, app.gc, 0, 0, &clip, 1, Unsorted);
        const size_t requestLimit = 10000;

        for (size_t index = 0; index < app.curves.size(); index++)
        {
            const Curve& curve = app.curves[index];
            if (!curve.visible || curve.function.empty())
            {
                continue;
            }
            XSetForeground(app.display, app.gc, curve.color);
            XSetLineAttributes(app.display, app.gc, index == static_cast<size_t>(app.selected) ? 2 : 1, LineSolid, CapButt, JoinRound);
            vector<XPoint> segment;
            segment.reserve(requestLimit);
            for (size_t i = 0; i < curve.x.size(); i++)
            {
                if (!isfinite(curve.y[i]))
                {
                    if (segment.size() > 1)
                    {
                        XDrawLines(app.display, app.window, app.gc, segment.data(), static_cast<int>(segment.size()), CoordModeOrigin);
                    }
                    segment.clear();
                    continue;
                }
                XPoint point;
                point.x = static_cast<short>(pixelX(app, curve.x[i]));
                point.y = static_cast<short>(pixelY(app, curve.y[i]));
                segment.push_back(point);
                if (segment.size() == requestLimit)
                {
                    XDrawLines(app.display, app.window, app.gc, segment.data(), static_cast<int>(segment.size()), CoordModeOrigin);
                    XPoint last = segment.back();
                    segment.clear();
                    segment.push_back(last);
                }
            }
            if (segment.size() > 1)
            {
                XDrawLines(app.display, app.window, app.gc, segment.data(), static_cast<int>(segment.size()), CoordModeOrigin);
            }
        }
        XSetLineAttributes(app.display, app.gc, 1, LineSolid, CapButt, JoinRound);
        XSetForeground(app.display, app.gc, app.curves[app.selected].color);
        if (app.marker && app.curves[app.selected].visible && app.markerX >= app.view.xMin && app.markerX <= app.view.xMax && app.markerY >= app.view.yMin && app.markerY <= app.view.yMax)
        {
            int x = pixelX(app, app.markerX);
            int y = pixelY(app, app.markerY);
            XDrawLine(app.display, app.window, app.gc, x, graphTop(), x, graphBottom(app));
            XDrawLine(app.display, app.window, app.gc, graphLeft(), y, graphRight(app), y);
            XFillArc(app.display, app.window, app.gc, x - 4, y - 4, 8, 8, 0, 360 * 64);
        }
        if (app.integral)
        {
            XSetLineAttributes(app.display, app.gc, 1, LineOnOffDash, CapButt, JoinRound);
            int lower = pixelX(app, app.integralLower);
            int upper = pixelX(app, app.integralUpper);
            XDrawLine(app.display, app.window, app.gc, lower, graphTop(), lower, graphBottom(app));
            XDrawLine(app.display, app.window, app.gc, upper, graphTop(), upper, graphBottom(app));
        }
        XSetLineAttributes(app.display, app.gc, 1, LineSolid, CapButt, JoinRound);
        XSetClipMask(app.display, app.gc, None);
    }

    void drawWindow(PlotApplication& app)
    {
        if (app.width < 650 || app.height < 400)
        {
            return;
        }
        XClearWindow(app.display, app.window);
        XSetForeground(app.display, app.gc, app.black);
        string method = methodName(app.method);
        textAt(app, 25, 25, "Graphing calculator: y0-y15 | " + method + " | " + app.lastTimingScope + ": " + number(app.lastSamplingSeconds * 1000.0, 6) + " ms | Auto sampling");
        textAt(app, 25, 48, "Shift+2: y at x   Shift+1: integral + area   Tab/click legend: select   Space: show/hide   E: edit");
        textAt(app, 25, 70, "B: benchmark   M: method   T: threads   Wheel/+/-: zoom   Arrows: pan   W: window   Home: reset   Esc: close");

        long double xStep = tickStep(app.view.xMax - app.view.xMin, max(2, (graphRight(app) - graphLeft()) / 90));
        long double yStep = tickStep(app.view.yMax - app.view.yMin, max(2, (graphBottom(app) - graphTop()) / 70));
        long double firstX = ceil(app.view.xMin / xStep) * xStep;
        long double firstY = ceil(app.view.yMin / yStep) * yStep;
        for (int i = 0; i < 100; i++)
        {
            long double value = firstX + i * xStep;
            if (value > app.view.xMax)
            {
                break;
            }
            int x = pixelX(app, value);
            if (app.grid)
            {
                XSetForeground(app.display, app.gc, app.gridColor);
                XDrawLine(app.display, app.window, app.gc, x, graphTop(), x, graphBottom(app));
            }
            XSetForeground(app.display, app.gc, app.black);
            textAt(app, x - 15, graphBottom(app) + 22, number(fabs(value) < xStep * 1e-10L ? 0.0L : value, 5));
        }
        for (int i = 0; i < 100; i++)
        {
            long double value = firstY + i * yStep;
            if (value > app.view.yMax)
            {
                break;
            }
            int y = pixelY(app, value);
            if (app.grid)
            {
                XSetForeground(app.display, app.gc, app.gridColor);
                XDrawLine(app.display, app.window, app.gc, graphLeft(), y, graphRight(app), y);
            }
            XSetForeground(app.display, app.gc, app.black);
            textAt(app, 4, y + 4, number(fabs(value) < yStep * 1e-10L ? 0.0L : value, 5));
        }
        XDrawRectangle(app.display, app.window, app.gc, graphLeft(), graphTop(), graphRight(app) - graphLeft(), graphBottom(app) - graphTop());
        if (app.view.xMin <= 0.0L && app.view.xMax >= 0.0L)
        {
            int x = pixelX(app, 0.0L);
            XDrawLine(app.display, app.window, app.gc, x, graphTop(), x, graphBottom(app));
        }
        if (app.view.yMin <= 0.0L && app.view.yMax >= 0.0L)
        {
            int y = pixelY(app, 0.0L);
            XDrawLine(app.display, app.window, app.gc, graphLeft(), y, graphRight(app), y);
        }
        drawCurves(app);
        int legend = app.width - 280;
        XSetForeground(app.display, app.gc, app.black);
        textAt(app, legend, 102, "Curves (click to select)");
        for (size_t i = 0; i < app.curves.size(); i++)
        {
            int top = 112 + static_cast<int>(i) * 24;
            if (i == static_cast<size_t>(app.selected))
            {
                XSetForeground(app.display, app.gc, app.panel);
                XFillRectangle(app.display, app.window, app.gc, legend - 5, top, 270, 23);
            }
            XSetForeground(app.display, app.gc, app.curves[i].color);
            XDrawLine(app.display, app.window, app.gc, legend, top + 12, legend + 22, top + 12);
            bool shown = app.curves[i].visible && !app.curves[i].function.empty();
            string formula = app.curves[i].function.empty() ? "(empty: E to enter)" : app.curves[i].function;
            textAt(app, legend + 30, top + 17, string(shown ? "[x] " : "[ ] ") + "y" + to_string(i) + " = " + formula, 34);
        }

        XSetForeground(app.display, app.gc, app.black);
        textAt(app, 25, app.height - 78, "Selected y" + to_string(app.selected) + "(x) = " + app.curves[app.selected].function);
        textAt(app, 25, app.height - 53, app.status, max(20, (app.width - 50) / 7));
        if (app.inputMode != InputMode::Idle)
        {
            string prompt;
            if (app.inputMode == InputMode::Coordinate)
            {
                prompt = "x = ";
            }
            if (app.inputMode == InputMode::Integration)
            {
                prompt = "lower ; upper ; even intervals [1000]: ";
            }
            if (app.inputMode == InputMode::EditFunction)
            {
                prompt = "new y" + to_string(app.selected) + "(x) = ";
            }
            if (app.inputMode == InputMode::WindowRange)
            {
                prompt = "xmin ; xmax ; ymin ; ymax: ";
            }
            if (app.inputMode == InputMode::ThreadCount)
            {
                prompt = "CPU threads (1-32): ";
            }
            textAt(app, 25, app.height - 28, prompt + app.input + "_  [Enter: apply, Ctrl+Q: cancel input, Ctrl+U: clear]", max(20, (app.width - 50) / 7));
        }
        else
        {
            textAt(app, 25, app.height - 28, "Integration uses Simpson's rule; undefined samples are gaps. Trigonometry uses radians.");
        }
        XFlush(app.display);
    }

    void keyPressed(PlotApplication& app, XKeyEvent& event)
    {
        KeySym base = XLookupKeysym(&event, 0);
        if (base == XK_Escape)
        {
            app.running = false;
            return;
        }
        if (app.inputMode != InputMode::Idle)
        {
            if ((event.state & ControlMask) && (base == XK_q || base == XK_Q))
            {
                app.inputMode = InputMode::Idle;
                app.input.clear();
                app.status = "Input cancelled.";
            }
            else if (base == XK_Return || base == XK_KP_Enter)
            {
                submitInput(app);
            }
            else if (base == XK_BackSpace || base == XK_Delete)
            {
                if (!app.input.empty())
                {
                    app.input.pop_back();
                }
            }
            else if ((event.state & ControlMask) && (base == XK_u || base == XK_U))
            {
                app.input.clear();
            }
            else
            {
                char characters[64];
                KeySym translated = NoSymbol;
                int length = XLookupString(&event, characters, sizeof(characters), &translated, nullptr);
                for (int i = 0; i < length && app.input.size() < 1024; i++)
                {
                    if (characters[i] >= 32 && characters[i] <= 126)
                    {
                        app.input += characters[i];
                    }
                }
            }
            return;
        }
        if ((event.state & ShiftMask) && base == XK_2)
        {
            app.inputMode = InputMode::Coordinate;
            app.input.clear();
            app.marker = false;
            app.status = "Enter x for selected y" + to_string(app.selected) + ". Expressions such as pi/2 are accepted.";
        }
        else if ((event.state & ShiftMask) && base == XK_1)
        {
            app.inputMode = InputMode::Integration;
            app.input.clear();
            app.integral = false;
            app.status = "Both signed integral and geometric area will be calculated. Example: -10 ; 10 ; 2000";
        }
        else if (base == XK_Tab || base == XK_ISO_Left_Tab)
        {
            int direction = event.state & ShiftMask ? -1 : 1;
            selectCurve(app, (app.selected + direction + static_cast<int>(app.curves.size())) % static_cast<int>(app.curves.size()));
        }
        else if (base == XK_space)
        {
            if (app.curves[app.selected].function.empty())
            {
                app.inputMode = InputMode::EditFunction;
                app.input.clear();
            }
            else
            {
                app.curves[app.selected].visible = !app.curves[app.selected].visible;
                if (app.curves[app.selected].visible)
                {
                    sampleCurve(app, app.curves[app.selected]);
                    reportCurveUpdate(app, app.selected);
                }
            }
        }
        else if (base == XK_e || base == XK_E)
        {
            app.inputMode = InputMode::EditFunction;
            app.input = app.curves[app.selected].function;
        }
        else if (base == XK_Home)
        {
            applyView(app, app.home);
        }
        else if (base == XK_w || base == XK_W)
        {
            app.inputMode = InputMode::WindowRange;
            app.input.clear();
            app.status = "Window range: xmin ; xmax ; ymin ; ymax. Expressions such as pi are accepted.";
        }
        else if (base == XK_t || base == XK_T)
        {
            app.inputMode = InputMode::ThreadCount;
            app.input.clear();
        }
        else if (base == XK_m || base == XK_M)
        {
            app.method = app.method % 3 + 1;
            resampleAll(app);
            app.status = "Plot method changed. Sampling resolution stays automatic.";
        }
        else if (base == XK_b || base == XK_B)
        {
            app.status = "Benchmarking visible curves. Esc cancels between batches.";
            if (app.display)
            {
                drawWindow(app);
            }
            benchmarkPlots(app);
        }
        else if (base == XK_Left || base == XK_Right || base == XK_Up || base == XK_Down)
        {
            View view = app.view;
            long double xShift = (view.xMax - view.xMin) * 0.2L;
            long double yShift = (view.yMax - view.yMin) * 0.2L;
            if (base == XK_Left || base == XK_Right)
            {
                xShift *= base == XK_Left ? -1.0L : 1.0L;
                view.xMin += xShift;
                view.xMax += xShift;
            }
            else
            {
                yShift *= base == XK_Down ? -1.0L : 1.0L;
                view.yMin += yShift;
                view.yMax += yShift;
            }
            applyView(app, view);
        }
        else if (base == XK_g || base == XK_G)
        {
            app.grid = !app.grid;
        }
        else if (base == XK_plus || base == XK_equal || base == XK_KP_Add)
        {
            zoomView(app, 0.8L);
        }
        else if (base == XK_minus || base == XK_KP_Subtract)
        {
            zoomView(app, 1.25L);
        }
    }

    void buttonPressed(PlotApplication& app, const XButtonEvent& event)
    {
        if (app.inputMode != InputMode::Idle)
        {
            return;
        }
        if (event.button == Button4)
        {
            zoomView(app, 0.8L);
        }
        else if (event.button == Button5)
        {
            zoomView(app, 1.25L);
        }
        else if (event.button == Button1 && event.x >= app.width - 285 && event.y >= 112)
        {
            int index = (event.y - 112) / 24;
            if (index >= 0 && index < static_cast<int>(app.curves.size()))
            {
                selectCurve(app, index);
                if (app.curves[index].function.empty())
                {
                    app.inputMode = InputMode::EditFunction;
                    app.input.clear();
                }
            }
        }
        else if (event.button == Button1 && event.x >= graphLeft() && event.x <= graphRight(app) && event.y >= graphTop() && event.y <= graphBottom(app))
        {
            long double fraction = static_cast<long double>(event.x - graphLeft()) / (graphRight(app) - graphLeft());
            double x = static_cast<double>(app.view.xMin + fraction * (app.view.xMax - app.view.xMin));
            try
            {
                queryCoordinate(app, number(x, 17));
            }
            catch (const exception& error)
            {
                app.marker = false;
                app.status = string("Error: ") + error.what();
            }
        }
    }

    void openWindow(PlotApplication& app)
    {
        app.display = XOpenDisplay(nullptr);
        if (!app.display)
        {
            throw runtime_error("Cannot open the X11 display.");
        }
        int screen = XDefaultScreen(app.display);
        app.black = XBlackPixel(app.display, screen);
        app.window = XCreateSimpleWindow(app.display, XRootWindow(app.display, screen), 0, 0, app.width, app.height, 1, app.black, XWhitePixel(app.display, screen));
        app.gc = XCreateGC(app.display, app.window, 0, nullptr);
        if (!app.gc)
        {
            throw runtime_error("Cannot create the drawing context.");
        }
        XStoreName(app.display, app.window, "Calculator plot_test - y0 through y15");
        XSelectInput(app.display, app.window, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
        Atom closeMessage = XInternAtom(app.display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(app.display, app.window, &closeMessage, 1);
        XSizeHints hints;
        memset(&hints, 0, sizeof(hints));
        hints.flags = PMinSize;
        hints.min_width = 950;
        hints.min_height = 600;
        XSetWMNormalHints(app.display, app.window, &hints);
        Colormap colormap = XDefaultColormap(app.display, screen);
        XColor color;
        XColor exact;
        for (size_t i = 0; i < app.curves.size(); i++)
        {
            if (!XAllocNamedColor(app.display, colormap, palette[i], &color, &exact))
            {
                throw runtime_error("Cannot allocate a curve color.");
            }
            app.curves[i].color = color.pixel;
        }
        if (!XAllocNamedColor(app.display, colormap, "#eeeeee", &color, &exact))
        {
            throw runtime_error("Cannot allocate panel color.");
        }
        app.panel = color.pixel;
        if (!XAllocNamedColor(app.display, colormap, "#dddddd", &color, &exact))
        {
            throw runtime_error("Cannot allocate grid color.");
        }
        app.gridColor = color.pixel;
        XMapRaised(app.display, app.window);

        while (app.running)
        {
            XEvent event;
            XNextEvent(app.display, &event);
            if (event.type == Expose && event.xexpose.count == 0)
            {
                drawWindow(app);
            }
            else if (event.type == ConfigureNotify)
            {
                XEvent newest = event;
                XEvent queued;
                while (XCheckTypedWindowEvent(app.display, app.window, ConfigureNotify, &queued))
                {
                    newest = queued;
                }
                int width = min(newest.xconfigure.width, 30000);
                int height = min(newest.xconfigure.height, 30000);
                if ((width != app.width || height != app.height) && width >= 650 && height >= 400)
                {
                    app.width = width;
                    app.height = height;
                    resampleAll(app);
                }
                drawWindow(app);
            }
            else if (event.type == KeyPress)
            {
                try
                {
                    keyPressed(app, event.xkey);
                }
                catch (const exception& error)
                {
                    app.status = string("Error: ") + error.what();
                }
                if (app.running)
                {
                    drawWindow(app);
                }
            }
            else if (event.type == ButtonPress)
            {
                try
                {
                    buttonPressed(app, event.xbutton);
                }
                catch (const exception& error)
                {
                    app.status = string("Error: ") + error.what();
                }
                drawWindow(app);
            }
            else if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == closeMessage)
            {
                app.running = false;
            }
            else if (event.type == DestroyNotify)
            {
                app.window = 0;
                app.running = false;
            }
        }
    }
}

// Called by option 6 in the calculator; Escape returns to its menu.
void runPlotTest()
{
    PlotApplication app;
    if (!readInitialFunctions(app))
    {
        return;
    }
    initializePlot(app);
    openWindow(app);
    cout << "Plot closed.\n";
}

// Optional standalone build: make plot.
#ifdef PLOT_TEST_STANDALONE
int main()
{
    try
    {
        runPlotTest();
    }
    catch (const exception& error)
    {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
#endif

