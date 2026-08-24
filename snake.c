#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

int main(void) {
    printf("========================================\n");
    printf("   LSM Integrated In-Game UI Snake Engine\n");
    printf("========================================\n");

    HWND hwnd = CreateWindowExA(
        0, 
        "STATIC", 
        "LSM Snake [In-Game UI & Auto-Pause]", 
        281739264,
        150, 150, 620, 660, 
        0, 0, 0, 0
    );

    HDC hdc = GetDC(hwnd);

    SetBkMode(hdc, 1);

    int64_t body_x[300];
    int64_t body_y[300];

    HBRUSH bgBrush = CreateSolidBrush(1644825);
    HBRUSH snakeBrush = CreateSolidBrush(3329330);
    HBRUSH foodBrush = CreateSolidBrush(16724786);

    int64_t app_running = 1;

    while (app_running == 1) {
        int64_t length = 3;
        body_x[0] = 300; body_y[0] = 320;
        body_x[1] = 280; body_y[1] = 320;
        body_x[2] = 260; body_y[2] = 320;

        int64_t dir_x = 20;
        int64_t dir_y = 0;

        int64_t food_x = 440;
        int64_t food_y = 320;
        int64_t score = 0;

        int64_t is_alive = 1;
        int64_t game_over_screen = 0;

        MessageBeep(64);

        while (is_alive == 1) {
            PeekMessageA(0, hwnd, 0, 0, 1);

            HWND active_win = GetForegroundWindow();
            if (active_win != hwnd) {
                SetTextColor(hdc, 65535);
                TextOutA(hdc, 240, 280, "[ PAUSED - CLICK TO RESUME ]", 28);
                Sleep(100);
            } else {
                if (GetAsyncKeyState(38) != 0) {
                    if (dir_y == 0) { dir_x = 0; dir_y = 0 - 20; }
                }
                if (GetAsyncKeyState(40) != 0) {
                    if (dir_y == 0) { dir_x = 0; dir_y = 20; }
                }
                if (GetAsyncKeyState(37) != 0) {
                    if (dir_x == 0) { dir_x = 0 - 20; dir_y = 0; }
                }
                if (GetAsyncKeyState(39) != 0) {
                    if (dir_x == 0) { dir_x = 20; dir_y = 0; }
                }

                int64_t i = length - 1;
                while (i > 0) {
                    body_x[i] = body_x[i - 1];
                    body_y[i] = body_y[i - 1];
                    i = i - 1;
                }

                body_x[0] = body_x[0] + dir_x;
                body_y[0] = body_y[0] + dir_y;

                if (body_x[0] < 0) { is_alive = 0; }
                if (body_x[0] >= 600) { is_alive = 0; }
                if (body_y[0] < 40) { is_alive = 0; }
                if (body_y[0] >= 600) { is_alive = 0; }

                int64_t k = 1;
                while (k < length) {
                    if (body_x[0] == body_x[k]) {
                        if (body_y[0] == body_y[k]) {
                            is_alive = 0;
                        }
                    }
                    k = k + 1;
                }

                if (body_x[0] == food_x) {
                    if (body_y[0] == food_y) {
                        score = score + 10;
                        length = length + 1;
                        MessageBeep(0);

                        food_x = (((food_x + 80) * 17) % 540) / 20 * 20 + 20;
                        food_y = (((food_y + 60) * 23) % 520) / 20 * 20 + 60;
                    }
                }

                SelectObject(hdc, bgBrush);
                Rectangle(hdc, 0, 0, 600, 600);

                SetTextColor(hdc, 16777215);
                TextOutA(hdc, 20, 10, "LSM SNAKE 2026", 14);
                TextOutA(hdc, 450, 10, "SCORE: [ +10/Apple ]", 20);

                SelectObject(hdc, foodBrush);
                Rectangle(hdc, food_x, food_y, food_x + 18, food_y + 18);

                SelectObject(hdc, snakeBrush);
                int64_t j = 0;
                while (j < length) {
                    int64_t bx = body_x[j];
                    int64_t by = body_y[j];
                    Rectangle(hdc, bx, by, bx + 18, by + 18);
                    j = j + 1;
                }

                Sleep(70);
            }
        }

        MessageBeep(16);
        game_over_screen = 1;

        while (game_over_screen == 1) {
            PeekMessageA(0, hwnd, 0, 0, 1);

            SelectObject(hdc, bgBrush);
            Rectangle(hdc, 100, 200, 500, 400);

            SetTextColor(hdc, 255);
            TextOutA(hdc, 245, 230, "G A M E   O V E R", 17);

            SetTextColor(hdc, 65535);
            TextOutA(hdc, 170, 280, "PRESS [ SPACE ] OR [ ENTER ] TO RETRY", 37);

            SetTextColor(hdc, 12632256);
            TextOutA(hdc, 210, 330, "PRESS [ ESC ] TO EXIT GAME", 26);

            if (GetAsyncKeyState(32) != 0) {
                game_over_screen = 0;
            }
            if (GetAsyncKeyState(13) != 0) {
                game_over_screen = 0;
            }

            if (GetAsyncKeyState(27) != 0) {
                game_over_screen = 0;
                app_running = 0;
            }

            Sleep(50);
        }
    }

    DeleteObject(bgBrush);
    DeleteObject(snakeBrush);
    DeleteObject(foodBrush);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    return 0;
}
