#include <imgui.h>
#include <windows.h>

static HDC              g_hDC = NULL;
static HBITMAP          g_hBitmap = NULL;
static HBITMAP          g_hBitmapOld = NULL;
static int              g_Width = 0;
static int              g_Height = 0;
static unsigned char*   g_Buffer = NULL;

// Initialize GDI rendering
IMGUI_IMPL_API bool ImGui_ImplGDI_Init(HDC hDC) {
    g_hDC = hDC;
    g_hBitmap = NULL;
    g_hBitmapOld = NULL;
    g_Width = 0;
    g_Height = 0;
    g_Buffer = NULL;
    return true;
}

// Shutdown GDI rendering
IMGUI_IMPL_API void ImGui_ImplGDI_Shutdown() {
    if (g_hBitmap) {
        if (g_hDC && g_hBitmapOld)
            SelectObject(g_hDC, g_hBitmapOld);
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
        g_hBitmapOld = NULL;
    }
    if (g_Buffer) {
        delete[] g_Buffer;
        g_Buffer = NULL;
    }
    g_hDC = NULL;
}

// Create/resize offscreen buffer
static bool ImGui_ImplGDI_CreateBuffer(int width, int height) {
    if (g_Width == width && g_Height == height)
        return true;

    // Cleanup old buffer
    if (g_hBitmap) {
        if (g_hDC && g_hBitmapOld)
            SelectObject(g_hDC, g_hBitmapOld);
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
        g_hBitmapOld = NULL;
    }
    if (g_Buffer) {
        delete[] g_Buffer;
        g_Buffer = NULL;
    }

    g_Width = width;
    g_Height = height;

    // Create new buffer
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = width * height * 4;

    g_Buffer = new unsigned char[width * height * 4];
    g_hBitmap = CreateDIBSection(g_hDC, &bmi, DIB_RGB_COLORS, (void**)&g_Buffer, NULL, 0);
    if (!g_hBitmap) {
        delete[] g_Buffer;
        g_Buffer = NULL;
        return false;
    }

    return true;
}

// Render ImGui draw data to GDI
