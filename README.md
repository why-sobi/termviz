# TermViz

**TermViz** is a lightweight terminal visualization library in C++ for building structured text-based UIs such as logs, bars, simple charts, and animations. It treats the terminal as a **frame buffer**, where drawing functions update window-local state and rendering is explicitly controlled by the user.

The library is intentionally minimal and focuses on predictable rendering, low flicker, and batching terminal output for performance.

---

## Key Concepts

* A `Window` represents a **bounded region** of the terminal.
* All drawing functions write to an **internal buffer**.
* **Nothing is printed immediately** — changes appear only after calling `render()`.
* Rendering is explicit and deterministic.
* Ideally, use **one window per use case** (logs, visualizer, headings, etc.).

---

## Features

* Multiple independent terminal windows with fixed boundaries.
* Primitive drawing operations (`print`, `print_msgln`, `draw_rectangle`).
* Higher-level visualization helpers (`draw_bars`, wrapping helpers).
* Buffered rendering using batched output (minimal flicker).
* Simple FPS literal (`X_FPS`) for frame timing or sleep control.
* Cross-platform terminal control (cursor movement, clear, hide/show).

---

## Installation

Clone the repository and include the header:

```bash
git clone https://github.com/why-sobi/termviz.git
```

```cpp
#include "termviz.hpp"
```

No build system or external dependencies are required.

---

## Basic Usage

```cpp
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
```

---

## Rendering Model (Important)

* All `print` and drawing functions **only modify the window buffer**.
* `render()` must be called to push changes to the terminal.
* This design avoids partial updates, flicker, and excessive `std::cout` calls.
* You control *when* rendering happens — immediately or on a fixed frame rate.

---

## Drawing API Overview

### Primitive Functions

Examples:

* `print`
* `print_msg`
* `print_msgln`
* `draw_rectangle`

Characteristics:

* Write at explicit positions or draw simple shapes.
* Do **not** trigger rendering automatically.
* Intended to be combined and flushed together via `render()`.

### Higher-Level Helpers

Examples:

* `draw_bars`
* wrapping / layout helpers

Characteristics:

* Built on top of primitive operations.
* Suitable for visualizers and Plotting UI elements.
* Still require an explicit `render()` call after updates.

---

## FPS Utility

TermViz provides a user-defined literal for frame timing:

```cpp
std::this_thread::sleep_for(60_FPS);
```

* Converts FPS → `std::chrono::milliseconds`
* Capped at 60 FPS
* Intended for simple frame pacing in animation loops

---

## Design Notes

* The library assumes **buffered rendering only**.
* Immediate (naive) printing was intentionally removed to avoid leaky abstractions.
* Windows do not manage timing or threads.
* Rendering strategy (manual vs frame-based) is left to the user.

---

## Notes

* The API is still evolving.
* More visualization helpers (charts, graphs) may be added later.
* The focus is correctness, predictability, and performance over feature breadth.
