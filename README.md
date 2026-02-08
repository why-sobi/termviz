# ECHO

**ECHO** is a lightweight, thread-safe terminal visualization library in C++ designed for building structured text-based UIs and real-time 3D animations. It treats the terminal as a **frame buffer**, utilizing an optimized "dirty-cell" rendering system to minimize flicker and CPU overhead.

---

## Key Concepts

* **Windows as Buffers**: A `Window` represents a bounded region. All drawing updates an internal grid of `Cell` objects (character + RGB color).
* **Explicit Rendering**: Nothing is printed to the screen until `render()` is called.
* **Dirty-Bit Optimization**: Only the characters that have changed since the last frame are sent to the terminal.
* **Batch Rendering**: ANSI escape codes are batched by color to reduce the number of bytes sent over the wire.

---

## Features

* **Thread-Safe**: Internal mutexes allow one thread to update data while another handles the render loop.
* **TrueColor (RGB)**: Full 24-bit color support with fallbacks for standard 8-color modes.
* **3D Wireframe Engine**: Built-in 3D coordinate system, perspective projection, and depth-aware line drawing.
* **Depth Shading**: Graphics automatically dim/fade based on  distance.
* **Primitive & Plotting**: High-precision progress bars, boxes, and bar charts.
* **Minimalist**: Header-only style, zero dependencies beyond the STL.

---

## Installation

```bash
git clone https://github.com/why-sobi/echo.git

```

Include `echo.hpp` in your project. Ensure you are using C++17 or later.

---

## Usage Examples

### 1. Real-Time 3D Animation

```cpp
#include "echo.hpp"

int main() {
    using namespace echo;
    clear_screen();
    Window view(5, 2, 80, 35, "3D Engine");

    // Define 3D points (x, y, z)
    ThreeD::Point3D p1(-1, -1, 5), p2(1, 1, 10);

    while(true) {
        view.clean_buffer();
        
        // draw_line3D automatically handles perspective and depth-shading
        Visualizer::ThreeD::draw_line3D(view, p1, p2, COLOR::GREEN);
        
        view.render();
        std::this_thread::sleep_for(30_FPS);
    }
}

```

### 2. High-Precision Progress Bars

```cpp
Window ui(0, 0, 40, 5, "Task");
// Takes a lambda/function for reactive updates
Visualizer::Plots::draw_progress_bar(ui, 1, 1, 38, [&](){ return current_progress; }, COLOR::CYAN);
ui.render();

```

---

## The 3D Pipeline

ECHO handles 3D in three distinct steps:

1. **Transformation**: Rotate or move your `Point3D` coordinates.
2. **Projection**: Convert `Point3D` to screen-space coordinates while preserving  depth.
3. **Rasterization**: Use `draw_line3D` (Bresenham's) to draw shaded lines onto the window buffer.

---

## API Overview

### `echo::COLOR`

Supports standard constants (`RED`, `GREEN`, etc.) or custom RGB:
`COLOR myCol(205, 135, 0);`

### `echo::Window`

* `print(row, col, msg, color)`: The core primitive.
* `clean_buffer()`: Clears the "ink" from the window without clearing the terminal screen.
* `render(bool clear_first)`: Pushes the buffer to the terminal.

### `echo::Visualizer`

* **Primitive**: `draw_rectangle`, `draw_line_2d`
* **Plots**: `draw_bars`, `draw_progress_bar`
* **ThreeD**: `draw_line3D` (depth-aware), `project` helpers.

---

## Design Philosophy

ECHO is built for developers who want "software-rendering" control. It does not use a complex `WindowManager` or event-bubbling system. It provides the grid, the math, and the optimized output—the logic of how windows interact is entirely up to you.

---
