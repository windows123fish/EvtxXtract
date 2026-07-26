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
 * 0x0028 | 4    | checksum
 * 0x002C | 4056 | reserved
 * -------|------|------
 * Total  | 4096 |
 */
struct EVT_FILE_HEADER {
  /**
   * @brief Magic number: "ElfFile\x00" (8 bytes)
   */
  std::array<uint8_t, 8> magic;

  /**
   * @brief File size in bytes (8 bytes, little-endian)
   */
  uint64_t file_size;

  /**
   * @brief Offset of the oldest chunk in the file (8 bytes, little-endian)
   */
  uint64_t oldest_chunk_offset;

  /**
   * @brief Offset of the newest chunk in the file (8 bytes, little-endian)
   */
  uint64_t newest_chunk_offset;

  /**
   * @brief File format version (4 bytes, little-endian)
   * 
   * High 16 bits: major version
   * Low 16 bits: minor version
   */
  uint32_t version;

  /**
   * @brief File flags (2 bytes, little-endian)
   */
  uint16_t flags;

  /**
   * @brief Number of chunks in the file (2 bytes, little-endian)
   */
  uint16_t chunk_count;

  /**
   * @brief CRC32 checksum of the file header (4 bytes, little-endian)
   */
  uint32_t checksum;

  /**
   * @brief Reserved/unused area (4056 bytes)
   */
  std::array<uint8_t, 4056> reserved;

  bool validate_magic() const noexcept;
  uint16_t get_major_version() const noexcept;
  uint16_t get_minor_version() const noexcept;
  bool is_dirty() const noexcept;
  std::string to_string() const;
};

/**
 * @brief EVT_CHUNK_HEADER structure
 * 
 * Represents the 512-byte header at the beginning of each 64KB chunk.
 * 
 * Layout verified against real .evtx files:
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfChnk\x00")
 * 0x0008 | 8    | first_event_record_number
 * 0x0010 | 8    | last_event_record_number
 * 0x0018 | 8    | first_event_record_id
 * 0x0020 | 8    | last_event_record_id
 * 0x0028 | 4    | header_size
 * 0x002C | 4    | last_event_offset
 * 0x0