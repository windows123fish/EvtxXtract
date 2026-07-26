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
 * 0x0000 | 8    | magic ("ElfFile\x00")
 * 0x0008 | 4    | version
 * 0x000C | 2    | flags
 * 0x000E | 2    | chunk_count
 * 0x0010 | 8    | file_size
 * 0x0018 | 8    | oldest_chunk_offset
 * 0x0020 | 8    | newest_chunk_offset
 * 0x0028 | 4    | checksum
 * 0x002C | 4052 | reserved
 */
struct EVT_FILE_HEADER {
  std::array<uint8_t, 8> magic;
  uint32_t version;
  uint16_t flags;
  uint16_t chunk_count;
  uint64_t file_size;
  uint64_t oldest_chunk_offset;
  uint64_t newest_chunk_offset;
  uint32_t checksum;
  std::array<uint8_t, 4052> reserved;

  bool validate_magic() const noexcept;
  uint16_t get_major_version() const noexcept;
  uint16_t get_minor_version() const noexcept;
  bool is_dirty() const noexcept;
  std::string to_string() const;
};

/**
 * @brief EVT_CHUNK_HEADER structure
 * 
 * MS-EVTX Section 2.2: Chunk Header Structure (512 bytes)
 * 
 * Verified against real .evtx file hex dump:
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 8    | magic ("ElfChnk\x00")
 * 0x0008 | 8    | first_event_record_number
 * 0x0010 | 8    | last_event_record_number
 * 0x0018 | 8    | first_event_record_id
 * 0x0020 | 8    | last_event_record_id
 * 0x0028 | 4    | header_size
 * 0x002C | 4    | last_event_offset
 * 0x0030 | 4    | free_space_offset
 * 0x0034 | 4    | events_checksum
 * 0x0038 | 4    | unknown1
 * 0x003C | 4    | flags
 * 0x0040 | 4    | chunk_checksum
 * 0x0044 | 4    | string_offset_array_offset
 * 0x0048 | 440  | reserved
 */
struct EVT_CHUNK_HEADER {
  std::array<uint8_t, 8> magic;
  uint64_t first_event_record_number;
  uint64_t last_event_record_number;
  uint64_t first_event_record_id;
  uint64_t last_event_record_id;
  uint32_t header_size;
  uint32_t last_event_offset;
  uint32_t free_space_offset;
  uint32_t events_checksum;
  uint32_t unknown1;
  uint32_t flags;
  uint32_t chunk_checksum;
  uint32_t string_offset_array_offset;
  std::array<uint8_t, 440> reserved;

  bool validate_magic() const noexcept;
  std::string to_string() const;
};

#pragma pack(pop)

constexpr size_t EVTX_FILE_HEADER_SIZE = 4096;
constexpr size_t EVTX_CHUNK_SIZE = 65536;
constexpr size_t EVTX_CHUNK_HEADER_SIZE = 512;

constexpr std::array<uint8_t, 8> EVTX_FILE_MAGIC = {
    'E', 'l', 'f', 'F', 'i', 'l', 'e', 0x00
};
constexpr std::array<uint8_t, 8> EVTX_CHUNK_MAGIC = {
    'E', 'l', 'f', 'C', 'h', 'n', 'k', 0x00
};

static_assert(sizeof(EVT_FILE_HEADER) == EVTX_FILE_HEADER_SIZE,
              "EVT_FILE_HEADER must be exactly 4096 bytes");
static_assert(sizeof(EVT_CHUNK_HEADER) == EVTX_CHUNK_HEADER_SIZE,
              "EVT_CHUNK_HEADER must be exactly 512 bytes");

/**
 * @brief EVT_EVENT_RECORD_HEADER structure
 * 
 * MS-EVTX Section 2.3: Event Record Structure
 * 
 * Offset | Size | Field
 * -------|------|------
 * 0x0000 | 4    | size
 * 0x0004 | 4    | magic (0x20202020)
 * 0x0008 | 8    | record_id
 * 0x0010 | 8    | timestamp (FILETIME)
 * 0x0018 | ...  | data (binary XML)
 */
struct EVT_EVENT_RECORD_HEADER {
    uint32_t size;
    uint32_t magic;
    uint64_t record_id;
    uint64_t timestamp;
    
    bool validate_magic() const noexcept;
    std::string get_timestamp_string() const;
};

constexpr size_t EVTX_EVENT_RECORD_HEADER_SIZE = 24;
constexpr uint32_t EVTX_EVENT_RECORD_MAGIC = 0x20202020;

/**
 * @brief Binary XML Token Types (MS-EVTX Section 2.4.1)
 */
enum class BXmlTokenType : uint8_t {
    EndOfStream = 0x00,
    OpenStartElement = 0x01,
    CloseStartElement = 0x02,
    CloseElement = 0x03,
    Value = 0x04,
    Attribute = 0x05,
    CDataSection = 0x06,
    CharRef = 0x07,
    EntityRef = 0x08,
    ProcessingInstruction = 0x09,
    Comment = 0x0A,
    StartOfStream = 0x0B,
    WhiteSpace = 0x0C,
    EndElementTag = 0x0D,
    StartElementTag = 0x0E
};

/**
 * @brief Event Record structure for parsed data
 */
struct EventRecord {
    uint64_t record_id;
    std::string timestamp;
    uint32_t event_id;
    std::string provider_name;
    std::string level;
    std::string channel;
    std::string computer;
    std::string message;
    std::string xml_content;
    
    std::string to_json() const;
};

}  // namespace Evtx
