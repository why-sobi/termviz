#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include "echo.hpp" // Make sure this matches your header filename

struct Edge {
    int u, v;
};

int main() {
    // 1. Initialize terminal screen and hide cursor
    echo::clear_screen();
    echo::hide_cursor();

    // Get initial terminal dimensions
    auto [term_w, term_h] = echo::get_terminal_size();
    
    // Create a central window occupying most of the terminal
    int win_w = (std::max)(40, term_w - 4);
    int win_h = (std::max)(20, term_h - 4);
    int win_x = (term_w - win_w) / 2 + 1;
    int win_y = (term_h - win_h) / 2 + 1;

    echo::Window win(win_x, win_y, win_w, win_h, " ECHO 3D WIREFRAME CUBE ");

    // 2. Define a 3D Cube (Vertices)
    std::vector<echo::ThreeD::Point3D> vertices = {
        {-5.0f, -5.0f, -5.0f}, { 5.0f, -5.0f, -5.0f},
        { 5.0f,  5.0f, -5.0f}, {-5.0f,  5.0f, -5.0f},
        {-5.0f, -5.0f,  5.0f}, { 5.0f, -5.0f,  5.0f},
        { 5.0f,  5.0f,  5.0f}, {-5.0f,  5.0f,  5.0f}
    };

    // Define Cube Edges connecting the vertices
    std::vector<Edge> edges = {
        {0,1}, {1,2}, {2,3}, {3,0}, // Back face
        {4,5}, {5,6}, {6,7}, {7,4}, // Front face
        {0,4}, {1,5}, {2,6}, {3,7}  // Connecting edges
    };

    float angle = 0.0f;
    bool running = true;

    // Optional: handle exit safely or loop for 300 frames (~10 seconds)
    int frame_count = 0;
    
    while (running && frame_count < 500) {
        // --- 3. Dynamic Resize Check ---
        auto [current_w, current_h] = echo::get_terminal_size();
        if (current_w != term_w || current_h != term_h) {
            term_w = current_w;
            term_h = current_h;
            win_w = (std::max)(40, term_w - 4);
            win_h = (std::max)(20, term_h - 4);
            win_x = (term_w - win_w) / 2 + 1;
            win_y = (term_h - win_h) / 2 + 1;
            
            // Resize the window buffer safely
            win.resize(win_w, win_h, " ECHO 3D WIREFRAME CUBE ");
        }

        win.clean_buffer();

        // Center point offset inside the window
        float cx = win.get_w() / 2.0f;
        float cy = win.get_h() / 2.0f;
        float distance = 25.0f; // Camera distance / depth offset

        // Rotate and project vertices
        std::vector<echo::ThreeD::Point3D> projected_vertices;
        projected_vertices.reserve(vertices.size());

        for (const auto& v : vertices) {
            // Apply 3D rotation (pitch/yaw combined via your rotate method)
            auto rotated = v.rotate(angle);
            
            // Push back along z-axis to prevent division by zero
            float z_pos = rotated.z + distance;
            
            // Perspective projection: x' = x/z, y' = y/z
            // Factor 0.5 accounts for terminal character aspect ratio (tall characters)
            float fov_scale = 35.0f;
            float proj_x = (rotated.x / z_pos) * fov_scale * 2.0f + cx;
            float proj_y = (rotated.y / z_pos) * fov_scale * 0.9f + cy; 

            projected_vertices.emplace_back(proj_x, proj_y, z_pos);
        }

        // --- 4. Render Edges using UTF-8 Shading Characters ---
        // We'll cycle through dense UTF-8 blocks (█, ▓, ▒, ░) for depth styling
        std::string utf_block = "█"; 
        if (frame_count % 100 < 25) utf_block = "█";
        else if (frame_count % 100 < 50) utf_block = "▓";
        else if (frame_count % 100 < 75) utf_block = "▒";
        else utf_block = "░";

        for (const auto& edge : edges) {
            const auto& p1 = projected_vertices[edge.u];
            const auto& p2 = projected_vertices[edge.v];

            // Dynamic color shift based on angle
            uint8_t r = static_cast<uint8_t>(128 + 127 * sin(angle * 0.05f));
            uint8_t g = static_cast<uint8_t>(128 + 127 * cos(angle * 0.03f));
            uint8_t b = 255;

            echo::Visualizer::ThreeD::draw_line3D(
                win, p1, p2, 
                echo::COLOR(r, g, b), 
                utf_block
            );
        }

        // Print status footer inside window
        win.print(win.get_h() - 1, 2, " Frame: " + std::to_string(frame_count) + " | Resize window to test! ", echo::COLOR(echo::COLOR::YELLOW));

        // Render buffer to screen
        win.render();

        // Increment rotation angle & frame counter
        angle += 3.0f;
        frame_count++;

        // Cap frame rate at 30 FPS
        std::this_thread::sleep_for(30_FPS);
    }

    // Clean up cursor and terminal state
    echo::reset_cursor();
    std::cout << "Exited successfully!\n";
    return 0;
}