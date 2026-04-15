#pragma once
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class ProgressBar {
public:
    explicit ProgressBar(int width = 40) : width_(width) {
#ifdef _WIN32
        stdout_ = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO ci{};
        GetConsoleCursorInfo(stdout_, &ci);
        ci.bVisible = FALSE;
        SetConsoleCursorInfo(stdout_, &ci);

        CONSOLE_SCREEN_BUFFER_INFO sbi{};
        GetConsoleScreenBufferInfo(stdout_, &sbi);
        origin_row_ = sbi.dwCursorPosition.Y;
#endif
    }

    ~ProgressBar() { show_cursor(); }

    ProgressBar(const ProgressBar&) = delete;
    ProgressBar& operator=(const ProgressBar&) = delete;

    void update(int percent, const std::string& postfix) {
        percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
        int filled = (width_ * percent) / 100;

#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO sbi{};
        int term_width = 80;
        if (GetConsoleScreenBufferInfo(stdout_, &sbi))
            term_width = sbi.dwSize.X - 1;

        COORD c{ 0, origin_row_ };
        SetConsoleCursorPosition(stdout_, c);
#else
        int term_width = 80;
#endif

        char buf[512];
        int pos = 0;
        buf[pos++] = '[';
        for (int i = 0; i < width_; ++i)
            buf[pos++] = (i < filled) ? '#' : ' ';
        buf[pos++] = ']';

        // Truncate postfix so total output never exceeds terminal width
        int budget = term_width - pos - 8; // 8 = " 100%  " + some margin
        std::string p = postfix;
        if (budget > 0 && static_cast<int>(p.size()) > budget)
            p.resize(budget);
        else if (budget <= 0)
            p.clear();

        pos += snprintf(buf + pos, sizeof(buf) - pos, " %3d%%  %s", percent, p.c_str());

        // Pad to erase any leftover chars from a longer previous line
        while (pos < last_len_ && pos < term_width && pos < static_cast<int>(sizeof(buf)) - 1)
            buf[pos++] = ' ';
        buf[pos] = '\0';
        last_len_ = pos;

        fputs(buf, stdout);
        fflush(stdout);
    }

    void finish() {
        update(100, "");
        fputc('\n', stdout);
        fflush(stdout);
        show_cursor();
        cursor_restored_ = true;
    }

private:
    void show_cursor() {
        if (cursor_restored_) return;
#ifdef _WIN32
        CONSOLE_CURSOR_INFO ci{};
        GetConsoleCursorInfo(stdout_, &ci);
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(stdout_, &ci);
#endif
    }

#ifdef _WIN32
    HANDLE stdout_ = INVALID_HANDLE_VALUE;
    SHORT origin_row_ = 0;
#endif
    int width_;
    int last_len_ = 0;
    bool cursor_restored_ = false;
};
