#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "result.h"
#include "qrcodegen.h"
#include "resource.h"
#include "stb_image_write.h"

#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>

extern HWND window;
extern HICON icon;

LPWSTR ConvertToUnicode(UINT codePage, char* utf8, int len, int* wideLen);
LPWSTR ConvertToBinary(char* data, int len);
int DecodeBinaryData(LPWSTR str, int len, unsigned char* out, int maxLen);
void RenewDialogItem(ResultContext* ctx, HWND hWnd);
void SetupComboxItem(HWND hWnd);
void RenewEditContent(ResultContext* ctx, HWND hWnd);
void RegenerateQrCode(ResultContext* ctx, HWND hWnd);
void SaveImageToFile(ResultContext* ctx);

INT_PTR CALLBACK ResultDlgFunc(HWND hWnd, UINT uMsg, WPARAM wParam,
                               LPARAM lParam) {
    ResultContext* ctx;
    HDC windowDC;
    PAINTSTRUCT ps;
    WCHAR wideContent[8192];
    int wLen, size, limit, scale, x, y, scaledSize;
    COLORREF originalBk, originalText;

    if (uMsg != WM_INITDIALOG) {
        ctx = (ResultContext*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, (LONG_PTR) WS_EX_APPWINDOW);
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) icon);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) lParam);
        ctx = (ResultContext*) lParam;
        windowDC = GetDC(hWnd);
        ctx->memoryDC = CreateCompatibleDC(windowDC);
        ctx->bitmap = CreateCompatibleBitmap(ctx->memoryDC, 180, 180);
        SelectObject(ctx->memoryDC, ctx->bitmap);
        ReleaseDC(hWnd, windowDC);
        ctx->imageRect.left = 7;
        ctx->imageRect.top = 126;
        ctx->imageRect.right = 248;
        ctx->imageRect.bottom = 243;
        MapDialogRect(hWnd, &ctx->imageRect);
        RenewEditContent(ctx, hWnd);
        SetupComboxItem(hWnd);
        RenewDialogItem(ctx, hWnd);
        RegenerateQrCode(ctx, hWnd);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_RESULTEDIT && HIWORD(wParam) == EN_CHANGE) {
            wLen = GetWindowTextW((HWND) lParam, wideContent, 8192);
            if (ctx->binaryMode == TRUE) {
                ctx->len = DecodeBinaryData(wideContent, wLen, ctx->data, 4095);
            } else {
                ctx->len = WideCharToMultiByte(CP_UTF8, 0, wideContent, wLen,
                                               ctx->data, 4095, NULL, NULL);
            }
            ctx->data[ctx->len] = '\0';
            RegenerateQrCode(ctx, hWnd);
            return TRUE;
        } else if (LOWORD(wParam) == IDOK) {
            EndDialog(hWnd, 0);
            return TRUE;
        } else if (LOWORD(wParam) == IDC_BINARY &&
                   HIWORD(wParam) == BN_CLICKED) {
            ctx->binaryMode = IsDlgButtonChecked(hWnd, IDC_BINARY);
            RenewEditContent(ctx, hWnd);
            return TRUE;
        } else if (LOWORD(wParam) == IDC_BOOSTECC &&
                   HIWORD(wParam) == BN_CLICKED) {
            ctx->boostEccLevel = IsDlgButtonChecked(hWnd, IDC_BOOSTECC);
            RegenerateQrCode(ctx, hWnd);
            return TRUE;
        } else if (LOWORD(wParam) == IDC_ECCLEVEL &&
                   HIWORD(wParam) == CBN_SELCHANGE) {
            ctx->eccLevel =
                (int) SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
            RegenerateQrCode(ctx, hWnd);
            return TRUE;
        } else if (LOWORD(wParam) == IDC_MASK &&
                   HIWORD(wParam) == CBN_SELCHANGE) {
            ctx->maskIndex =
                (int) SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0) - 1;
            RegenerateQrCode(ctx, hWnd);
            return TRUE;
        } else if (LOWORD(wParam) == IDSAVEIMAGE) {
            SaveImageToFile(ctx);
        }
        break;
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    case WM_DESTROY:
        DeleteDC(ctx->memoryDC);
        DeleteObject(ctx->bitmap);
        free(ctx);
        return TRUE;
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        if (ctx->failed == TRUE) {
            EndPaint(hWnd, &ps);
            return TRUE;
        }
        size = qrcodegen_getSize(ctx->qrCode) + 6;
        limit = ctx->imageRect.bottom - ctx->imageRect.top;
        for (scale = 8; scale >= 1; scale--) {
            scaledSize = size * scale;
            if (scaledSize <= limit) {
                x = (ctx->imageRect.left + ctx->imageRect.right - scaledSize) /
                    2;
                y = (ctx->imageRect.top + ctx->imageRect.bottom - scaledSize) /
                    2;
                originalText = SetTextColor(ps.hdc, RGB(0, 0, 0));
                originalBk = SetBkColor(ps.hdc, RGB(255, 255, 255));
                StretchBlt(ps.hdc, x, y, scaledSize, scaledSize, ctx->memoryDC,
                           0, 0, size, size, SRCCOPY);
                SetTextColor(ps.hdc, originalText);
                SetBkColor(ps.hdc, originalBk);
                break;
            }
        }
        EndPaint(hWnd, &ps);
        return TRUE;
    }
    return FALSE;
}

void SetupComboxItem(HWND hWnd) {
    HWND hEccLevel, hMask;

    hEccLevel = GetDlgItem(hWnd, IDC_ECCLEVEL);
    hMask = GetDlgItem(hWnd, IDC_MASK);
    SendMessage(hEccLevel, CB_ADDSTRING, 0, (LPARAM) TEXT("低"));
    SendMessage(hEccLevel, CB_ADDSTRING, 0, (LPARAM) TEXT("中"));
    SendMessage(hEccLevel, CB_ADDSTRING, 0, (LPARAM) TEXT("较高"));
    SendMessage(hEccLevel, CB_ADDSTRING, 0, (LPARAM) TEXT("高"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("自动"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("0"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("1"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("2"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("3"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("4"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("5"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("6"));
    SendMessage(hMask, CB_ADDSTRING, 0, (LPARAM) TEXT("7"));
}

void RenewDialogItem(ResultContext* ctx, HWND hWnd) {
    SendMessage(GetDlgItem(hWnd, IDC_ECCLEVEL), CB_SETCURSEL, ctx->eccLevel, 0);
    SendMessage(GetDlgItem(hWnd, IDC_MASK), CB_SETCURSEL, ctx->maskIndex + 1,
                0);
    CheckDlgButton(hWnd, IDC_BINARY,
                   ctx->binaryMode ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_BOOSTECC,
                   ctx->boostEccLevel ? BST_CHECKED : BST_UNCHECKED);
}

void RenewEditContent(ResultContext* ctx, HWND hWnd) {
    LPWSTR text;

    if (ctx->binaryMode == TRUE) {
        text = ConvertToBinary(ctx->data, ctx->len);
    } else {
        text = ConvertToUnicode(CP_UTF8, ctx->data, ctx->len, NULL);
    }
    if (text != NULL) {
        SendMessageW(GetDlgItem(hWnd, IDC_RESULTEDIT), WM_SETTEXT, 0,
                     (LPARAM) text);
        free(text);
    }
}

void RegenerateQrCode(ResultContext* ctx, HWND hWnd) {
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX];
    bool result;
    int size, x, y, cur;
    int width, bound;
    uint8_t *p, v;
    uint8_t bits[24 * 183], *pos;
    struct {
        BITMAPINFOHEADER bh;
        RGBQUAD color[2];
    } bi;

    if (ctx->binaryMode == TRUE) {
        memcpy(temp, ctx->data, ctx->len);
        result = qrcodegen_encodeBinary(temp, ctx->len, ctx->qrCode,
                                        ctx->eccLevel, 1, 40, ctx->maskIndex,
                                        (bool) ctx->boostEccLevel);
    } else {
        result =
            qrcodegen_encodeText(ctx->data, temp, ctx->qrCode, ctx->eccLevel, 1,
                                 40, ctx->maskIndex, (bool) ctx->boostEccLevel);
    }
    ctx->failed = !result;
    if (result == FALSE) {
        InvalidateRect(hWnd, &ctx->imageRect, TRUE);
        ShowWindow(GetDlgItem(hWnd, IDC_FAILED), SW_SHOW);
        return;
    }
    ShowWindow(GetDlgItem(hWnd, IDC_FAILED), SW_HIDE);
    size = qrcodegen_getSize(ctx->qrCode);
    p = ctx->qrCode + 1;
    x = y = cur = 0;
    width = ((size + 6 + 31) / 32) * 4;
    bound = size + 3;
    memset(bits, 0, width * (size + 6));
    for (y = bound - 1; y >= 3; y--) {
        pos = bits + y * width;
        for (x = 3; x < bound; x++) {
            if (cur == 0) {
                v = *p++;
                cur = 8;
            }
            pos[x / 8] |= ((0x80 >> (x & 7)) * (v & 1));
            v >>= 1;
            cur--;
        }
    }
    size += 6;
    memset(&bi, 0, sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD));
    bi.bh.biSize = sizeof(BITMAPINFOHEADER);
    bi.bh.biWidth = size;
    bi.bh.biHeight = size;
    bi.bh.biPlanes = 1;
    bi.bh.biBitCount = 1;
    bi.bh.biSizeImage = width * size;
    bi.color[0].rgbRed = bi.color[0].rgbGreen = bi.color[0].rgbBlue = 255;
    bi.color[1].rgbRed = bi.color[1].rgbGreen = bi.color[1].rgbBlue = 1;
    SetDIBits(ctx->memoryDC, ctx->bitmap, 0, size, (void*) bits,
              (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    InvalidateRect(hWnd, &ctx->imageRect, TRUE);
}

LPWSTR ConvertToUnicode(UINT codePage, char* utf8, int len, int* wideLen) {
    LPWSTR result;
    int widecharLen;

    widecharLen = MultiByteToWideChar(codePage, 0, utf8, len, NULL, 0);
    result = malloc((widecharLen + 1) * sizeof(WCHAR));
    if (result == NULL) {
        return NULL;
    }
    MultiByteToWideChar(codePage, 0, utf8, len, result, widecharLen);
    result[widecharLen] = L'\0';
    if (wideLen != NULL) {
        *wideLen = widecharLen;
    }
    return result;
}

void SaveImageToFile(ResultContext* ctx) {
    OPENFILENAMEA ofna;
    TCHAR fileName[256];
    uint8_t *data, *p;
    uint8_t *pos, v;
    int size, x, y, cur;
    int width, bound, i;

    fileName[0] = 0;
    memset(&ofna, 0, sizeof(OPENFILENAMEA));
    ofna.lStructSize = sizeof(OPENFILENAMEA);
    ofna.hwndOwner = window;
    ofna.lpstrFilter = "PNG (*.png)\0*.png\0\0";
    ofna.nFilterIndex = 1;
    ofna.lpstrFile = fileName;
    ofna.nMaxFile = 256;
    ofna.lpstrTitle = "保存图片文件";
    ofna.lpstrDefExt = "png";
    if (GetSaveFileNameA(&ofna) == 0) {
        return;
    }
    size = qrcodegen_getSize(ctx->qrCode);
    p = ctx->qrCode + 1;
    x = y = cur = 0;
    width = size + 6;
    bound = size + 3;
    data = malloc(width * width * 64);
    if (data == NULL) {
        return;
    }
    memset(data, 0xff, width * width * 64);
    for (y = 3; y < bound; y++) {
        pos = data + y * width * 64 + 24;
        for (x = 3; x < bound; x++) {
            if (cur == 0) {
                v = *p++;
                cur = 8;
            }
            for (i = 0; i < 8; i++) {
                *pos++ = 0xff * ((~v) & 1);
            }
            v >>= 1;
            cur--;
        }
        for (i = 1; i < 8; i++) {
            memcpy(data + (y * 8 + i) * width * 8, data + y * 8 * width * 8,
                   width * 8);
        }
    }
    stbi_write_png(fileName, width * 8, width * 8, 1, data, 0);
    free(data);
}

LPWSTR ConvertToBinary(char* data, int len) {
    WCHAR *result, *p;
    char ch, c;
    int i;

    result = malloc((len * 2 + 1) * sizeof(WCHAR));
    if (result == NULL) {
        return NULL;
    }
    p = result;
    for (i = 0; i < len; i++) {
        ch = data[i];
        c = (ch >> 4) & 0xF;
        if (c < 10) {
            *p++ = c + L'0';
        } else {
            *p++ = c - 10 + L'A';
        }
        c = ch & 0xF;
        if (c < 10) {
            *p++ = c + L'0';
        } else {
            *p++ = c - 10 + L'A';
        }
    }
    *p = L'\0';
    return result;
}

int DecodeBinaryData(LPWSTR str, int len, unsigned char* out, int maxLen) {
    int decoded;
    WCHAR ch;
    LPWSTR p;
    int x;

    maxLen = min(maxLen, (len >> 1));
    decoded = 0;
    p = str;
    while (decoded < maxLen) {
        ch = *p;
        if (ch >= L'0' && ch <= L'9') {
            x = (ch - L'0');
        } else if (ch >= L'a' && ch <= L'f') {
            x = (ch - L'a' + 10);
        } else if (ch >= L'A' && ch <= L'F') {
            x = (ch - L'A' + 10);
        } else {
            break;
        }
        x = x << 4;
        ch = *(p + 1);
        if (ch >= L'0' && ch <= L'9') {
            x += (ch - L'0');
        } else if (ch >= L'a' && ch <= L'f') {
            x += (ch - L'a' + 10);
        } else if (ch >= L'A' && ch <= L'F') {
            x += (ch - L'A' + 10);
        } else {
            break;
        }
        *out++ = (unsigned char) x;
        p += 2;
        decoded++;
    }
    return decoded;
}
