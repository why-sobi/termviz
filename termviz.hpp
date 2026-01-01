#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <string_view>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <climits>

using namespace std::chrono;

std::chrono::milliseconds operator ""_FPS(unsigned long long fps) {
        if (fps == 0)   throw std::invalid_argument("\nERROR: FPS must be a positive integer (0, 60]");
        if (fps > 60)   throw std::invalid_argument("\nERROR: FPS are capped at 60 FPS");
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(1.0 / fps));
}

namespace termviz
{
    inline int max_height = INT_MIN;
    inline std::mutex screen_lock;

    struct COLOR {
        static constexpr uint8_t RED = 0;
        static constexpr uint8_t GREEN = 1;
        static constexpr uint8_t YELLOW = 2;
        static constexpr uint8_t BLUE = 3;
        static constexpr uint8_t MAGENTA = 4;
        static constexpr uint8_t ORANGE = 5;
        static constexpr uint8_t RESET = 6;
    private:
        void idToRGB(uint8_t colorID) {
            switch (colorID) {
                case RED: { r = 205; g = b = 0; break; }
                case GREEN: { g = 205; r = b = 0; break; }
                case YELLOW: { r = g = 205; b = 0; break; }
                case BLUE: { b = 205; r = g = 0; break; }
                case MAGENTA: { r = b = 205; g = 0; break; }
                case ORANGE: { r = 205; g = 135; b = 0; break; }
                case RESET: { r = g = b = 229; break; }
                default: throw std::invalid_argument("COLOR ID IS INVALID!\n");
            }
        }
    public:
        uint8_t r, g, b;

        COLOR(uint8_t colorID = COLOR::RESET) { idToRGB(colorID); }
        COLOR(uint8_t red, uint8_t green, uint8_t blue): r(red), g(green), b(blue) {}

        std::string asANSI() const {
            return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        }

        static std::string_view asANSI(uint8_t colorID) {
            switch (colorID) {
            case RED: return "\033[31m"; // RED
            case GREEN: return "\033[32m"; // GREEN
            case YELLOW: return "\033[33m"; // YELLOW
            case BLUE: return "\033[34m"; // BLUE
            case MAGENTA: return "\033[35m"; // MAGENTA
            case ORANGE: return "\033[38;5;208m"; // ORANGE
            case RESET: return "\033[37m"; // RESET
            default: throw std::invalid_argument("COLOR ID IS INVALID!\n");
            }
        }

        static COLOR random_color() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<uint8_t> dist(0, 255);

            return COLOR(dist(gen), dist(gen), dist(gen));
        }

        bool operator == (const COLOR& other) const { return r == other.r && g == other.g && b == other.b; }
        bool operator != (const COLOR& other) const { return r != other.r || g != other.g || b != other.b; }
    };

    void hide_cursor()
    {
        std::cout << "\033[?25l";
    }

    void show_cursor()
    {
        std::cout << "\033[?25h";
    }

    void reset_cursor()
    {
        std::lock_guard<std::mutex> lock(screen_lock);
        show_cursor();
        std::cout << "\033[" << max_height << ";1H" << COLOR::asANSI(COLOR::RESET) << std::flush;
    }
    
    inline void clear_screen()
    {
        std::lock_guard<std::mutex> lock(screen_lock);
        hide_cursor();
        std::cout << "\033[2J\033[H" << std::flush;        
    }

    struct Cell
    { // so that each cell can have its own character and color
        char ch;
        COLOR color;

        Cell(char ch = ' ', const COLOR& color = COLOR(COLOR::RESET)) : ch(ch), color(color) {}

        bool operator==(const Cell &other) const { return ch == other.ch && color == other.color; }
        bool operator!=(const Cell &other) const { return ch != other.ch || color != other.color; }

        friend std::ostream &operator<<(std::ostream &out, const Cell &cell)
        {
            out << cell.color.asANSI() << cell.ch;
            return out;
        }
    };

    class Window
    {
    private:
        int x, y;
        int width, height;
        int r, c;

        std::vector<std::vector<bool>> dirty; // to track modified cells for optimized rendering
        std::vector<std::vector<Cell>> content;


        // ----------------- CORE PRIMITIVES -----------------
        void move_string_to_cell(int row_index, const std::string &msg, int start_col, const COLOR& color)
        {
            size_t msg_length = msg.length();
            size_t total_columns = content[row_index].size();

            for (size_t i = 0; i < msg_length && (start_col + i) < total_columns; i++)
            {
                content[row_index][start_col + i] = Cell(msg[i], color);
                dirty[row_index][start_col + i] = true;
            }
        }

        std::string_view trim_string(const std::string &msg, size_t max_length) {
            if (msg.length() <= max_length)
                return msg;
            else
                return std::string_view(msg.c_str(), max_length);
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
            std::lock_guard<std::mutex> lock(screen_lock);
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
            
        }

        public:
        Window(int x, int y, int w, int h, std::string title = "")
            : x(x), y(y), width(w), height(h), r(0), c(1) {
            std::cout << COLOR::asANSI(COLOR::RESET);
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

        ~Window() {
            std::cout << COLOR::asANSI(COLOR::RESET);
        }
        void clear_inside()
        {
            std::lock_guard<std::mutex> lock(screen_lock);
            std::string row_content(width - 2, ' ');

            for (int i = 1; i < height - 1; i++)
            {
                move_cursor(x + 1, y + i);
                std::cout << row_content;
            }
            std::cout << std::flush;            
        }

        void clean_buffer() {
            std::string row_content(width - 2, ' ');

            for (int i = 1; i < height - 1; i++)
            {
                move_string_to_cell(i - 1, row_content, 0, COLOR::RESET);
            }
        }

        // ----------------- PUBLIC PRINT FUNCTIONS -----------------
        void print_msg(const std::string_view &msg, const COLOR& color = COLOR(COLOR::RESET))
        {
            if (msg.length() > static_cast<size_t>(width - 2))
                throw std::out_of_range("\nERROR: Message length exceeds window width in print_msg");
            move_string_to_cell(r, msg.data(), 0, color);
            (++r) %= content.size();
            c = 1;
        }

        void print_msgln(const std::string &msg, const COLOR& color = COLOR(COLOR::RESET))
        {
            if (msg.length() > static_cast<size_t>(width - 2))
            {
                print_msg(trim_string(msg, width - 2), color);
            }
            else
            {
                int append_chars = (width - 2) - msg.length();
                std::string full_msg = msg + std::string(append_chars > 0 ? append_chars : 0, ' ');
                print_msg(full_msg, color);
            }
        }

        void print_line(char ch = '-', const COLOR& color = COLOR(COLOR::RESET))
        {
            std::string line(width - 2, ch);
            print_msg(line, color);
        }

        void print(int row, int col, const std::string &msg, const COLOR& color = COLOR(COLOR::RESET))
        {
            move_string_to_cell(row, msg, col, color);
        }

        void render(bool clear_first=false)
        {
            if (clear_first) clear_inside(); // use it with visalizer

            std::lock_guard<std::mutex> lock(screen_lock);

            size_t total_rows = content.size();
            size_t total_cols = content[0].size();

    
            std::stringstream ss;
            COLOR curr_color(COLOR::RESET); // temp setting

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
                    
                    curr_color = content[i][j].color;
                    ss << curr_color.asANSI() << content[i][j].ch;
                    dirty[i][j] = false;
                    j++;

                    while (j < total_cols) { 
                        if (curr_color == content[i][j].color) {
                            ss << content[i][j].ch;
                            dirty[i][j] = false;
                        } else {
                            ss << content[i][j].color.asANSI() << content[i][j].ch;
                            curr_color = content[i][j].color;
                            dirty[i][j] = false;
                        }
                        j++;
                    }
                    // Output the accumulated string with color changes
                    std::cout << ss.str();
                    ss.str("");
                    ss.clear();
                }
            }

            std::cout << std::flush;
            
        }

        int get_h() const { return height - 2; }
        int get_w() const { return width - 2; }
        int get_x() const { return x; }
        int get_y() const { return y; }
        int get_rows() const { return content.size(); }
        int get_cols() const { return content[0].size(); }
    };

    namespace Visualizer
    {
        namespace Primitive
        {
            void draw_rectangle(Window &win, int row, int col, int width, int height, const COLOR& color = COLOR(COLOR::RESET), char ch = '#')
            {
                if (col < 0 || col + width > win.get_w() || row < 0 || row + height > win.get_h())
                    throw std::out_of_range("\nERROR: Rectangle dimensions exceed window bounds in draw_rectangle");

                for (int r = row; r < row + height; r++)
                    win.print(r, col, std::string(width, ch), color);
            }
        }
        namespace Plots
        {
            // functions meant for static display, like album covers, titles, etc.
            void wrap_around(Window &win, const std::string &msg, const COLOR& color = COLOR(COLOR::RESET))
            {
                win.clean_buffer();

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
            }

            int getMaxBars(Window &win, int bar_width) { return (win.get_w()) / bar_width; }

            void draw_bars(Window &win, const std::vector<int> &heights, int bar_width, const std::vector<COLOR> &colors = {}, char ch = '#')
            {
                if (heights.empty())
                    throw std::invalid_argument("\nERROR: Heights vector is empty in draw_bars");
                if (bar_width <= 0)
                    throw std::invalid_argument("\nnERROR: Bar width must be positive in draw_bars");
                if (heights.size() * bar_width > win.get_w())
                    throw std::out_of_range("\nnERROR: Bars exceed window width in draw_bars");
                if (!colors.empty() && colors.size() != heights.size())
                    throw std::invalid_argument("\nnERROR: Colors vector size must match heights vector size in draw_bars");

                win.clean_buffer();

                int total_rows = win.get_rows();
                int cols = heights.size();

                for (int i = 0; i < cols; i++)
                {
                    int bar_height = heights[i];
                    const COLOR& color = colors.empty() ? COLOR(COLOR::BLUE) : colors[i];

                    for (int r = total_rows - bar_height; r < total_rows; r++)
                        win.print(r, i * bar_width, std::string(bar_width, ch), color);
                }
            }
            
            void draw_frame(Window &win, const std::vector<char> &chars, const std::vector<COLOR>& colors = {}) {
                if (chars.size() != colors.size()) throw std::invalid_argument("Size of characters do not match size of colors vector!\n");
                if (chars.size() != win.get_w() * win.get_h()) throw std::out_of_range("Total characters do not match window size!\n");

                win.clean_buffer();

                int total_chars = chars.size();
                int max_width = win.get_w();
                for (int i = 0; i < total_chars; i++) {
                    int x = i % max_width;
                    int y = i / max_width;

                    win.print(y, x, std::string(1, chars[i]), colors[i]);
                }
            }
        }
        
    }
}
