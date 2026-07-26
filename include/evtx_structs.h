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
 * 0x0008 | 2    | version_major
 * 0x000A | 2    | version_minor
 * 0x000C | 2    | flags
 * 0x000E | 2    | chunk_count
 * 0x0010 | 8    | file_size
 * 0x0018 | 8    | oldest_chunk_offset
 * 0x0020 | 8    | newest_chunk_offset
 * 0x0028 | 4    | checksum
 * 0x002C | 4068 | reserved
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
   * @brief Major version (2 bytes, little-endian)
   * 
   * Currently always 1
   */
  uint16_t version_major;

  /**
   * @brief Minor version (2 bytes, little-endian)
   * 
   * Currently always 0
   */
  uint16_t version_minor;

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
  uint32_t checksum;

  /**
   * @brief Reserved/unused area (4068 bytes)
   * 
   * Remaining bytes of the 4KB header, typically filled with zeros
   */
  std::array<uint8_t, 4068> reserved;

  /**
   * @brief Validate the file header magic number
   * 
   * @return true if magic matches "ElfFile\x00", false otherwise
   */
  bool validate_magic() const noexcept;

  /**
   * @brief Get the major version number
   * 
   * @return Major version
   */
  uint16_t get_major_version() const noexcept;

  /**
   * @brief Get the minor version number
   * 
   * @return Minor version
   */
  uint16_t get_minor_version() const noexcept;

  /**
   * @brief Check if the dirty flag is set
   * 
   * @return true if file was not properly closed, false otherwise
   */
  bool is_dirty() const noexcept;

  /**
   * @brief Get a human-readable description of the header
   * 
   * @return String containing header information
   */
  std::string to_string() const;
};

/**
 * @brief EVT_CHUNK_HEADER structure
 * 
 * Represents the 512-byte header at the beginning of each 64KB chunk.
 * The magic number "ElfChnk\x00" identifies valid chunks.
 * 
 * Reference: MS-EVTX Section 2.2
 * 
 * Layout verified against real .evtx files:
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfChnk\x00")
 * 0x0008 | 8    | first_event_record_number
 * 0x0010 | 8    | last_event_record_number
 * 0x0018 | 8    | first_event_record_id
 * 0x0020 | 8    | last_event_record_id
 * 0x0028 | 4    | header_size (always 128 = 0x80)
 * 0x002C | 4    | last_event_offset
 * 0x0030 | 4    | free_space_offset
 * 0x0034 | 4    | events_checksum
 * 0x0038 | 4    | unknown1
 * 0x003C | 4    | flags
 * 0x0040 | 4    | chunk_checksum
 * 0x0044 | 4    | string_offset_array_offset
 * 0x0048 | 464  | reserved
 * -------|------|------
 * Total  | 512  |
 */
struct EVT_CHUNK_HEADER {
  /**
   * @brief Magic number: "ElfChnk\x00" (8 bytes)
   * 
   * Must be exactly {0x45, 0x6C, 0x66, 0x43, 0x68, 0x6E, 0x6B, 0x00}
   */
  std::array<uint8_t, 8> magic;

  /**
   * @brief First event record number in this chunk (8 bytes, little-endian)
   */
  uint64_t first_event_record_number;

  /**
   * @brief Last event record number in this chunk (8 bytes, little-endian)
   */
  uint64_t last_event_record_number;

  /**
   * @brief First event record identifier in this chunk (8 bytes, little-endian)
   */
  uint64_t first_event_record_id;

  /**
   * @brief Last event record identifier in this chunk (8 bytes, little-endian)
   */
  uint64_t last_event_record_id;

  /**
   * @brief Header size in bytes (4 bytes, little-endian)
   * 
   * Currently always 128 (0x80)
   */
  uint32_t header_size;

  /**
   * @brief Offset of the last event record in this chunk (4 bytes, little-endian)
   */
  uint32_t last_event_offset;

  /**
   * @brief Offset of free space in this chunk (4 bytes, little-endian)
   */
  uint32_t free_space_offset;

  /**
   * @brief CRC32 checksum of all event records (4 bytes, little-endian)
   */
  uint32_t events_checksum;

  /**
   * @brief Unknown field (4 bytes)
   */
  uint32_t unknown1;

  /**
   * @brief Chunk flags (4 bytes, little-endian)
   */
  uint32_t flags;

  /**
   * @brief CRC32 checksum of the chunk header (4 bytes, little-endian)
   * 
   * Computed over bytes 0x0008 through 0x01FF of the header
   */
  uint32_t chunk_checksum;

  /**
   * @brief Offset of the string offset array (4 bytes, little-endian)
   */
  uint32