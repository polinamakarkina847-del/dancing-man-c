#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<locale.h>
#include <windows.h>
#include<conio.h>
#include <math.h>
#include <time.h>
#include <mmsystem.h>

#ifndef M_PI
#define M_PI 3.14
#endif

#pragma comment(lib, "winmm.lib")

#define GRAPH_WIDTH   800
#define GRAPH_HEIGHT  600
#define TIMER_ID      1
#define TIMER_SPEED   40

HWND g_hWnd = NULL;
double g_phase = 0.0;
int g_muted = 0;
int g_beatCounter = 0;

int melody[] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int bassLine[] = { 131, 131, 165, 165, 196, 196, 220, 220 };

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawDancer(HDC hdc, double phase);
void PlaySoundFrequency(int freq, int durationMs, int volumePercent, int waveformType);
void PlayKick(void);
void PlaySnare(void);
void PlayHat(void);
void PlayMelodyNote(int noteIndex);
void PlayBass(int noteIndex);
void OnTimer(void);
void StartDancingProgram(void);

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void PlaySoundFrequency(int freq, int durationMs, int volumePercent, int waveformType)
{
    if (g_muted || freq <= 0) return;

    int sampleRate = 44100;
    int samples = (int)((double)sampleRate * durationMs / 1000.0);
    if (samples <= 0) return;

    short* buffer = (short*)malloc(samples * sizeof(short));
    if (!buffer) return;

    double vol = (volumePercent / 100.0) * 30000.0;

    for (int i = 0; i < samples; i++) {
        double t = (double)i / sampleRate;
        double value = 0;

        if (waveformType == 0) {
            value = sin(2.0 * M_PI * freq * t);
        }
        else if (waveformType == 1) {
            value = 2.0 * (t * freq - floor(0.5 + t * freq));
            value = (value - 1.0) * 0.8;
        }
        else if (waveformType == 2) {
            value = (double)(rand() % 200 - 100) / 100.0;
        }

        double envelope = 1.0;
        if (t < 0.005) envelope = t / 0.005;
        if (t > durationMs / 1000.0 - 0.05) {
            envelope *= (durationMs / 1000.0 - t) / 0.05;
        }
        envelope *= exp(-t * 3.0);

        buffer[i] = (short)(value * vol * envelope);
    }

    WAVEFORMATEX wf = { 0 };
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = sampleRate;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

    HWAVEOUT hWaveOut;
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        free(buffer);
        return;
    }

    WAVEHDR header = { 0 };
    header.lpData = (LPSTR)buffer;
    header.dwBufferLength = samples * sizeof(short);
    header.dwFlags = 0;

    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

    while (!(header.dwFlags & WHDR_DONE)) {
        Sleep(1);
    }

    waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
    free(buffer);
}

void PlayKick(void)
{
    PlaySoundFrequency(60, 200, 80, 1);
}

void PlaySnare(void)
{
    PlaySoundFrequency(180, 150, 65, 1);
}

void PlayHat(void)
{
    PlaySoundFrequency(4500, 40, 30, 2);
}

void PlayMelodyNote(int noteIndex)
{
    int note = melody[noteIndex % 8];
    PlaySoundFrequency(note, 180, 45, 0);
}

void PlayBass(int noteIndex)
{
    int note = bassLine[noteIndex % 8];
    PlaySoundFrequency(note, 200, 55, 1);
}

void OnTimer(void)
{
    g_phase += 0.15;
    if (g_phase > M_PI * 2) g_phase -= M_PI * 2;

    int beatPos = (int)(g_phase / (M_PI * 2 / 16.0)) % 16;

    if (beatPos == 0) {
        PlayKick();
        PlayMelodyNote(g_beatCounter % 8);
        PlayBass(g_beatCounter % 8);
    }
    if (beatPos == 4) {
        PlayKick();
        PlayMelodyNote((g_beatCounter + 2) % 8);
    }
    if (beatPos == 8) {
        PlaySnare();
        PlayMelodyNote((g_beatCounter + 4) % 8);
    }
    if (beatPos == 12) {
        PlaySnare();
        PlayMelodyNote((g_beatCounter + 6) % 8);
    }

    if (beatPos % 2 == 1) {
        PlayHat();
    }

    if (beatPos == 15) {
        g_beatCounter++;
    }

    InvalidateRect(g_hWnd, NULL, TRUE);
    UpdateWindow(g_hWnd);
}

void DrawDancer(HDC hdc, double phase)
{
    double bodySway = sin(phase) * 0.1;
    double armAngleLeft = sin(phase * 2.0) * 1.0;
    double armAngleRight = sin(phase * 2.0 + M_PI) * 1.0;
    double legAngleLeft = sin(phase * 2.5) * 0.9;
    double legAngleRight = sin(phase * 2.5 + M_PI) * 0.9;
    double bounceY = fabs(sin(phase * 3.0)) * 12.0;
    double headBob = sin(phase * 2.0) * 3.0;

    int centerX = GRAPH_WIDTH / 2;
    int centerY = GRAPH_HEIGHT / 2 + 50;

    int headX = centerX + (int)headBob;
    int headY = centerY - 70 + (int)bounceY;

    HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    HBRUSH skinBrush = CreateSolidBrush(RGB(255, 220, 180));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, skinBrush);

    Ellipse(hdc, headX - 25, headY - 30, headX + 25, headY + 5);

    int eyeOffsetX = (int)(sin(phase * 4) * 2);
    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    Ellipse(hdc, headX - 12 + eyeOffsetX, headY - 15, headX - 6 + eyeOffsetX, headY - 9);
    Ellipse(hdc, headX + 6 + eyeOffsetX, headY - 15, headX + 12 + eyeOffsetX, headY - 9);

    int smileH = 4 + (int)(sin(phase * 4) * 2);
    POINT smile[3] = {
        {headX - 12, headY - 2},
        {headX, headY + smileH},
        {headX + 12, headY - 2}
    };
    Polyline(hdc, smile, 3);

    MoveToEx(hdc, headX, headY + 5, NULL);
    LineTo(hdc, headX, headY + 20);

    int bodyEndX = centerX + (int)(bodySway * 50);
    int bodyEndY = centerY + 20 + (int)bounceY;

    HPEN bodyPen = CreatePen(PS_SOLID, 6, RGB(50, 100, 200));
    SelectObject(hdc, bodyPen);
    MoveToEx(hdc, headX, headY + 20, NULL);
    LineTo(hdc, bodyEndX, bodyEndY);

    int shoulderX = headX + (int)(bodySway * 20);
    int shoulderY = headY + 35;

    int handLX = shoulderX + (int)(sin(armAngleLeft) * 60);
    int handLY = shoulderY + (int)(cos(armAngleLeft) * 45);
    int handRX = shoulderX + (int)(sin(armAngleRight) * 60);
    int handRY = shoulderY + (int)(cos(armAngleRight) * 45);

    HPEN limbPen = CreatePen(PS_SOLID, 4, RGB(50, 150, 200));
    SelectObject(hdc, limbPen);
    MoveToEx(hdc, shoulderX, shoulderY, NULL);
    LineTo(hdc, handLX, handLY);
    MoveToEx(hdc, shoulderX, shoulderY, NULL);
    LineTo(hdc, handRX, handRY);

    int hipX = bodyEndX;
    int hipY = bodyEndY;

    int footLX = hipX + (int)(sin(legAngleLeft) * 45);
    int footLY = hipY + (int)(cos(legAngleLeft) * 55);
    int footRX = hipX + (int)(sin(legAngleRight) * 45);
    int footRY = hipY + (int)(cos(legAngleRight) * 55);

    MoveToEx(hdc, hipX, hipY, NULL);
    LineTo(hdc, footLX, footLY);
    MoveToEx(hdc, hipX, hipY, NULL);
    LineTo(hdc, footRX, footRY);

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    Ellipse(hdc, footLX - 10, footLY - 6, footLX + 10, footLY + 10);
    Ellipse(hdc, footRX - 10, footRY - 6, footRX + 10, footRY + 10);

    HBRUSH hatBrush = CreateSolidBrush(RGB(80, 50, 30));
    SelectObject(hdc, hatBrush);
    Rectangle(hdc, headX - 30, headY - 52, headX + 30, headY - 38);
    Rectangle(hdc, headX - 16, headY - 65, headX + 16, headY - 50);

    DeleteObject(hatBrush);
    DeleteObject(bodyPen);
    DeleteObject(limbPen);
    DeleteObject(pen);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(skinBrush);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        SetTimer(hWnd, TIMER_ID, TIMER_SPEED, NULL);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect = { 0, 0, GRAPH_WIDTH, GRAPH_HEIGHT };
        HBRUSH bgBrush = CreateSolidBrush(RGB(10, 20, 50));
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        DrawDancer(hdc, g_phase);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(150, 150, 150));
        if (g_muted) {
            TextOutA(hdc, 20, 20, "M - SOUND OFF", 13);
        }
        else {
            TextOutA(hdc, 20, 20, "M - SOUND ON", 12);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hWnd);
        }
        if (wParam == 'M') {
            g_muted = !g_muted;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            OnTimer();
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void StartDancingProgram(void)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"DancingMan";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Ошибка", L"Ошибка", MB_OK);
        return;
    }

    g_hWnd = CreateWindowEx(
        0,
        L"DancingMan",
        L"Танцующий человечек - Задача",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        GRAPH_WIDTH, GRAPH_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) {
        MessageBox(NULL, L"Ошибка", L"Ошибка", MB_OK);
        return;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    int choice;

    do {
        system("cls");
        gotoxy(53, 4);
        printf("----------------\n");
        gotoxy(53, 5);
        printf("| ГЛАВНОЕ МЕНЮ:|\n");
        gotoxy(53, 6);
        printf("|              |\n");
        gotoxy(53, 7);
        printf("|  1. Заставка |\n");
        gotoxy(53, 8);
        printf("|  2. Задача   |\n");
        gotoxy(53, 9);
        printf("|  3. Об авторе|\n");
        gotoxy(53, 10);
        printf("|  4. Выход    |\n");
        gotoxy(53, 11);
        printf("----------------\n");
        gotoxy(55, 13);
        printf("Ваш выбор: ");
        scanf_s("%d", &choice);
        getchar();

        switch (choice) {
        case 1: {
            system("cls");

            char title[] = "ТАНЦУЮЩИЙ ЧЕЛОВЕЧЕК";
            char subtitle[] = "Курсовая работа по прое ктной деятельности";
            char author[] = "Макаркина Полина Ильинична";
            char group[] = "ИВТ-253, ОмГТУ, 2026";

            for (int i = 0; i < 79; i++) {
                gotoxy(i, 3); printf("=");
                gotoxy(i, 20); printf("=");
            }
            for (int i = 3; i < 21; i++) {
                gotoxy(0, i); printf("|");
                gotoxy(78, i); printf("|");
            }

            int len = (int)strlen(title);
            for (int i = 0; i < len; i++) {
                gotoxy(40 - len / 2 + i, 7);
                printf("%c", title[i]);
                Sleep(50);
            }

            Sleep(200);

            len = (int)strlen(subtitle);
            for (int i = 0; i < len; i++) {
                gotoxy(40 - len / 2 + i, 9);
                printf("%c", subtitle[i]);
                Sleep(30);
            }

            Sleep(200);

            len = (int)strlen(author);
            for (int i = 0; i < len; i++) {
                gotoxy(40 - len / 2 + i, 12);
                printf("%c", author[i]);
                Sleep(30);
            }

            Sleep(200);

            len = (int)strlen(group);
            for (int i = 0; i < len; i++) {
                gotoxy(40 - len / 2 + i, 14);
                printf("%c", group[i]);
                Sleep(30);
            }

            char runningText[] = ">>> НАЖМИТЕ ЛЮБУЮ КЛАВИШУ, ЧТОБЫ ПРОДОЛЖИТЬ <<<";
            len = (int)strlen(runningText);

            for (int frame = 0; frame < len + 78; frame++) {
                for (int i = 0; i < 78; i++) {
                    gotoxy(1, 18);
                    printf(" ");
                }
                for (int i = 0; i < len; i++) {
                    int pos = frame + i;
                    if (pos >= 1 && pos <= 77) {
                        gotoxy(pos, 18);
                        printf("%c", runningText[i]);
                    }
                }
                Sleep(30);
                if (_kbhit()) {
                    _getch();  
                    break;
                }
            }

            gotoxy(30, 22);
            printf("Нажмите Enter для продолжения...");
            getchar();
            getchar();
            break;
        }

        case 2: {
            system("cls");
            int choice2;

            do {
                system("cls");
                gotoxy(53, 5);
                printf("ПОДМЕНЮ ЗАДАЧА:\n");
                gotoxy(53, 7);
                printf("1. Танцующий человечек\n");
                gotoxy(53, 8);
                printf("2. Выход\n");
                gotoxy(53, 10);
                printf("Ваш выбор: ");
                scanf_s("%d", &choice2);
                getchar();

                switch (choice2) {
                case 1:
                    system("cls");
                    printf("Запуск программы 'Танцующий человечек'...\n");
                    printf("Закройте окно с человечком для возврата в меню.\n");
                    printf("Нажмите Enter для запуска...");
                    getchar();
                    StartDancingProgram();
                    break;
                case 2:
                    break;
                default:
                    gotoxy(45, 15);
                    printf("Неверный выбор!");
                    getchar();
                    getchar();
                    break;
                }
            } while (choice2 != 2);
            break;
        }

        case 3:
            system("cls");
            gotoxy(55, 6);
            printf("ОБ АВТОРЕ:\n");
            gotoxy(45, 8);
            printf("ФИО студента: Макаркина Полина Ильинична\n");
            gotoxy(45, 9);
            printf("ВУЗ: ОмГТУ\n");
            gotoxy(45, 10);
            printf("Предмет: проектная деятельность\n");
            gotoxy(45, 11);
            printf("Год: 2026\n");
            gotoxy(45, 12);
            printf("Группа: ИВТ-253\n");
            gotoxy(45, 14);
            printf("Нажмите Enter...");
            getchar();
            getchar();
            break;

        case 4:
            system("cls");
            gotoxy(55, 10);
            printf("До новых встреч!\n\n");
            break;

        default:
            gotoxy(55, 15);
            printf("Неверный выбор!");
            getchar();
            getchar();
            break;
        }
    } while (choice != 4);

    return 0;
}