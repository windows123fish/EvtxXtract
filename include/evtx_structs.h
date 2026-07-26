#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace Evtx {

#pragma pack(push, 1)

struct EVT_FILE_HEADER {
  std::array<uint8_t, 8> magic;
  uint64_t file_size;
  uint64_t oldest_chunk_offset;
  uint64_t newest_chunk_offset;
  uint32_t version;
  uint16_t flags;
  uint16_t chunk_count;
  uint32_t checksum;
  std::array<uint8_t, 4052> reserved;

  bool validate_magic() const noexcept;
  uint16_t get_major_version() const noexcept;
  uint16_t get_minor_version() const noexcept;
  bool is_dirty() const noexcept;
  std::string to_string() const;
};

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

}  // namespace Evtx
