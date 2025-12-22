#include "termviz.hpp"
#include <thread>
#include <chrono>
#include <vector>

using namespace termviz;

int main()
{
    clear_screen();

    // ----------------- Hybrid window demo -----------------
    Window hybrid_win(2, 1, 92, 15, "Hybrid Demo"); // start in buffered mode

    // Static content first (buffered)
    Visualizer::Plots::wrap_around(hybrid_win,
        "This window demonstrates switching between buffered and naive modes."
        "Buffered mode accumulates content and renders efficiently."
        "Naive mode prints immediately.",
        COLOR::GREEN);

    Visualizer::Primitive::draw_rectangle(hybrid_win, 8, 5, 15, 3, COLOR::BLUE);
    hybrid_win.render();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ----------------- Dynamic bars (buffered) -----------------
    int max_bars = Visualizer::Plots::getMaxBars(hybrid_win, 3);
    std::vector<int> heights(max_bars, 0);
    std::vector<uint8_t> colors(max_bars, COLOR::BLUE);


    for (int t = 0; t < 60; t++)
    {
        for (int i = 0; i < max_bars; i++)
        {
            heights[i] = 1 + (rand() % hybrid_win.get_h());
        }
        Visualizer::Plots::draw_bars(hybrid_win, heights, 3, colors);
        std::this_thread::sleep_for(20_FPS);
    }

    // ----------------- Final pause -----------------
    std::this_thread::sleep_for(std::chrono::seconds(2));

    reset_cursor();
    return 0;
}
