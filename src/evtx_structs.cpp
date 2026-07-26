#include "../include/evtx_structs.h"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace Evtx {

bool EVT_FILE_HEADER::validate_magic() const noexcept {
  // Compare magic bytes against expected "ElfFile\x00"
  // Using std::memcmp for performance with raw byte arrays
  return std::memcmp(magic.data(), EVTX_FILE_MAGIC.data(), magic.size()) == 0;
}

uint16_t EVT_FILE_HEADER::get_major_version() const noexcept {
  // Major version is stored in the high 16 bits of version (little-endian)
  return static_cast<uint16_t>((version >> 16) & 0xFFFF);
}

uint16_t EVT_FILE_HEADER::get_minor_version() const noexcept {
  // Minor version is stored in the low 16 bits of version (little-endian)
  return static_cast<uint16_t>(version & 0xFFFF);
}

bool EVT_FILE_HEADER::is_dirty() const noexcept {
  // Bit 0: Dirty flag - set if file was not properly closed
  return (flags & 0x01) != 0;
}

std::string EVT_FILE_HEADER::to_string() const {
  std::ostringstream oss;
  oss << "EVT_FILE_HEADER:\n"
      << "  Magic: " << std::string(reinterpret_cast<const char*>(magic.data()), 7) << "\n"
      << "  Version: " << get_major_version() << "." << get_minor_version() << "\n"
      << "  Flags: 0x" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(flags) << std::dec << "\n"
      << "    - Dirty: " << (is_dirty() ? "Yes" : "No") << "\n"
      << "  Chunk Count: " << chunk_count << "\n"
      << "  File Size: " << file_size << " bytes\n"
      << "  Oldest Chunk Offset: " << oldest_chunk_offset << "\n"
      << "  Newest Chunk Offset: " << newest_chunk_offset << "\n"
      << "  Checksum: 0x" << std::hex << std::setw(8) << std::setfill('0') << checksum << std::dec;
  return oss.str();
}

bool EVT_CHUNK_HEADER::validate_magic() const noexcept {
  // Compare magic bytes against expected "ElfChnk\x00"
  return std::memcmp(magic.data(), EVTX_CHUNK_MAGIC.data(), magic.size()) == 0;
}

std::string EVT_CHUNK_HEADER::to_string() const {
  std::ostringstream oss;
  oss << "EVT_CHUNK_HEADER:\n"
      << "  Magic: " << std::string(reinterpret_cast<const char*>(magic.data()), 7) << "\n"
      << "  Header Size: " << header_size << " bytes\n"
      << "  First Event Record Number: " << first_event_record_number << "\n"
      << "  Last Event Record Number: " << last_event_record_number << "\n"
      << "  First Event Record ID: " << first_event_record_id << "\n"
      << "  Last Event Record ID: " << last_event_record_id << "\n"
      << "  Last Event Offset: " << last_event_offset << "\n"
      << "  Free Space Offset: " << free_space_offset << "\n"
      << "  Events Checksum: 0x" << std::hex << std::setw(8) << std::setfill('0') << events_checksum << std::dec << "\n"
      << "  Flags: 0x" << std::hex << std::setw(8) << std::setfill('0') << flags << std::dec << "\n"
      << "  Chunk Checksum: 0x" << std::hex << std::setw(8) << std::setfill('0') << chunk_checksum << std::dec << "\n"
      << "  String Offset Array Offset: " << string_offset_array_offset;
  return oss.str();
}

}  // namespace Evtx
