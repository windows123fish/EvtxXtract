#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "evtx_structs.h"

// Forward declarations
struct EVT_FILE_HEADER;
struct EVT_CHUNK_HEADER;

struct ChunkInfo {
    uint64_t offset;
    uint32_t event_count;
    uint32_t checksum;
};

class GuiWindow {
public:
    GuiWindow();
    ~GuiWindow();
    
    bool init(HINSTANCE hInstance, const std::string& title, int width, int height);
    void shutdown();
    void run();
    
private:
    void render();
    void renderFileSelection();
    void renderFileInfo();
    void renderFooter();
    
    void scanEvtxFiles();
    void parseSelectedFile();
    
    LRESULT wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
private:
    HWND m_hWnd;
    HDC m_hDC;
    bool m_isRunning;
    
    // File selection
    std::vector<std::string> m_evtxFiles;
    int m_selectedFileIndex;
    
    // Parsing state
    bool m_isParsing;
    EVT_FILE_HEADER m_fileHeader;
    std::vector<ChunkInfo> m_chunkInfo;
    uint32_t m_validChunkCount;
};
