#define INITGUID
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <stdio.h>
#include <stdlib.h>
#include "filters.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define WINDOW_CLASS_NAME "MFWindowClass"

// Globals for drawing
static HDC memDC = NULL;
static HBITMAP bmp = NULL;
static HGDIOBJ oldBmp = NULL;
static BITMAPINFO bmi;

void InitMediaFoundation();
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
BOOL InitWebcam(IMFSourceReader **pReader);
void ConfigureMediaType(IMFSourceReader *pReader);
void RenderLoop(HWND hwnd, IMFSourceReader *pReader);
void initFrameWithGrayBGRA(unsigned char *buffer, int width, int height);
void ProcessFrame(HWND hwnd, IMFSourceReader *pReader);
void InitDrawing(HWND hwnd);
void DrawFrames(HWND hwnd, BYTE *inputFrame, BYTE *outputFrame0, BYTE *outputFrame1, BYTE *outputFrame2);
void Cleanup(HWND hwnd, IMFSourceReader *pReader);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Converts between BGRA and RGB to simplify filter development by hiding alpha channel handling
void convertBGRAtoRGB(unsigned char* bgra, unsigned char* rgb, int width, int height);
void convertRGBtoBGRA(unsigned char* rgb, unsigned char* bgra, int width, int height);
void convertYUY2toRGB(unsigned char* yuy2, unsigned char* rgb, int width, int height);

void applyFilter(unsigned char * in, unsigned char * out, int width, int height) 
{
    negativeFilter(in, out, FRAME_WIDTH, FRAME_HEIGHT);
    grayscalefilter(in, out, FRAME_WIDTH, FRAME_HEIGHT);
    duplicatefilter(in, out, FRAME_WIDTH, FRAME_HEIGHT);
    coolFilter (in, out, FRAME_WIDTH, FRAME_HEIGHT);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InitMediaFoundation();

    HWND hwnd = CreateMainWindow(hInstance, nCmdShow);
    if (!hwnd) return -1;

    InitDrawing(hwnd);

    IMFSourceReader *pReader = NULL;
    if (!InitWebcam(&pReader)) return -1;

    ConfigureMediaType(pReader);
    RenderLoop(hwnd, pReader);
    Cleanup(hwnd, pReader);
    return 0;
}

void InitMediaFoundation() {
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL))) {
        MessageBox(NULL, "Media Foundation failed to start.", "Error", MB_OK);
        exit(-1);
    }
}

HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow) 
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, "Filtermania!",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        DISPLAY_WIDTH * 2 + 16, DISPLAY_HEIGHT + 39,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "Failed to create window.", "Error", MB_OK);
        MFShutdown();
        return NULL;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return hwnd;
}

BOOL InitWebcam(IMFSourceReader **pReader) 
{
    IMFAttributes *pAttr = NULL;
    IMFActivate **ppDevices = NULL;
    IMFMediaSource *pSource = NULL;

    MFCreateAttributes(&pAttr, 1);
    pAttr->lpVtbl->SetGUID(pAttr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    UINT32 count = 0;
    MFEnumDeviceSources(pAttr, &ppDevices, &count);
    if (count == 0) {
        MessageBox(NULL, "No video capture devices found.", "Error", MB_OK);
        return FALSE;
    }
    
    // Activate webcam
    HRESULT hr = ppDevices[0]->lpVtbl->ActivateObject(ppDevices[0], &IID_IMFMediaSource, (void**)&pSource);
    if (FAILED(hr)) {
        MessageBox(NULL, "Failed to activate webcam.", "Error", MB_OK);
        return FALSE;
    }

    // Create SourceReader with video processing enabled
    IMFAttributes* pReaderAttributes = NULL;
    MFCreateAttributes(&pReaderAttributes, 1);
    pReaderAttributes->lpVtbl->SetUINT32(pReaderAttributes, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    MFCreateSourceReaderFromMediaSource(pSource, pReaderAttributes, pReader);

    pReaderAttributes->lpVtbl->Release(pReaderAttributes);
    pAttr->lpVtbl->Release(pAttr);
    for (UINT32 i = 0; i < count; i++) ppDevices[i]->lpVtbl->Release(ppDevices[i]);
    CoTaskMemFree(ppDevices);
    pSource->lpVtbl->Release(pSource);

    return TRUE;
}

void ConfigureMediaType(IMFSourceReader *pReader) {
    IMFMediaType *pTypeOut = NULL;
    HRESULT hr = MFCreateMediaType(&pTypeOut);
    if (SUCCEEDED(hr)) {
        pTypeOut->lpVtbl->SetGUID(pTypeOut, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        
        // Try YUY2 first (most commonly supported by cameras)
        pTypeOut->lpVtbl->SetGUID(pTypeOut, &MF_MT_SUBTYPE, &MFVideoFormat_YUY2);
        UINT64 frameSize = ((UINT64)FRAME_WIDTH << 32) | FRAME_HEIGHT;
        pTypeOut->lpVtbl->SetUINT64(pTypeOut, &MF_MT_FRAME_SIZE, frameSize);
        
        hr = pReader->lpVtbl->SetCurrentMediaType(pReader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pTypeOut);
        if (FAILED(hr)) {
            // If YUY2 fails, try RGB24
            pTypeOut->lpVtbl->SetGUID(pTypeOut, &MF_MT_SUBTYPE, &MFVideoFormat_RGB24);
            hr = pReader->lpVtbl->SetCurrentMediaType(pReader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pTypeOut);
            if (FAILED(hr)) {
                // Final fallback to RGB32
                pTypeOut->lpVtbl->SetGUID(pTypeOut, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
                pReader->lpVtbl->SetCurrentMediaType(pReader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pTypeOut);
            }
        }
    }
    if (pTypeOut) pTypeOut->lpVtbl->Release(pTypeOut);
}

void RenderLoop(HWND hwnd, IMFSourceReader *pReader) {
    if (!pReader) return;
    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        ProcessFrame(hwnd, pReader);
        Sleep(10);
    }
}

void InitDrawing(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    memDC = CreateCompatibleDC(hdc);
    bmp = CreateCompatibleBitmap(hdc, DISPLAY_WIDTH * 2, DISPLAY_HEIGHT);
    oldBmp = SelectObject(memDC, bmp);
    ReleaseDC(hwnd, hdc);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = FRAME_WIDTH;
    bmi.bmiHeader.biHeight = -FRAME_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}


void initFrameWithGrayBGRA(unsigned char *buffer, int width, int height) {
    const unsigned char grayPixel[4] = {128, 128, 128, 255}; // B, G, R, A
    for (int i = 0; i < width * height; i++) {
        memcpy(&buffer[i * 4], grayPixel, 4);
    }
}


void ProcessFrame(HWND hwnd, IMFSourceReader *pReader) {
    static unsigned char rgbInput[FRAME_WIDTH * FRAME_HEIGHT * 3];
    static unsigned char rgbOutput0[FRAME_WIDTH * FRAME_HEIGHT * 3];
    
    static unsigned char inputFrame[FRAME_WIDTH * FRAME_HEIGHT * 4];
    static unsigned char outputFrame0[FRAME_WIDTH * FRAME_HEIGHT * 4];
    static unsigned char outputFrame1[FRAME_WIDTH * FRAME_HEIGHT * 4];
    static unsigned char outputFrame2[FRAME_WIDTH * FRAME_HEIGHT * 4];
    static unsigned char rawBuffer[FRAME_WIDTH * FRAME_HEIGHT * 4]; // Raw camera data

    static int initialized = 0;
    if (!initialized) {
        initFrameWithGrayBGRA(inputFrame, FRAME_WIDTH, FRAME_HEIGHT);
        initFrameWithGrayBGRA(outputFrame0, FRAME_WIDTH, FRAME_HEIGHT);
        initFrameWithGrayBGRA(outputFrame1, FRAME_WIDTH, FRAME_HEIGHT);
        initFrameWithGrayBGRA(outputFrame2, FRAME_WIDTH, FRAME_HEIGHT);
        initialized = 1;
    }

    IMFSample *sample = NULL;
    DWORD flags;
    LONGLONG ts;
    if (FAILED(pReader->lpVtbl->ReadSample(pReader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &flags, &ts, &sample)) || !sample) return;

    IMFMediaBuffer *buffer = NULL;
    sample->lpVtbl->ConvertToContiguousBuffer(sample, &buffer);
    BYTE *data = NULL;
    DWORD maxLen, curLen;
    buffer->lpVtbl->Lock(buffer, &data, &maxLen, &curLen);
    
    // Copy raw data
    memcpy(rawBuffer, data, curLen < sizeof(rawBuffer) ? curLen : sizeof(rawBuffer));
    
    buffer->lpVtbl->Unlock(buffer);
    buffer->lpVtbl->Release(buffer);
    sample->lpVtbl->Release(sample);

    // Determine format based on data size and convert to RGB
    if (curLen == FRAME_WIDTH * FRAME_HEIGHT * 2) {
        // YUY2 format (2 bytes per pixel)
        convertYUY2toRGB(rawBuffer, rgbInput, FRAME_WIDTH, FRAME_HEIGHT);
    } else if (curLen == FRAME_WIDTH * FRAME_HEIGHT * 3) {
        // RGB24 format (3 bytes per pixel) - direct copy
        memcpy(rgbInput, rawBuffer, FRAME_WIDTH * FRAME_HEIGHT * 3);
    } else if (curLen >= FRAME_WIDTH * FRAME_HEIGHT * 4) {
        // RGB32/ARGB32 format (4 bytes per pixel) - convert from BGRA
        convertBGRAtoRGB(rawBuffer, rgbInput, FRAME_WIDTH, FRAME_HEIGHT);
    } else {
        // Unknown format - fill with test pattern
        for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT * 3; i += 3) {
            rgbInput[i] = 255;     // R
            rgbInput[i + 1] = 0;   // G  
            rgbInput[i + 2] = 0;   // B
        }
    }

    // Convert RGB to BGRA for display
    convertRGBtoBGRA(rgbInput, inputFrame, FRAME_WIDTH, FRAME_HEIGHT);

    // Apply filter
    applyFilter(rgbInput, rgbOutput0, FRAME_WIDTH, FRAME_HEIGHT);
    convertRGBtoBGRA(rgbOutput0, outputFrame0, FRAME_WIDTH, FRAME_HEIGHT);

    DrawFrames(hwnd, inputFrame, outputFrame0, outputFrame1, outputFrame2);
}


void DrawFrames(HWND hwnd, BYTE *inputFrame, BYTE *outputFrame0, BYTE *outputFrame1, BYTE *outputFrame2) {
    HDC hdc = GetDC(hwnd);

    SetStretchBltMode(memDC, HALFTONE);

    // Left: original
    StretchDIBits(memDC,
        0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
        0, 0, FRAME_WIDTH, FRAME_HEIGHT,
        inputFrame, &bmi, DIB_RGB_COLORS, SRCCOPY);

    // Right: filtered output
    StretchDIBits(memDC,
        DISPLAY_WIDTH, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
        0, 0, FRAME_WIDTH, FRAME_HEIGHT,
        outputFrame0, &bmi, DIB_RGB_COLORS, SRCCOPY);

    BitBlt(hdc, 0, 0, DISPLAY_WIDTH * 2, DISPLAY_HEIGHT, memDC, 0, 0, SRCCOPY);

    ReleaseDC(hwnd, hdc);
}


void Cleanup(HWND hwnd, IMFSourceReader *pReader) {
    if (pReader) pReader->lpVtbl->Release(pReader);
    if (memDC && bmp) {
        SelectObject(memDC, oldBmp);
        DeleteObject(bmp);
        DeleteDC(memDC);
    }
    MFShutdown();
    CoUninitialize();
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam == 'S') {
                return 0;
            }
            break;

        case WM_LBUTTONDOWN:
            return 0;

        case WM_RBUTTONDOWN:
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void convertBGRAtoRGB(unsigned char* bgra, unsigned char* rgb, int width, int height) {
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        int inIdx = i * 4;
        int outIdx = i * 3;
        // Handle both BGRA and ARGB formats
        rgb[outIdx]     = bgra[inIdx + 2]; // R
        rgb[outIdx + 1] = bgra[inIdx + 1]; // G
        rgb[outIdx + 2] = bgra[inIdx];     // B
    }
}

void convertRGBtoBGRA(unsigned char* rgb, unsigned char* bgra, int width, int height) {
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        int inIdx = i * 3;
        int outIdx = i * 4;
        bgra[outIdx]     = rgb[inIdx + 2]; // B
        bgra[outIdx + 1] = rgb[inIdx + 1]; // G
        bgra[outIdx + 2] = rgb[inIdx];     // R
        bgra[outIdx + 3] = 255;            // A (opaque)
    }
}

void convertYUY2toRGB(unsigned char* yuy2, unsigned char* rgb, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2) {
            int yuy2Idx = (y * width + x) * 2;
            int rgb1Idx = (y * width + x) * 3;
            int rgb2Idx = (y * width + x + 1) * 3;
            
            unsigned char Y1 = yuy2[yuy2Idx];
            unsigned char U  = yuy2[yuy2Idx + 1];
            unsigned char Y2 = yuy2[yuy2Idx + 2];
            unsigned char V  = yuy2[yuy2Idx + 3];
            
            // Convert YUV to RGB for first pixel
            int C1 = Y1 - 16;
            int D = U - 128;
            int E = V - 128;
            
            int R1 = (298 * C1 + 409 * E + 128) >> 8;
            int G1 = (298 * C1 - 100 * D - 208 * E + 128) >> 8;
            int B1 = (298 * C1 + 516 * D + 128) >> 8;
            
            // Convert YUV to RGB for second pixel
            int C2 = Y2 - 16;
            int R2 = (298 * C2 + 409 * E + 128) >> 8;
            int G2 = (298 * C2 - 100 * D - 208 * E + 128) >> 8;
            int B2 = (298 * C2 + 516 * D + 128) >> 8;
            
            // Clamp values
            rgb[rgb1Idx]     = (R1 < 0) ? 0 : (R1 > 255) ? 255 : R1;
            rgb[rgb1Idx + 1] = (G1 < 0) ? 0 : (G1 > 255) ? 255 : G1;
            rgb[rgb1Idx + 2] = (B1 < 0) ? 0 : (B1 > 255) ? 255 : B1;
            
            if (x + 1 < width) {
                rgb[rgb2Idx]     = (R2 < 0) ? 0 : (R2 > 255) ? 255 : R2;
                rgb[rgb2Idx + 1] = (G2 < 0) ? 0 : (G2 > 255) ? 255 : G2;
                rgb[rgb2Idx + 2] = (B2 < 0) ? 0 : (B2 > 255) ? 255 : B2;
            }
        }
    }
}