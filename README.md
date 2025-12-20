# TermViz

**TermViz** is a lightweight terminal visualization library in C++ for building text-based interfaces, charts, bars, and animations. It provides a `Window` class for creating bounded terminal areas and a `Visualizer` namespace for complex drawing operations. Supports both naive and buffered printing modes.

## Features

* Create multiple independent windows in the terminal, ideally **one window per use case** (e.g., one for logs, one for graphs, one for headings).
* Primitive and high-level drawing functions (`print`, `draw_rectangle`, `draw_bars`, etc.).
* Naive and buffered printing modes for performance control.
* Simple cross-platform terminal control (clear, cursor movement, hide/show).

## Installation

Clone the repository and include the header in your project:

```bash
git clone https://github.com/why-sobi/termviz.git
```

```cpp
#include "termviz.hpp"
```

## Basic Usage

```cpp
#include "termviz.hpp"

int main() {
    using namespace termviz;

    // Create separate windows for different purposes
    Window logWin(0, 0, 50, 10, "Logs");
    Window graphWin(0, 11, 50, 10, "Graph");

    // Naive printing for logs
    logWin.set_buffer_mode(false); 
    logWin.print_msgln("Starting application...");

    // Buffered printing for graph
    graphWin.set_buffer_mode(true);
    Visualizer::draw_bars(graphWin, {3, 5, 2, 4}, 2);
    graphWin.render(); // flush buffered content

    // Primitive rectangle drawing
    Visualizer::draw_rectangle(graphWin, 1, 1, 10, 3, COLOR::GREEN);

    return 0;
}
```

### Usage Notes

* **Primitive functions (`draw_rectangle`, `print`)**: Write at specific positions or draw simple shapes. Typically used in buffered mode to control when the content is flushed.
* **High-level functions (`draw_bars`, `wrap_around`)**: Designed for larger, more complex visualizations. They internally use the primitive functions and handle rendering automatically.
* **Naive mode**: Directly writes output to the terminal; easier to debug but slower for frequent updates.
* **Buffered mode**: Updates an internal buffer first and renders all changes at once; preferred for animations or frequent updates.
* **One window per use case**: Helps maintain visual clarity and avoids overwriting unrelated content. For example, dedicate one window to logs, another to charts, and another for headings or static graphics.


**NOTE**: Further updates like graphs, charts may be worked on later. 