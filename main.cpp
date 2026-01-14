#include "termviz.hpp"
#include <thread>
#include <chrono>

// A simple general-purpose data source
struct TaskManager {
    float progress = 0.0f;
    bool complete = false;
    void update() { 
        progress += 0.01f; 
        if (progress > 1.0f) complete = true; 
    }

};

int main() {
    using namespace termviz;
    
    TaskManager task;
    clear_screen();
    
    // Create a generic status window
    Window statusWin(5, 5, 30, 3, "Task Progress");
    Window logWin(5, 10, 50, 10, "Logs");

    while (!task.complete) {
        // Update the user data
        task.update();

        logWin.print_msgln("Current Progress: " + std::to_string(static_cast<int>(task.progress * 100)) + "%", COLOR(COLOR::YELLOW));
        logWin.render();        

        // DRAW: The library just asks "What is the float right now?"
        // No void* casting, no storage in the library core.
        Visualizer::Plots::draw_progress_bar(statusWin, 0, 0, statusWin.get_w(), [&]() {
            return int(task.progress * 100);
        });

        statusWin.render();
        
        // Check if we should exit (user input logic would go here)
        std::this_thread::sleep_for(15_FPS);
        
        // Optional: clear window buffer for next frame if your 
        // draw_progress_bar doesn't overwrite everything.
        // statusWin.clear(); 
    }

    reset_cursor();
    return 0;
}