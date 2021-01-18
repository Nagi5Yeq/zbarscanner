#pragma once

#include "qrcodegen.h"

#include <Windows.h>

typedef struct {
    char data[4096];
    int len;
    BOOL binaryMode;
    enum qrcodegen_Ecc eccLevel;
    enum qrcodegen_Mask maskIndex;
    BOOL boostEccLevel;
    HDC memoryDC;
    HBITMAP bitmap;
    BOOL failed;
    RECT imageRect;
    uint8_t qrCode[qrcodegen_BUFFER_LEN_MAX];
} ResultContext;

INT_PTR CALLBACK ResultDlgFunc(HWND hWnd, UINT uMsg, WPARAM wParam,
                               LPARAM lParam);
