#include "termviz.hpp"
#include <thread>
#include <chrono>

using namespace termviz::ThreeD;
using namespace termviz::Visualizer;

int main() {
    termviz::clear_screen();
    termviz::hide_cursor();
    termviz::Window viewWin(5, 2, 80, 35, "3D Spinning Cube");

    termviz::ThreeD::Point3D v[8] = {
        {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
        {-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
    };

    float angle = 0;
    while (true) {
        viewWin.clean_buffer(); // Clear the "ink" from last frame
        angle += 2.0f;          // Increase rotation

        // 1. Rotate, 2. Project, 3. Draw
        termviz::ThreeD::Point3D proj_v[8];
        for(int i=0; i<8; i++) {
            auto rotated = v[i].rotate(angle);
            
            // Perspective Projection
            float f = 30.0f; 
            float factor = f / (rotated.z + 5.0f);
            proj_v[i] = termviz::ThreeD::Point3D(
                (rotated.x * factor * 2.2f) + (viewWin.get_w() / 2.0f),
                (rotated.y * factor) + (viewWin.get_h() / 2.0f),
                rotated.z
            );
        }

        auto draw = [&](int a, int b) {
            ThreeD::draw_line3D(viewWin, proj_v[a], proj_v[b], termviz::COLOR::GREEN, 'o');
        };

        for(int i=0; i<4; i++) {
            draw(i, (i+1)%4); draw(i+4, ((i+1)%4)+4); draw(i, i+4);
        }

        viewWin.render(); // Only prints what changed!
        std::this_thread::sleep_for(30_FPS);
    }
}