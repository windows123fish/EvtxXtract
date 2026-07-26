#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace Evtx {

#pragma pack(push, 1)

/**
 * @brief EVT_FILE_HEADER structure
 * 
 * MS-EVTX Section 2.1: File Header Structure (4096 bytes)
 * 
 * Verified against real .evtx file hex dump:
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfFile\x0