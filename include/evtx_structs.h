#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace Evtx {

#pragma pack(push, 1)

struct EVT_FILE_HEADER {
  std::array<uint8_t, 8> magic;
  uint64_t file_size;
  uint64