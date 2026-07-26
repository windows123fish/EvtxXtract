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
 * Reference: MS-EVTX Section 2.1
 * 
 * Layout verified against real .evtx files (hex dump analysis):
 * 
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfFile\x00")
 * 0x0008 | 4    | version (major << 16 | minor)
 * 0x000C | 2    | flags
 * 0x000E | 2    | chunk_count
 * 0x0010 | 8    | file_size
 * 0x0018 | 8    | oldest_chunk_offset
 * 0x0020 | 8    | newest_chunk_offset
 * 0x0028 | 4    | checksum
 * 0x002C | 4052 | reserved
 * -------|------|------
 * Total  | 4096 |
 */
struct EVT_FILE_HEADER {
  /**
   * @brief Magic number: "ElfFile\x00" (8 bytes)
   * 
   * Must be exactly {0x45, 0x6C, 0x66, 0x46, 0x69, 0x6C, 0x65, 0x00}
   */
  std::array<uint8_t, 8> magic;

  /**
   * @brief File format version (4 bytes, little-endian)
   * 
   * High 16 bits: major version (currently 1)
   * Low 16 bits: minor version (currently 0)
   */
  uint32_t version;

  /**
   * @brief File flags (2 bytes, little-endian)
   * 
   * Bit 0: Dirty flag - set if the file was not properly closed
   */
  uint16_t flags;

  /**
   * @brief Number of chunks in the file (2 bytes, little-endian)
   */
  uint16_t chunk_count;

  /**
   * @brief File size in bytes (8 bytes, little-endian)
   */
  uint64_t file_size;

  /**
   * @brief Offset of the oldest chunk in the file (8 bytes, little-endian)
   * 
   * If no chunks exist, this is set to 0xFFFFFFFFFFFFFFFF
   */
  uint64_t oldest_chunk_offset;

  /**
   * @brief Offset of the newest chunk in the file (8 bytes, little-endian)
   * 
   * If no chunks exist, this is set to 0xFFFFFFFFFFFFFFFF
   */
  uint64_t newest_chunk_offset;

  /**
   * @brief CRC32 checksum of the file header (4 bytes, little-endian)
   * 
   * Computed over bytes 0x0008 through 0x0FFF of the header
   */
