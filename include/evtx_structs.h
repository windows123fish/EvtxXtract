#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace Evtx {

#pragma pack(push, 1)

struct EVT_FILE_HEADER {
  std::array<uint8_t