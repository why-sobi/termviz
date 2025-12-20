#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <random>
#include <vector>
#include <mutex>

namespace termviz
{
    inline int max_height = INT_MIN;
    inline std::mutex screen_lock;

    inline void hide_cursor()
    {
        std::cout << "\033[?25l";
    }

    inline void show_cursor()
    {
        std::cout << "\033[?25h";
    }

    inline void clear_screen()
    {
        screen_lock.lock();
        hide_cursor();
        std::cout << "\033[2J\033[H" << std::flush;
        screen_lock.unlock();
    }

    inline void reset_cursor()
    {
        screen_lock.lock();
        show_cursor();
        std::cout << "\033[" << max_height << ";1H" << std::flush;
        screen_lock.unlock();
    }

    namespace COLOR
    {
        constexpr uint8_t RED = 0;
        constexpr uint8_t GREEN = 1;
        constexpr uint8_t YELLOW = 2;
        constexpr uint8_t BLUE = 3;
        constexpr uint8_t MAGENTA = 4;
        constexpr uint8_t ORANGE = 5;
        constexpr uint8_t RESET = 6;

        std::string_view ANSI(uint8_t color)
        {
            switch (color)
            {
            case 0:
                return "\033[31m"; // RED
            case 1:
                return "\033[32m"; // GREEN
            case 2:
                return "\033[33m"; // YELLOW
            case 3:
                return "\033[34m"; // BLUE
            case 4:
                return "\033[35m"; // MAGENTA
            case 5:
                return "\033[38;5;208m"; // ORANGE
            case 6:
                return "\033[37m"; // RESET
            default:
                return "\033[37m"; // Default RESET
            }
        }

        const uint8_t random_color()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<uint8_t> dist(RED, ORANGE);
            return dist(gen);
        }
    }

    struct Cell
    { // so that each cell can have its own character and color
        char ch;
        uint8_t color;

        Cell(char ch = ' ', uint8_t color = COLOR::RESET) : ch(ch), color(color) {}

        bool operator==(const Cell &other) const { return ch == other.ch && color == other.color; }
        bool operator!=(const Cell &other) const { return ch != other.ch || color != other.color; }

        friend std::ostream &operator<<(std::ostream &out, const Cell &cell)
        {
            out << COLOR::ANSI(cell.color) << cell.ch;
            return out;
        }
    };

    class Window
    {
    private:
        int x, y;
        int width, height;
        int r, c;
        bool buffered_mode = true; // true = use dirty-bit, false = naive printing

        std::vector<std::vector<bool>> dirty; // to track modified cells for optimized rendering
        std::vector<std::vector<Cell>> content;

        // ----------------- CORE PRIMITIVES -----------------
        void move_string_to_cell(int row_index, const std::string &msg, int start_col, uint8_t color)
        {
            size_t msg_length = msg.length();
            size_t total_columns = content[row_index].size();

            for (size_t i = 0; i < msg_length && (start_col + i) < total_columns; i++)
            {
                content[row_index][start_col + i] = Cell(msg[i], color);
                if (buffered_mode)
                    dirty[row_index][start_col + i] = true;
            }
        }

        void move_cursor(int cx, int cy) const
        {
            std::cout << "\033[" << cy << ";" << cx << "H";
        }

        void clear_line(int px, int py) const
        {
            int cx = x + 1 + px;
            int cy = y + 1 + py;
            move_cursor(cx, cy);
            std::cout << std::string(width - 2, ' ');
            std::cout << std::flush;
        }

        void draw_border(const std::string &heading = "") const
        {
            screen_lock.lock();
            move_cursor(x, y);

            if (heading != "")
            {
                size_t heading_length = heading.length();
                int left = ((width - 2) - heading_length) / 2;
                int right = ((width - 2) - heading_length) - left;
                std::cout << "+" << std::string(left, '-') << heading << std::string(right, '-') << "+";
            }
            else
            {
                std::cout << "+" << std::string(width - 2, '-') << '+';
            }

            for (int i = 1; i < height - 1; i++)
            {
                move_cursor(x, y + i);
                std::cout << "|" << std::string(width - 2, ' ') << "|";
            }

            move_cursor(x, y + height - 1);
            std::cout << "+" << std::string(width - 2, '-') << "+";
            std::cout << std::flush;
            screen_lock.unlock();
        }

        // ----------------- PRIVATE NAIVE/BLOCK FUNCTIONS -----------------
        void print_msg_naive(const std::string &msg, uint8_t color)
        {
            move_string_to_cell(r, msg, 0, color);
            screen_lock.lock();
            move_cursor(x + c, y + r + 1);
            std::cout << COLOR::ANSI(color) << msg << COLOR::ANSI(COLOR::RESET) << std::flush;
            screen_lock.unlock();

            (++r) %= content.size();
            c = 1;
        }

        void print_msg_buffered(const std::string &msg, uint8_t color)
        {
            move_string_to_cell(r, msg, 0, color);
            (++r) %= content.size();
            c = 1;
        }

        void print_msgln_naive(const std::string &msg, uint8_t color)
        {
            int append_chars = (width - 2) - msg.length();
            std::string full_msg = msg + std::string(append_chars > 0 ? append_chars : 0, ' ');
            print_msg_naive(full_msg, color);
        }

        void print_msgln_buffered(const std::string &msg, uint8_t color)
        {
            int append_chars = (width - 2) - msg.length();
            std::string full_msg = msg + std::string(append_chars > 0 ? append_chars : 0, ' ');
            print_msg_buffered(full_msg, color);
        }

        void print_naive(int row, int col, const std::string &msg, uint8_t color)
        {
            move_string_to_cell(row, msg, col, color);
            screen_lock.lock();
            move_cursor(x + 1 + col, y + 1 + row);
            std::cout << COLOR::ANSI(color) << msg << COLOR::ANSI(COLOR::RESET) << std::flush;
            screen_lock.unlock();
        }

        void print_buffered(int row, int col, const std::string &msg, uint8_t color)
        {
            move_string_to_cell(row, msg, col, color);
        }

    public:
        Window(int x, int y, int w, int h, std::string title = "", bool buffered = true)
            : x(x), y(y), width(w), height(h), r(0), c(1), buffered_mode(buffered)
        {
            std::cout << COLOR::ANSI(COLOR::RESET);
            max_height = (std::max)(max_height, y + h);
            draw_border(title);

            content.resize(h - 2);
            dirty.resize(h - 2);
            for (int i = 0; i < h - 2; i++)
            {
                content[i].resize(w - 2, Cell(' ', COLOR::RESET));
                dirty[i].resize(w - 2, false);
            }
        }

        void clear_inside()
        {
            screen_lock.lock();
            for (int i = 1; i < height - 1; i++)
            {
                move_cursor(x + 1, y + i);
                std::string row_content(width - 2, ' ');
                move_string_to_cell(i - 1, row_content, 0, COLOR::RESET);
                if (!buffered_mode)
                    std::cout << row_content;
            }
            if (!buffered_mode)
                std::cout << std::flush;
            screen_lock.unlock();
        }

        // ----------------- PUBLIC PRINT FUNCTIONS -----------------
        void print_msg(const std::string &msg, uint8_t color = COLOR::RESET)
        {
            if (buffered_mode)
                print_msg_buffered(msg, color);
            else
                print_msg_naive(msg, color);
        }

        void print_msgln(const std::string &msg, uint8_t color = COLOR::RESET)
        {
            if (buffered_mode)
                print_msgln_buffered(msg, color);
            else
                print_msgln_naive(msg, color);
        }

        void print_line(char ch = '-', uint8_t color = COLOR::RESET)
        {
            std::string line(width - 2, ch);
            print_msg(line, color);
        }

        void print(int row, int col, const std::string &msg, uint8_t color = COLOR::RESET)
        {
            if (buffered_mode)
                print_buffered(row, col, msg, color);
            else
                print_naive(row, col, msg, color);
        }

        void render()
        {
            if (!buffered_mode)
                return; // naive mode prints immediately

            screen_lock.lock();
            int total_rows = content.size();
            int total_cols = content[0].size();

            for (size_t i = 0; i < total_rows; i++)
            {
                size_t j = 0;
                while (j < total_cols)
                {
                    while (j < total_cols && !dirty[i][j])
                        j++;
                    if (j == total_cols)
                        break;
                    move_cursor(x + 1 + j, y + 1 + i);
                    while (j < total_cols && dirty[i][j])
                    {
                        std::cout << content[i][j];
                        dirty[i][j] = false;
                        j++;
                    }
                }
            }

            std::cout << std::flush;
            screen_lock.unlock();
        }

        int get_h() const { return height - 2; }
        int get_w() const { return width - 2; }
        int get_x() const { return x; }
        int get_y() const { return y; }
        int get_rows() const { return content.size(); }
        int get_cols() const { return content[0].size(); }

        void set_buffer_mode(bool buffered) { buffered_mode = buffered; }
    };

    namespace Visualizer
    {
        namespace Static
        {
            // functions meant for static display, like album covers, titles, etc.
            void wrap_around(Window &win, const std::string &msg, uint8_t color = COLOR::RESET)
            {
                win.clear_inside();
                int total_rows = win.get_rows();
                int total_cols = win.get_cols();

                size_t start = 0;
                size_t msg_length = msg.length();

                for (int r = 0; r < total_rows && start < msg_length; r++)
                {
                    std::string line;
                    size_t remaining = msg_length - start;

                    if (remaining <= total_cols)
                        line = msg.substr(start, remaining);
                    else
                        line = msg.substr(start, total_cols);

                    win.print(r, 0, line, color);
                    start += line.length();
                }
                win.render();
            }

            void draw_rectangle(Window &win, int row, int col, int width, int height, uint8_t color = COLOR::RESET, char ch = char(219))
            {
                if (col < 0 || col + width > win.get_w() || row < 0 || row + height > win.get_h())
                    throw std::out_of_range("\nERROR: Rectangle dimensions exceed window bounds in draw_rectangle");

                for (int r = row; r < row + height; r++)
                    win.print(r, col, std::string(width, ch), color);
            }
        }

        namespace Dynamic
        {
            // functions meant for dynamic updates, like bars, graphs, animations
            int getMaxBars(Window &win, int bar_width) { return (win.get_w()) / bar_width; }

            void draw_bars(Window &win, const std::vector<int> &heights, int bar_width, const std::vector<uint8_t> &colors = {}, char ch = char(219))
            {
                win.clear_inside();

                if (heights.empty())
                    throw std::invalid_argument("\nERROR: Heights vector is empty in draw_bars");
                if (bar_width <= 0)
                    throw std::invalid_argument("\nnERROR: Bar width must be positive in draw_bars");
                if (heights.size() * bar_width > win.get_w())
                    throw std::out_of_range("\nnERROR: Bars exceed window width in draw_bars");
                if (!colors.empty() && colors.size() != heights.size())
                    throw std::invalid_argument("\nnERROR: Colors vector size must match heights vector size in draw_bars");

                int total_rows = win.get_rows();
                int cols = heights.size();

                for (int i = 0; i < cols; i++)
                {
                    int bar_height = heights[i];
                    uint8_t color = colors.empty() ? COLOR::RESET : colors[i];

                    for (int r = total_rows - bar_height; r < total_rows; r++)
                        win.print(r, i * bar_width, std::string(bar_width, ch), color);
                }
                win.render();
            }
        }
    }
}
