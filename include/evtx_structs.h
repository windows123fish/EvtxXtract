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
  uint16_t get_minor_version