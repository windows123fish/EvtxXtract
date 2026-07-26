#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace Evtx {

// According to MS-EVTX documentation and real .evtx file analysis:
// Section 2.1: File Header Structure (4096 bytes)
// Section 2.2: Chunk Header Structure (512 bytes)

// Ensure no padding between struct members
#pragma pack(push, 1)

/**
 * @brief EVT_FILE_HEADER structure
 * 
 * Represents the 4KB file header at the beginning of every .evtx file.
 * The magic number "ElfFile\x00" identifies valid EVTX files.
 * 
 * Layout verified against real .evtx files (hex dump analysis):
 * 
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfFile\x00")
 * 0x0008 | 8    | file_size
 * 0x0010 | 8    | oldest_chunk_offset
 * 0x0018 | 8    | newest_chunk_offset
 * 0x0020 | 4    | version (major << 16 | minor)
 * 0x0024 | 2    | flags
 * 0x0026 | 2    | chunk_count
