#include "gui_window.h"
#include "evtx_parser.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

GuiWindow::GuiWindow() : 
    m_hWnd(nullptr), 
    m_hDC(nullptr), 
    m_isRunning(false),
    m_selectedFileIndex(-1),
    m_isParsing(false),
    m_validChunkCount(0)
{
}

GuiWindow::~GuiWindow() {
    shutdown();
}

bool GuiWindow::init(HINSTANCE hInstance, const std::string& title, int width, int height) {
    // Create window
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = s_wndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "EvtxXtract_GUI_Class";
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "窗口类注册失败", "错误", MB_OK | MB_ICONERROR);
        return false;
    }

    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    
    m_hWnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) {
        MessageBoxA(NULL, "窗口创建失败", "错误", MB_OK | MB_ICONERROR);
        return false;
    }

    m_hDC = GetDC(m_hWnd);
    
    // Initialize ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL; // Disable .ini file
    
    // Build font atlas
    unsigned char* pixels;
    int width_, height_, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width_, &height_, &bytes_per_pixel);
    
    // Initialize Win32 backend
    ImGui_ImplWin32_Init(m_hWnd);
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Scan for .evtx files
    scanEvtxFiles();
    
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
    m_isRunning = true;
    
    return true;
}

void GuiWindow::shutdown() {
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (m_hDC) {
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = nullptr;
    }
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void GuiWindow::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    while (msg.message != WM_QUIT && m_isRunning) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            render();
        }
    }
}

void GuiWindow::render() {
    // Start ImGui frame
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("EvtxXtract - EVTX文件解析器", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    // File selection panel
    renderFileSelection();
    
    // File info panel
    renderFileInfo();
    
    // Footer
    renderFooter();
    
    ImGui::End();
    
    // Render
    ImGui::Render();
    
    // Custom GDI rendering
    renderDrawData(ImGui::GetDrawData());
}

void GuiWindow::renderDrawData(ImDrawData* draw_data) {
    if (!m_hDC || !draw_data)
        return;

    // Get display size
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Create buffer
    static HBITMAP hBitmap = NULL;
    static unsigned char* buffer = NULL;
    static int buffer_width = 0, buffer_height = 0;
    
    if (buffer_width != width || buffer_height != height) {
        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
        if (buffer) {
            delete[] buffer;
            buffer = NULL;
        }
        
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        buffer = new unsigned char[width * height * 4];
        hBitmap = CreateDIBSection(m_hDC, &bmi, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);
        
        buffer_width = width;
        buffer_height = height;
    }
    
    // Clear buffer
    memset(buffer, 0, width * height * 4);
    
    // Get font texture
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* font_pixels = NULL;
    int font_width = 0, font_height = 0, font_bpp = 0;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height, &font_bpp);
    
    // Render commands
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            
            // Clip rectangle
            float clip_x0 = pcmd->ClipRect.x;
            float clip_y0 = pcmd->ClipRect.y;
            float clip_x1 = pcmd->ClipRect.z;
            float clip_y1 = pcmd->ClipRect.w;
            
            // Draw triangles
            for (int i = 0; i < pcmd->ElemCount; i += 3) {
                const ImDrawIdx idx0 = idx_buffer[pcmd->IdxOffset + i];
                const ImDrawIdx idx1 = idx_buffer[pcmd->IdxOffset + i + 1];
                const ImDrawIdx idx2 = idx_buffer[pcmd->IdxOffset + i + 2];
                
                const ImVec2& v0 = cmd_list->VtxBuffer[idx0].pos;
                const ImVec2& v1 = cmd_list->VtxBuffer[idx1].pos;
                const ImVec2& v2 = cmd_list->VtxBuffer[idx2].pos;
                
                // Simple rasterization
                int min_x = static_cast<int>(std::max(clip_x0, std::min(std::min(v0.x, v1.x), v2.x)));
                int min_y = static_cast<int>(std::max(clip_y0, std::min(std::min(v0.y, v1.y), v2.y)));
                int max_x = static_cast<int>(std::min(clip_x1, std::max(std::max(v0.x, v1.x), v2.x)));
                int max_y = static_cast<int>(std::min(clip_y1, std::max(std::max(v0.y, v1.y), v2.y)));
                
                for (int y = min_y; y < max_y; y++) {
                    for (int x = min_x; x < max_x; x++) {
                        // Barycentric interpolation
                        float w0 = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / 
                                   ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
                        float w1 = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / 
                                   ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
                        float w2 = 1.0f - w0 - w1;
                        
                        if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                            const ImVec4& c0 = cmd_list->VtxBuffer[idx0].col;
                            const ImVec4& c1 = cmd_list->VtxBuffer[idx1].col;
                            const ImVec4& c2 = cmd_list->VtxBuffer[idx2].col;
                            
                            float r = w0 * c0.x + w1 * c1.x + w2 * c2.x;
                            float g = w0 * c0.y + w1 * c1.y + w2 * c2.y;
                            float b = w0 * c0.z + w1 * c1.z + w2 * c2.z;
                            float a = w0 * c0.w + w1 * c1.w + w2 * c2.w;
                            
                            int idx = (y * width + x) * 4;
                            float alpha = a / 255.0f;
                            buffer[idx] =