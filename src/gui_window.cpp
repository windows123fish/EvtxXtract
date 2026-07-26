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
                            buffer[idx] = static_cast<unsigned char>(buffer[idx] * (1 - alpha) + r * alpha);
                            buffer[idx + 1] = static_cast<unsigned char>(buffer[idx + 1] * (1 - alpha) + g * alpha);
                            buffer[idx + 2] = static_cast<unsigned char>(buffer[idx + 2] * (1 - alpha) + b * alpha);
                        }
                    }
                }
            }
        }
    }
    
    // Draw to screen
    HBITMAP oldBitmap = (HBITMAP)SelectObject(m_hDC, hBitmap);
    BitBlt(m_hDC, 0, 0, width, height, NULL, 0, 0, SRCCOPY);
    SelectObject(m_hDC, oldBitmap);
}

void GuiWindow::renderFileSelection() {
    ImGui::BeginChild("文件选择", ImVec2(300, ImGui::GetWindowHeight() - 100), true);
    
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "📁 EVTX文件列表");
    ImGui::Separator();
    
    if (m_evtxFiles.empty()) {
        ImGui::Text("未找到EVTX文件");
        if (ImGui::Button("重新扫描")) {
            scanEvtxFiles();
        }
    } else {
        for (size_t i = 0; i < m_evtxFiles.size(); i++) {
            bool isSelected = (m_selectedFileIndex == (int)i);
            ImGui::PushID((int)i);
            std::string filename = fs::path(m_evtxFiles[i]).filename().string();
            if (ImGui::Selectable(filename.c_str(), isSelected)) {
                m_selectedFileIndex = (int)i;
                m_isParsing = false;
                m_fileHeader = Evtx::EVT_FILE_HEADER();
                m_chunkInfo.clear();
                m_validChunkCount = 0;
            }
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        if (m_selectedFileIndex >= 0 && ImGui::Button("解析文件")) {
            parseSelectedFile();
        }
    }
    
    ImGui::EndChild();
}

void GuiWindow::renderFileInfo() {
    ImGui::SameLine();
    ImGui::BeginChild("文件信息", ImVec2(0, ImGui::GetWindowHeight() - 100), true);
    
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "📋 文件信息");
    ImGui::Separator();
    
    if (!m_isParsing && m_fileHeader.magic[0] == 0) {
        ImGui::Text("请选择一个文件并点击\"解析文件\"");
    } else if (m_isParsing) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "正在解析文件...");
        ImGui::ProgressBar(0.5f, ImVec2(-1, 20), "处理中");
    } else {
        // Show file header info
        ImGui::Text("魔术数: %s", m_fileHeader.validate_magic() ? "有效 (ElfFile)" : "无效");
        
        uint16_t major = m_fileHeader.get_major_version();
        uint16_t minor = m_fileHeader.get_minor_version();
        ImGui::Text("版本: %u.%u", major, minor);
        
        ImGui::Text("标志位: 0x%04X", m_fileHeader.flags);
        ImGui::Text("块数量: %u", m_fileHeader.chunk_count);
        ImGui::Text("有效块数: %u", m_validChunkCount);
        
        if (m_fileHeader.chunk_count != m_validChunkCount) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "警告: 文件头块数与实际不符");
        }
        
        ImGui::Text("文件大小: %llu bytes", m_fileHeader.file_size);
        ImGui::Text("最旧块偏移: 0x%llX", m_fileHeader.oldest_chunk_offset);
        ImGui::Text("最新块偏移: 0x%llX", m_fileHeader.newest_chunk_offset);
        ImGui::Text("校验和: 0x%08X", m_fileHeader.checksum);
        
        // Show chunk info
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "📦 块信息");
        
        if (m_chunkInfo.empty()) {
            ImGui::Text("未解析块信息");
        } else {
            ImGui::BeginTable("chunks", 3, ImGuiTableFlags_Borders);
            ImGui::TableSetupColumn("块偏移");
            ImGui::TableSetupColumn("事件数");
            ImGui::TableSetupColumn("校验和");
            ImGui::TableHeadersRow();
            
            for (const auto& info : m_chunkInfo) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%llX", info.offset);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", info.event_count);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0x%08X", info.checksum);
            }
            ImGui::EndTable();
        }
    }
    
    ImGui::EndChild();
}

void GuiWindow::renderFooter() {
    ImGui::Separator();
    ImGui::Text("EvtxXtract v1.0.0 | 高性能EVTX文件流式解析器");
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    ImGui::Text("按 ESC 退出");
}

void GuiWindow::scanEvtxFiles() {
    m_evtxFiles.clear();
    
    // Scan default Windows log directory
    const std::string default_dir = "C:\\Windows\\System32\\winevt\\Logs\\";
    if (fs::exists(default_dir) && fs::is_directory(default_dir)) {
        for (const auto& entry : fs::directory_iterator(default_dir)) {
            if (entry.path().extension() == ".evtx") {
                m_evtxFiles.push_back(entry.path().string());
            }
        }
    }
    
    // If no files found, try to export using wevtutil
    if (m_evtxFiles.empty()) {
        // We'll add this feature later
    }
}

void GuiWindow::parseSelectedFile() {
    if (m_selectedFileIndex < 0 || m_selectedFileIndex >= (int)m_evtxFiles.size()) {
        return;
    }
    
    m_isParsing = true;
    render();
    
    const std::string& filepath = m_evtxFiles[m_selectedFileIndex];
    
    try {
        Evtx::EvtxParser parser(filepath);
        if (!parser.open()) {
            MessageBoxA(m_hWnd, ("无法打开文件: " + filepath).c_str(), "错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        if (!parser.read_file_header()) {
            MessageBoxA(m_hWnd, "读取文件头失败", "错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        m_fileHeader = parser.get_file_header();
        m_validChunkCount = parser.validate_chunks();
        
        const auto& chunks = parser.get_valid_chunks();
        m_chunkInfo.clear();
        
        uint64_t current_offset = Evtx::EVTX_FILE_HEADER_SIZE;
        const uint64_t chunk_size = Evtx::EVTX_CHUNK_SIZE;
        
        for (const auto& chunk_header : chunks) {
            ChunkInfo info;
            info.offset = current_offset;
            info.event_count = static_cast<uint32_t>(chunk_header.last_event_record_number - chunk_header.first_event_record_number + 1);
            info.checksum = chunk_header.chunk_checksum;
            m_chunkInfo.push_back(info);
            current_offset += chunk_size;
        }
        
        m_fileHeader.file_size = fs::file_size(filepath);
        
    } catch (const std::exception& e) {
        MessageBoxA(m_hWnd, ("解析错误: " + std::string(e.what())).c_str(), "错误", MB_OK | MB_ICONERROR);
    }
    
    m_isParsing = false;
}

LRESULT CALLBACK GuiWindow::s_wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* window = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<GuiWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<GuiWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    
    if (window) {
        return window->wndProc(hWnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT GuiWindow::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return 0;
    }
    
    switch (msg) {
        case WM_CLOSE:
            m_isRunning = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                m_isRunning = false;
                PostQuitMessage(0);
            }
            return 0;
            
        case WM_DESTROY:
            m_isRunning = false;
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
