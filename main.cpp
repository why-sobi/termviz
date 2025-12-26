#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

#include "termviz.hpp"
using namespace termviz;

int main()
{
    clear_screen();

    Window hybrid_win(2, 1, 92, 15, "Hybrid Demo"); 
    int max_bars = Visualizer::Plots::getMaxBars(hybrid_win, 3);
    std::vector<int> heights(max_bars, 0);
    std::vector<uint8_t> colors(max_bars);

    std::generate(colors.begin(), colors.end(), []() {return COLOR::random_color(); });

    for (int t = 0; t < 60; t++)
    {
        for (int i = 0; i < max_bars; i++)
        {
            heights[i] = 1 + (rand() % hybrid_win.get_h());

        }
        Visualizer::Plots::draw_bars(hybrid_win, heights, 3, colors);
        hybrid_win.render();
        std::this_thread::sleep_for(60_FPS);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    reset_cursor();
    return 0;
}