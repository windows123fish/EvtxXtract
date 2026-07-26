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
IMGUI_IMPL_API void ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data) {
    if (!g_hDC || !draw_data)
        return;

    // Create/resize buffer
    int width = (int)draw_data->DisplaySize.x;
    int height = (int)draw_data->DisplaySize.y;
    if (!ImGui_ImplGDI_CreateBuffer(width, height))
        return;

    // Clear buffer
    memset(g_Buffer, 0, width * height * 4);

    // Get the font texture
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* font_pixels = NULL;
    int font_width = 0, font_height = 0, font_bytes_per_pixel = 0;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height, &font_bytes_per_pixel);

    // Create font bitmap if not exists
    static HBITMAP hFontBitmap = NULL;
    static HDC hFontDC = NULL;
    static unsigned char* font_buffer = NULL;
    if (!hFontBitmap) {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = font_width;
        bmi.bmiHeader.biHeight = -font_height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hFontDC = CreateCompatibleDC(g_hDC);
        font_buffer = new unsigned char[font_width * font_height * 4];
        hFontBitmap = CreateDIBSection(hFontDC, &bmi, DIB_RGB_COLORS, (void**)&font_buffer, NULL, 0);
        SelectObject(hFontDC, hFontBitmap);
        memcpy(font_buffer, font_pixels, font_width * font_height * 4);
    }

    // Render commands
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
                continue;
            }

            // Setup clipping rectangle
            RECT clip_rect;
            clip_rect.left = (LONG)pcmd->ClipRect.x;
            clip_rect.top = (LONG)pcmd->ClipRect.y;
            clip_rect.right = (LONG)pcmd->ClipRect.z;
            clip_rect.bottom = (LONG)pcmd->ClipRect.w;
            IntersectClipRect(g_hDC, clip_rect.left, clip_rect.top, clip_rect.right, clip_rect.bottom);

            // Draw indexed triangles
            for (int i = 0; i < pcmd->ElemCount; i += 3) {
                const ImDrawIdx idx0 = idx_buffer[pcmd->IdxOffset + i];
                const ImDrawIdx idx1 = idx_buffer[pcmd->IdxOffset + i + 1];
                const ImDrawIdx idx2 = idx_buffer[pcmd->IdxOffset + i + 2];

                const ImVec2& v0 = cmd_list->VtxBuffer[idx0].pos;
                const ImVec2& v1 = cmd_list->VtxBuffer[idx1].pos;
                const ImVec2& v2 = cmd_list->VtxBuffer[idx2].pos;

                const ImVec4& c0 = cmd_list->VtxBuffer[idx0].col;
                const ImVec4& c1 = cmd_list->VtxBuffer[idx1].col;
                const ImVec4& c2 = cmd_list->VtxBuffer[idx2].col;

                // Simple triangle rasterization
                // This is a simplified implementation for demonstration
                float min_x = std::min(std::min(v0.x, v1.x), v2.x);