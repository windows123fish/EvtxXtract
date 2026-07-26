#include <imgui.h>
#include <windows.h>

static HDC              g_hDC = NULL;
static HBITMAP          g_hBitmap = NULL;
static HBITMAP          g_hBitmapOld = NULL;
static int              g_Width = 0;
static