/*
 * coordenadas.c - Lector de coordenadas de pantalla
 * Compilar: gcc -o coordenadas.exe coordenadas.c -lgdi32 -luser32 -lcomctl32
 *
 * Overlay fullscreen semi-transparente click-through.
 * Click izquierdo -> registra coordenada + pide tiempo.
 * 'q' -> guarda pipeline_state/coordenadas.json y sale.
 */

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PTOS 200
#define ALPHA 64

typedef struct { int x, y; char tm[64]; char ts[32]; } Punto;

static struct {
    Punto pts[MAX_PTOS];
    int n, mx, my, paused, running;
    HWND hw, hDlg;
    HHOOK hk;
    HINSTANCE hi;
} S = {0};

/* Prototipos */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MouseProc(int, WPARAM, LPARAM);
void save_json(void);
void add_pt(int x, int y, const char* t);
void draw(HWND);

/* Manejador del dialogo de entrada */
static POINT dlg_pt;
INT_PTR CALLBACK DlgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_INITDIALOG) {
        wchar_t s[128];
        swprintf(s, 128, L"(%d,%d) Tiempo de espera (seg):", dlg_pt.x, dlg_pt.y);
        SetDlgItemText(h, 101, s);
        SetFocus(GetDlgItem(h, 102));
        return TRUE;
    }
    if (m == WM_COMMAND) {
        if (LOWORD(w) == IDOK) {
            wchar_t buf[64];
            GetDlgItemText(h, 102, buf, 64);
            char mb[64];
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, mb, 64, NULL, NULL);
            add_pt(dlg_pt.x, dlg_pt.y, mb);
            S.paused = 0;
            EndDialog(h, IDOK);
            return TRUE;
        }
        if (LOWORD(w) == IDCANCEL) {
            S.paused = 0;
            EndDialog(h, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

/* Hook global de mouse */
LRESULT CALLBACK MouseProc(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && wp == WM_LBUTTONDOWN && !S.paused && S.running) {
        MSLLHOOKSTRUCT* hs = (MSLLHOOKSTRUCT*)lp;
        S.paused = 1;
        dlg_pt = hs->pt;
        DialogBoxParam(S.hi, MAKEINTRESOURCE(100), S.hw, DlgProc, 0);
    }
    return CallNextHookEx(NULL, code, wp, lp);
}

void add_pt(int x, int y, const char* t) {
    if (S.n >= MAX_PTOS) return;
    Punto* p = &S.pts[S.n++];
    p->x = x; p->y = y;
    strncpy(p->tm, t, 63);
    time_t now = time(NULL);
    struct tm lc;
    localtime_s(&lc, &now);
    strftime(p->ts, 32, "%H:%M:%S", &lc);
    char line[256];
    snprintf(line, 256, "  #%d: (%d,%d) t:%s\n", S.n, x, y, t);
    printf("%s", line);
    InvalidateRect(S.hw, NULL, TRUE);
}

void save_json(void) {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    wchar_t* p = wcsrchr(path, L'\\');
    if (p) {
        wcscpy(p+1, L"pipeline_state");
        CreateDirectory(path, NULL);
        wcscat(path, L"\\coordenadas.json");
    }
    FILE* f = NULL;
    if (_wfopen_s(&f, path, L"w, ccs=UTF-8") || !f) return;
    fprintf(f, "{\n  \"puntos\": [\n");
    for (int i = 0; i < S.n; i++) {
        fprintf(f, "    {\"x\":%d,\"y\":%d,\"timeout\":\"%s\",\"timestamp\":\"%s\"}",
                S.pts[i].x, S.pts[i].y, S.pts[i].tm, S.pts[i].ts);
        fprintf(f, i < S.n-1 ? ",\n" : "\n");
    }
    fprintf(f, "  ],\n  \"screen\": {\"w\":%d,\"h\":%d}\n}\n",
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    fclose(f);
    printf("\nGuardado: %d puntos en %S\n", S.n, path);
}

void draw(HWND hw) {
    RECT rc;
    GetClientRect(hw, &rc);
    int w = rc.right, h = rc.bottom;
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hw, &ps);

    /* Fondo */
    HBRUSH bb = CreateSolidBrush(RGB(0,0,0));
    FillRect(dc, &rc, bb);
    DeleteObject(bb);

    SetBkMode(dc, TRANSPARENT);

    /* Cuadricula */
    HPEN pg = CreatePen(PS_SOLID, 1, RGB(50,50,50));
    SelectObject(dc, pg);
    HFONT fg = CreateFont(12,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,PROOF_QUALITY,0,L"Consolas");
    SelectObject(dc, fg);
    SetTextColor(dc, RGB(100,100,100));

    wchar_t lb[32];
    for (int x = 0; x < w; x += 100) {
        MoveToEx(dc, x, 0, NULL); LineTo(dc, x, h);
        swprintf(lb, 32, L"%d", x); TextOut(dc, x+2, 2, lb, wcslen(lb));
    }
    for (int y = 0; y < h; y += 100) {
        MoveToEx(dc, 0, y, NULL); LineTo(dc, w, y);
        swprintf(lb, 32, L"%d", y); TextOut(dc, 2, y+2, lb, wcslen(lb));
    }
    DeleteObject(pg);

    /* Info */
    HFONT fb = CreateFont(24,0,0,0,700,0,0,0,DEFAULT_CHARSET,0,0,PROOF_QUALITY,0,L"Consolas");
    SelectObject(dc, fb);
    SetTextColor(dc, RGB(0,200,0));
    TextOut(dc, 50, 15, L"Click IZQUIERDO = registrar coordenada", 42);

    /* Mouse coords */
    SetTextColor(dc, RGB(220,220,0));
    wchar_t mc[48];
    swprintf(mc, 48, L"X:%d  Y:%d", S.mx, S.my);
    TextOut(dc, 50, 45, mc, wcslen(mc));
    DeleteObject(fb);

    /* Puntos */
    HFONT fp = CreateFont(15,0,0,0,700,0,0,0,DEFAULT_CHARSET,0,0,PROOF_QUALITY,0,L"Consolas");
    SelectObject(dc, fp);
    for (int i = 0; i < S.n; i++) {
        int px = S.pts[i].x, py = S.pts[i].y;
        HPEN pp = CreatePen(PS_SOLID, 3, RGB(255,0,0));
        SelectObject(dc, pp);
        HBRUSH bp = CreateSolidBrush(RGB(255,0,0));
        SelectObject(dc, bp);
        Ellipse(dc, px-8, py-8, px+8, py+8);
        DeleteObject(pp); DeleteObject(bp);

        wchar_t tw[64]; MultiByteToWideChar(CP_UTF8,0,S.pts[i].tm,-1,tw,64);
        wchar_t lb[128];
        swprintf(lb,128,L"#%d (%d,%d) t:%s", i+1, px, py, tw);
        SetTextColor(dc, RGB(255,120,120));
        TextOut(dc, px+12, py-8, lb, wcslen(lb));
    }
    DeleteObject(fp);

    /* Help */
    HFONT fh = CreateFont(12,0,0,0,400,0,0,0,DEFAULT_CHARSET,0,0,PROOF_QUALITY,0,L"Consolas");
    SelectObject(dc, fh);
    SetTextColor(dc, RGB(120,120,120));
    TextOut(dc, 50, h-30, L"'q' = guardar y salir", 22);
    DeleteObject(fh);
    DeleteObject(fg);

    EndPaint(hw, &ps);
}

LRESULT CALLBACK WndProc(HWND hw, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_CREATE:
            S.hw = hw;
            SetTimer(hw, 1, 50, NULL);
            break;
        case WM_TIMER: {
            POINT pt;
            GetCursorPos(&pt);
            S.mx = pt.x; S.my = pt.y;
            InvalidateRect(hw, NULL, FALSE);
            break;
        }
        case WM_PAINT: draw(hw); return 0;
        case WM_CHAR:
            if (w == 'q' || w == 'Q') {
                save_json();
                S.running = 0;
                PostQuitMessage(0);
            }
            break;
        case WM_DESTROY:
            KillTimer(hw, 1);
            if (S.hk) UnhookWindowsHookEx(S.hk);
            PostQuitMessage(0);
            break;
        default: return DefWindowProc(hw, m, w, l);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    S.hi = hi; S.running = 1;

    /* Registrar clase */
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.lpszClassName = L"COverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClass(&wc)) return 1;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hw = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"COverlay", L"Coords", WS_POPUP,
        0, 0, sw, sh, NULL, NULL, hi, NULL);
    if (!hw) return 1;

    /* Transparencia uniforme */
    SetLayeredWindowAttributes(hw, 0, ALPHA, LWA_ALPHA);

    ShowWindow(hw, SW_SHOW);
    UpdateWindow(hw);

    /* Hook global de mouse */
    S.hk = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hi, 0);

    /* Consola para debug */
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    printf("== LECTOR DE COORDENADAS ==\n");
    printf("Click izquierdo -> registra\n'q' -> guarda y sale\n\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
