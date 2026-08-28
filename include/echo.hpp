#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <string_view>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <climits>
#include <cassert>
#include <cwctype>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <signal.h>
#endif

using namespace std::chrono;

inline std::chrono::milliseconds operator ""_FPS(unsigned long long fps) {
        if (fps <= 0)   throw std::invalid_argument("\nERROR: FPS must be a positive integer (0, 60]");
        if (fps > 60)   throw std::invalid_argument("\nERROR: FPS are capped at 60 FPS");

        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(1.0 / fps));
}

namespace echo
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
        static constexpr uint8_t CYAN = 6;
        static constexpr uint8_t RESET = 100;
    private:
        void idToRGB(uint8_t colorID) {
            switch (colorID) {
                case RED:       { r = 205; g = b = 0; break; }
                case GREEN:     { g = 205; r = b = 0; break; }
                case YELLOW:    { r = g = 205; b = 0; break; }
                case BLUE:      { b = 205; r = g = 0; break; }
                case MAGENTA:   { r = b = 205; g = 0; break; }
                case ORANGE:    { r = 205; g = 135; b = 0; break; }
                case CYAN:      { r = 0; g = b = 255; break; }
                case RESET:     { r = g = b = 229; break; }
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
            case RED: return "\033[31m";
            case GREEN: return "\033[32m";
            case YELLOW: return "\033[33m";
            case BLUE: return "\033[34m";
            case MAGENTA: return "\033[35m";
            case ORANGE: return "\033[38;5;208m";
            case RESET: return "\033[37m";
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

    // --------------- GLOBAL HELPERS --------------
    inline void init_terminal() {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }

    inline void hide_cursor() { std::cout << "\033[?25l"; }
    inline void show_cursor(){ std::cout << "\033[?25h"; }

    inline void reset_cursor() {
        std::lock_guard<std::mutex> lock(screen_lock);
        show_cursor();
        std::cout << "\033[" << max_height << ";1H" << COLOR::asANSI(COLOR::RESET) << std::flush;
    }

    inline void clear_screen() {
        std::lock_guard<std::mutex> lock(screen_lock);
        hide_cursor();
        std::cout << "\033[2J\033[H" << std::flush;        
    }

    inline std::pair<int, int> get_terminal_size() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
#else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return { w.ws_col, w.ws_row };
#endif
    }


    struct Cell
    { 
        std::string utf_char; // Full UTF-8 support (handles 1-4 byte characters)
        COLOR color;

        Cell(const std::string &ch = " ", const COLOR& color = COLOR(COLOR::RESET)) : utf_char(ch), color(color) {}
        Cell(char ch, const COLOR& color = COLOR(COLOR::RESET)) : utf_char(1, ch), color(color) {}

        bool operator==(const Cell &other) const { return utf_char == other.utf_char && color == other.color; }
        bool operator!=(const Cell &other) const { return utf_char != other.utf_char || color != other.color; }

        friend std::ostream &operator<<(std::ostream &out, const Cell &cell)
        {
            out << cell.color.asANSI() << cell.utf_char;
            return out;
        }
    };

    class Window
    {
    private:
        int x, y;
        int width, height;
        int r, c;

        std::vector<std::vector<bool>> dirty; 
        std::vector<std::vector<Cell>> content;

        void move_string_to_cell(int row_index, const std::string &msg, int start_col, const COLOR& color)
        {
            size_t total_columns = content[row_index].size();
            size_t i = 0;
            size_t col_offset = 0;

            while (i < msg.length() && (start_col + col_offset) < total_columns)
            {
                size_t char_len = 1;
                unsigned char c = msg[i];
                if (c >= 0xFC) char_len = 6;
                else if (c >= 0xF8) char_len = 5;
                else if (c >= 0xF0) char_len = 4;
                else if (c >= 0xE0) char_len = 3;
                else if (c >= 0xC0) char_len = 2;

                if (i + char_len > msg.length()) char_len = msg.length() - i;

                std::string utf_symbol = msg.substr(i, char_len);
                content[row_index][start_col + col_offset] = Cell(utf_symbol, color);
                dirty[row_index][start_col + col_offset] = true;

                i += char_len;
                col_offset++;
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
            init_terminal();
            std::cout << COLOR::asANSI(COLOR::RESET);
            max_height = (std::max)(max_height, y + h);
            draw_border(title);

            content.resize(h - 2);
            dirty.resize(h - 2);
            for (int i = 0; i < h - 2; i++)
            {
                content[i].resize(w - 2, Cell(" ", COLOR::RESET));
                dirty[i].resize(w - 2, false);
            }
        }

        ~Window() {
            std::cout << COLOR::asANSI(COLOR::RESET);
        }

        void resize(int new_w, int new_h, const std::string &title = "") {
            std::lock_guard<std::mutex> lock(screen_lock);
            width = new_w;
            height = new_h;
            max_height = (std::max)(max_height, y + new_h);

            content.assign(height - 2, std::vector<Cell>(width - 2, Cell(" ", COLOR::RESET)));
            dirty.assign(height - 2, std::vector<bool>(width - 2, true)); // Force complete redraw on resize

            clear_inside();
            draw_border(title);
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
            if (clear_first) clear_inside();

            std::lock_guard<std::mutex> lock(screen_lock);

            size_t total_rows = content.size();
            size_t total_cols = content[0].size();

            std::stringstream ss;
            COLOR curr_color(COLOR::RESET);

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
                    ss << curr_color.asANSI() << content[i][j].utf_char;
                    dirty[i][j] = false;
                    j++;

                    while (j < total_cols) { 
                        if (curr_color == content[i][j].color) {
                            ss << content[i][j].utf_char;
                            dirty[i][j] = false;
                        } else {
                            ss << content[i][j].color.asANSI() << content[i][j].utf_char;
                            curr_color = content[i][j].color;
                            dirty[i][j] = false;
                        }
                        j++;
                    }
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

    namespace ThreeD {
            struct Point2D {
                int x, y;
                Point2D(int x=0, int y=0) : x(x), y(y) {}
            };

            struct Point3D {
                float x, y, z;
                Point3D(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}

                Point3D rotate(float angle) const {
                    float rad = angle * 0.0174533f;

                    float nx = x * cos(rad) - z * sin(rad);
                    float nz = x * sin(rad) + z * cos(rad);

                    float ny = y * cos(rad) - nz * sin(rad);
                    nz = y * sin(rad) + nz * cos(rad);

                    return Point3D(nx, ny, nz);
                }

                operator Point2D() const {
                    return Point2D(static_cast<int>(x), static_cast<int>(y));
                }
            };
    }

    namespace Visualizer
    {
        namespace Primitive
        {
            void draw_rectangle(Window &win, int row, int col, int width, int height, const COLOR& color = COLOR(COLOR::RESET), const std::string &ch = "#")
            {
                if (col < 0 || col + width > win.get_w() || row < 0 || row + height > win.get_h())
                    throw std::out_of_range("\nERROR: Rectangle dimensions exceed window bounds in draw_rectangle");

                for (int r = row; r < row + height; r++) {
                    std::string line = "";
                    for(int w_i = 0; w_i < width; ++w_i) line += ch;
                    win.print(r, col, line, color);
                }
            }

            void draw_line(Window& win, int x0, int y0, int x1, int y1, const COLOR& col, const std::string &ch = "#") {
                int dx = std::abs(x1 - x0);
                int dy = -std::abs(y1 - y0);
                int sx = (x0 < x1) ? 1 : -1;
                int sy = (y0 < y1) ? 1 : -1;
                
                int err = dx + dy; 
                int e2;

                while (true) {
                    if (x0 >= 0 && x0 < win.get_w() && y0 >= 0 && y0 < win.get_h()) {
                        win.print(y0, x0, ch, col);
                    }

                    if (x0 == x1 && y0 == y1) break;

                    e2 = 2 * err;
                    if (e2 >= dy) { err += dy; x0 += sx; }
                    if (e2 <= dx) { err += dx; y0 += sy; }
                }
            }
        }
        namespace Plots
        {
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

            void draw_bars(Window &win, const std::vector<int> &heights, int bar_width, const std::vector<COLOR> &colors = {}, const std::string &ch = "#")
            {
                if (heights.empty())
                    throw std::invalid_argument("\nERROR: Heights vector is empty in draw_bars");
                if (bar_width <= 0)
                    throw std::invalid_argument("\nERROR: Bar width must be positive in draw_bars");
                if (heights.size() * bar_width > static_cast<size_t>(win.get_w()))
                    throw std::out_of_range("\nERROR: Bars exceed window width in draw_bars");
                if (!colors.empty() && colors.size() != heights.size())
                    throw std::invalid_argument("\nERROR: Colors vector size must match heights vector size in draw_bars");

                win.clean_buffer();

                int total_rows = win.get_rows();
                int cols = heights.size();

                for (int i = 0; i < cols; i++)
                {
                    int bar_height = heights[i];
                    const COLOR& color = colors.empty() ? COLOR(COLOR::BLUE) : colors[i];

                    std::string bar_str = "";
                    for(int bw = 0; bw < bar_width; ++bw) bar_str += ch;

                    for (int r = total_rows - bar_height; r < total_rows; r++)
                        win.print(r, i * bar_width, bar_str, color);
                }
            }

            void draw_frame(Window &win, const std::vector<std::string> &chars, const std::vector<COLOR>& colors = {}) {
                if (chars.size() != colors.size()) throw std::invalid_argument("Size of characters do not match size of colors vector!\n");
                if (chars.size() != static_cast<size_t>(win.get_w() * win.get_h())) throw std::out_of_range("Total characters do not match window size!\n");

                win.clean_buffer();

                int total_chars = chars.size();
                int max_width = win.get_w();
                for (int i = 0; i < total_chars; i++) {
                    int x = i % max_width;
                    int y = i / max_width;

                    win.print(y, x, chars[i], colors[i]);
                }
            }

            void draw_progress_bar(
                Window &win, int row, 
                int col, int width, 
                std::function<int()> progress_func, 
                const COLOR& color = COLOR(COLOR::GREEN),
                const std::string &fill_ch = "#", const std::string &empty_ch = "="
            ) {

                if (col < 0 || col + width > win.get_w() || row < 0 || row >= win.get_h())
                    throw std::out_of_range("\nERROR: Progress bar dimensions exceed window bounds in draw_progress_bar");

                int progress = progress_func();
                assert(progress >= 0 && progress <= 100 && "Echo: Progress out of bounds!");

                win.print(row, col, "[", color);
                win.print(row, col + width - 1, "]", color);

                width -= 2; 
                col += 1; 
                
                int filled_length = static_cast<int>(width * (progress / 100.0f));
                int empty_length = width - filled_length;

                std::string filled_part = "";
                for(int i = 0; i < filled_length; ++i) filled_part += fill_ch;

                std::string empty_part = "";
                for(int i = 0; i < empty_length; ++i) empty_part += empty_ch;

                win.print(row, col, filled_part + empty_part, color);
            }
        }
        namespace ThreeD {

            using namespace echo::ThreeD;

            void draw_point3D(Window &win, const Point3D &point, const COLOR& color = COLOR(COLOR::RESET), const std::string &ch = "#") {
                Point2D p2d = static_cast<Point2D>(point); 
                if (p2d.x < 0 || p2d.x >= win.get_w() || p2d.y < 0 || p2d.y >= win.get_h())
                    throw std::out_of_range("\nERROR: 3D Point projects outside window bounds in draw_point3D");

                win.print(p2d.y, p2d.x, ch, color);
            }

            void draw_line3D(Window &win, const Point3D &p1, const Point3D &p2, const COLOR& color = COLOR(COLOR::RESET), const std::string &ch = "#") {
                Point2D s = static_cast<Point2D>(p1);
                Point2D e = static_cast<Point2D>(p2);

                int dx = abs(e.x - s.x), sx = s.x < e.x ? 1 : -1;
                int dy = -abs(e.y - s.y), sy = s.y < e.y ? 1 : -1;
                int err = dx + dy, e2;

                int steps = (std::max)(dx, abs(dy)); 
                int current_step = 0; 

                while (true) {
                    float t = (steps == 0) ? 1.0f : (float)current_step / steps;
                    float current_z = p1.z + t * (p2.z - p1.z);

                    float brightness = 1.0f / (1.0f + (current_z * 0.1f)); 
                    COLOR depth_color(
                        static_cast<uint8_t>(color.r * brightness),
                        static_cast<uint8_t>(color.g * brightness),
                        static_cast<uint8_t>(color.b * brightness)
                    );

                    if (s.x >= 0 && s.x < win.get_w() && s.y >= 0 && s.y < win.get_h()) {
                        win.print(s.y, s.x, ch, depth_color);
                    }

                    if (s.x == e.x && s.y == e.y) break;
                    e2 = 2 * err;
                    if (e2 >= dy) { err += dy; s.x += sx; }
                    if (e2 <= dx) { err += dx; s.y += sy; }
                    current_step++;
                }
            }
        }
    }
}