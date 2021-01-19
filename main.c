#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "resource.h"
#include "result.h"
#include "stb_image.h"
#include "zbar.h"

#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>

#define WM_NOFITYICONTRAY ((WM_APP) + 1)

HINSTANCE instance;
HWND window;
HICON icon;
HMENU menu = NULL;
NOTIFYICONDATA ni;
BOOL rawMode = FALSE;
UINT taskbarCreatedMsg;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void ShowTrayIcon();
void InitMenu();
void HandleTrayIcon(LPARAM lParam);
void FullScreenScan();
void ImageFileScan();
void* GetScreenBits(int x, int y, int w, int h);
void* ConvertRGBToY800(void* rgbData, int w, int h);
void ZbarScanImage(zbar_image_t* image);
void OpenResultDialog(ResultContext* ctx, const char* data, int len);
INT_PTR CALLBACK AboutDlgFunc(HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nShowCmd) {
    MSG message;
    WNDCLASS zbarClass;

    InitCommonControls();
    instance = hInstance;
    icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
    memset(&zbarClass, 0, sizeof(WNDCLASS));
    zbarClass.hInstance = instance;
    zbarClass.lpszClassName = TEXT("zbarScanner");
    zbarClass.lpfnWndProc = WndProc;
    zbarClass.style = CS_DBLCLKS;
    zbarClass.hIcon = icon;
    zbarClass.hCursor = NULL;
    if (!RegisterClass(&zbarClass)) {
        return 1;
    }
    window = CreateWindow(TEXT("zbarScanner"), TEXT("QR scanner"),
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          640, 320, HWND_DESKTOP, NULL, instance, NULL);
    if (window == NULL) {
        return 2;
    }
    InitMenu();
    ShowTrayIcon();
    taskbarCreatedMsg = RegisterWindowMessage(TEXT("TaskBarCreated"));
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
    ResultContext* ctx;

    switch (message) {
    case WM_NOFITYICONTRAY:
        HandleTrayIcon(lParam);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_SCAN) {
            FullScreenScan();
        } else if (LOWORD(wParam) == IDM_FROMIMAGE) {
            ImageFileScan();
        } else if (LOWORD(wParam) == IDM_RAWMODE) {
            rawMode = !rawMode;
            CheckMenuItem(menu, IDM_RAWMODE,
                          rawMode ? MF_CHECKED : MF_UNCHECKED);
        } else if (LOWORD(wParam) == IDM_OPEN) {
            ctx = malloc(sizeof(ResultContext));
            if (ctx != NULL) {
                OpenResultDialog(ctx, "", 0);
            }
        } else if (LOWORD(wParam) == IDM_EXIT) {
            DestroyWindow(window);
        } else if (LOWORD(wParam) == IDM_ABOUT) {
            DialogBox(instance, MAKEINTRESOURCE(IDD_ABOUT), window,
                      AboutDlgFunc);
        }
        break;
    case WM_DESTROY:
        DestroyMenu(menu);
        Shell_NotifyIcon(NIM_DELETE, &ni);
        PostQuitMessage(0);
        return 0;
    default:
        if (message == taskbarCreatedMsg) {
            ShowTrayIcon();
            return 0;
        }
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitMenu() {
    menu = CreatePopupMenu();
    AppendMenu(menu, 0, IDM_SCAN, TEXT("扫描全屏幕"));
    // AppendMenu(menu, 0, IDM_SELECTSCAN, TEXT("扫描选定屏幕区域"));
    AppendMenu(menu, 0, IDM_FROMIMAGE, TEXT("扫描文件"));
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, 0, IDM_RAWMODE, TEXT("二进制模式"));
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, 0, IDM_OPEN, TEXT("打开编辑器"));
    AppendMenu(menu, 0, IDM_ABOUT, TEXT("关于"));
    AppendMenu(menu, 0, IDM_EXIT, TEXT("退出"));
}

void ShowTrayIcon() {
    ni.cbSize = sizeof(NOTIFYICONDATA);
    ni.uID = 0;
    ni.hWnd = window;
    ni.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    ni.uCallbackMessage = WM_NOFITYICONTRAY;
    ni.hIcon = icon;
    (void) lstrcpyn(ni.szTip, TEXT("QR scanner"), _countof(ni.szTip));
    Shell_NotifyIcon(NIM_ADD, &ni);
}

void HandleTrayIcon(LPARAM lParam) {
    POINT pt;
    switch (lParam) {
    case WM_RBUTTONUP:
        GetCursorPos(&pt);
        SetForegroundWindow(window);
        TrackPopupMenu(menu, TPM_RIGHTALIGN, pt.x, pt.y, 0, window, NULL);
        break;
    case WM_LBUTTONUP:
        SendMessage(window, WM_COMMAND, IDM_SCAN, 0);
        break;
    }
}

void* GetScreenBits(int x, int y, int w, int h) {
    HDC desktopDC, memoryDC;
    HBITMAP bitmap;
    BITMAPINFO bitmapHeader;
    void* data;

    desktopDC = GetDC(NULL);
    memoryDC = CreateCompatibleDC(desktopDC);
    bitmap = CreateCompatibleBitmap(desktopDC, w, h);
    ZeroMemory(&bitmapHeader, sizeof(BITMAPINFO));
    bitmapHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapHeader.bmiHeader.biWidth = w;
    bitmapHeader.bmiHeader.biHeight = h;
    bitmapHeader.bmiHeader.biPlanes = 1;
    bitmapHeader.bmiHeader.biBitCount = 24;
    SelectObject(memoryDC, bitmap);
    data = malloc((size_t)((w * 3 + 3) / 4) * 4 * h);
    if (data == NULL) {
        return NULL;
    }
    BitBlt(memoryDC, 0, 0, w, h, desktopDC, x, y, SRCCOPY);
    GetDIBits(memoryDC, bitmap, 0, h, data, &bitmapHeader, DIB_RGB_COLORS);
    DeleteDC(memoryDC);
    DeleteObject(bitmap);
    ReleaseDC(NULL, desktopDC);
    return data;
}

void FullScreenScan() {
    int w, h, x, y;
    void *RGBData, *Y800Data;
    zbar_image_t* image;

    w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    RGBData = GetScreenBits(x, y, w, h);
    if (RGBData == NULL) {
        return;
    }
    Y800Data = ConvertRGBToY800(RGBData, w, h);
    free(RGBData);
    if (Y800Data == NULL) {
        return;
    }
    image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y', '8', '0', '0'));
    zbar_image_set_size(image, w, h);
    zbar_image_set_data(image, Y800Data, w * h, zbar_image_free_data);
    ZbarScanImage(image);
    zbar_image_destroy(image);
}

void* ConvertRGBToY800(void* RGBData, int w, int h) {
    int x, y, line;
    uint8_t r, g, b;
    uint16_t y0;
    void* y800Data;
    uint8_t *srcp, *dstp;

    y800Data = malloc((size_t) w * h);
    if (y800Data == NULL) {
        return NULL;
    }
    dstp = y800Data;
    line = (w * 3 + 3) / 4 * 4;
    for (y = 0; y < h; y++) {
        srcp = (uint8_t*) RGBData + y * line;
        for (x = 0; x < w; x++) {
            r = *srcp;
            g = *(srcp + 1);
            b = *(srcp + 2);
            srcp += 3;
            y0 = ((77 * r + 150 * g + 29 * b) + 0x80) >> 8;
            *(dstp++) = (uint8_t) y0;
        }
    }
    return y800Data;
}

void ZbarScanImage(zbar_image_t* image) {
    zbar_image_scanner_t* scanner;
    const zbar_symbol_t* symbol;
    const char* message;
    int result;
    unsigned int len;
    ResultContext* ctx;

    scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
    if (rawMode == TRUE) {
        zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_BINARY, 1);
    }
    result = zbar_scan_image(scanner, image);
    if (result <= 0) {
        MessageBox(window, TEXT("未找到二维码，可以尝试选定区域扫描。"),
                   TEXT("扫描失败"), MB_ICONINFORMATION);
    }
    for (symbol = zbar_image_first_symbol(image); symbol != NULL;
         symbol = zbar_symbol_next(symbol)) {
        message = zbar_symbol_get_data(symbol);
        len = zbar_symbol_get_data_length(symbol);
        ctx = malloc(sizeof(ResultContext));
        if (ctx == NULL) {
            continue;
        }
        OpenResultDialog(ctx, message, len);
    }
    zbar_image_scanner_destroy(scanner);
}

void OpenResultDialog(ResultContext* ctx, const char* data, int len) {
    len = min(len, 4095);
    memcpy(ctx->data, data, len);
    ctx->data[len] = '\0';
    ctx->len = len;
    ctx->eccLevel = qrcodegen_Ecc_MEDIUM;
    ctx->maskIndex = qrcodegen_Mask_AUTO;
    ctx->binaryMode = rawMode;
    ctx->boostEccLevel = TRUE;
    ctx->failed = TRUE;
    DialogBoxParam(instance, MAKEINTRESOURCE(IDD_RESULTDLG), window,
                   ResultDlgFunc, (LPARAM) ctx);
}

void StbiFreeImageHandler(zbar_image_t* image) {
    stbi_image_free((void*) zbar_image_get_data(image));
}

void ImageFileScan() {
    OPENFILENAMEA ofna;
    TCHAR fileName[256];
    void* Y800Data;
    int x, y, channels;
    zbar_image_t* image;

    fileName[0] = 0;
    memset(&ofna, 0, sizeof(OPENFILENAMEA));
    ofna.lStructSize = sizeof(OPENFILENAMEA);
    ofna.hwndOwner = window;
    ofna.lpstrFilter = "图片文件\0*.*\0\0";
    ofna.nFilterIndex = 1;
    ofna.lpstrFile = fileName;
    ofna.nMaxFile = 256;
    ofna.lpstrTitle = "打开图片文件";
    if (GetOpenFileName(&ofna) == 0) {
        return;
    }
    Y800Data = (void*) stbi_load(fileName, &x, &y, &channels, 1);
    if (Y800Data == NULL) {
        MessageBox(window, TEXT("图片加载失败。"), TEXT("扫描失败"),
                   MB_ICONWARNING);
        return;
    }
    image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y', '8', '0', '0'));
    zbar_image_set_size(image, x, y);
    zbar_image_set_data(image, Y800Data, x * y, StbiFreeImageHandler);
    ZbarScanImage(image);
    zbar_image_destroy(image);
}

INT_PTR CALLBACK AboutDlgFunc(HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam) {
    PNMLINK link;

    switch (uMsg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hWnd, 0);
            return TRUE;
        }
        break;
    case WM_NOTIFY:
        link = (PNMLINK) lParam;
        if (link->hdr.code == NM_CLICK && link->hdr.idFrom >= IDC_ABOUT &&
            link->hdr.idFrom <= IDC_ABOUT3) {
            ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOW);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;
}
