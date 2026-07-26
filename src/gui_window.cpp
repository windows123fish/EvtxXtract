#include "gui_window.h"
#include "evtx_parser.h"
#include <imgui.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

// Forward declarations
extern IMGUI_IMPL_API bool ImGui_ImplWin32_Init(HWND hWnd);
extern IMGUI_IMPL_API void ImGui_ImplWin32_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplWin32_NewFrame();
extern IMGUI_IMPL_API bool ImGui_ImplGDI_Init(HDC hDC);
extern IMGUI_IMPL_API void ImGui_ImplGDI_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data);
extern IMGUI_IMPL_API void ImGui_ImplGDI_NewFrame();

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
    
    // Initialize backends
    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplGDI_Init(m_hDC);
    
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
    ImGui_ImplGDI_Shutdown();
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
    ImGui_ImplGDI_NewFrame();
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
    ImGui_ImplGDI_RenderDrawData(ImGui::GetDrawData());
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
            if (ImGui::Selectable(m_evtxFiles[i].c_str(), isSelected)) {
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
    
    // Force immediate render to show loading state
    render();
    
    const std::string& filepath = m_evtxFiles[m_selectedFileIndex];
    
    try {
        Evtx::EvtxParser parser(filepath);
        if (!parser.open()) {
            MessageBoxA(m_hWnd, ("无法打开文件: " + filepath).c_str(), "错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        // Read file header
        if (!parser.read_file_header()) {
            MessageBoxA(m_hWnd, "读取文件头失败", "错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        // Get file header
        m_fileHeader = parser.get_file_header();
        
        // Validate chunks
        m_validChunkCount = parser.validate_chunks();
        
        // Get chunk info
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
        
        // Update file size with actual size
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
        return window->wndProc(hWnd, msg, wParam