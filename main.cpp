#include "termviz.hpp"
#include <thread>

int main() {
    using namespace termviz;

    clear_screen();

    // One window per use case
    Window logWin(0, 0, 50, 8, "Logs");
    Window visWin(0, 9, 50, 10, "Visualizer");

    // Write log messages
    logWin.print_msgln("Starting application...");
    logWin.print_msgln("Loading resources...");
    logWin.render(); // explicit flush

    // Draw a rectangle (primitive)
    Visualizer::Primitive::draw_rectangle(visWin, 1, 1, 20, 4, COLOR::GREEN);

    // Draw bars (higher-level helper)
    Visualizer::Plots::draw_bars(visWin, {3, 5, 2, 6, 4}, 2);

    /*
        The rectangle will not be drawn as it is immediately overwritten by the bars.
        if you want rectangle to appear you must call visWin.render() right after drawing the rectangle.
    */

    // Example frame-based update loop
    while (true) {
        Visualizer::Plots::draw_bars(visWin, {4, 2, 6, 3, 5}, 2);
        visWin.render();
        std::this_thread::sleep_for(30_FPS);
    }

    reset_cursor();

    return 0;
}