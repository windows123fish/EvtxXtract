#include "evtx_structs.h"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <ctime>

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

bool EVT_EVENT_RECORD_HEADER::validate_magic() const noexcept {
    return magic == EVTX_EVENT_RECORD_MAGIC;
}

std::string EVT_EVENT_RECORD_HEADER::get_timestamp_string() const {
    // Convert FILETIME (100-nanosecond intervals since 1601-01-01) to time_t
    // FILETIME is in little-endian format
    uint64_t filetime = timestamp;
    
    // Convert to time_t (seconds since 1970-01-01)
    // FILETIME epoch is 1601-01-01, time_t epoch is 1970-01-01
    // Difference is 11644473600 seconds
    const uint64_t FILETIME_TO_TIME_T_OFFSET = 11644473600ULL;
    const uint64_t HUNDRED_NS_PER_SECOND = 10000000ULL;
    
    uint64_t seconds_since_epoch = (filetime / HUNDRED_NS_PER_SECOND) - FILETIME_TO_TIME_T_OFFSET;
    
    std::time_t tt = static_cast<std::time_t>(seconds_since_epoch);
    std::tm tm_buf{};
    
#ifdef _WIN32
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << (tm_buf.tm_year + 1900) << "-"
        << std::setw(2) << (tm_buf.tm_mon + 1) << "-"
        << std::setw(2) << tm_buf.tm_mday << " "
        << std::setw(2) << tm_buf.tm_hour << ":"
        << std::setw(2) << tm_buf.tm_min << ":"
        << std::setw(2) << tm_buf.tm_sec << " UTC";
    
    return oss.str();
}

std::string EventRecord::to_json() const {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"record_id\": " << record_id << ",\n"
        << "  \"timestamp\": \"" << timestamp << "\",\n"
        << "  \"event_id\": " << event_id << ",\n"
        << "  \"provider_name\": \"" << provider_name << "\",\n"
        << "  \"level\": \"" << level << "\",\n"
        << "  \"channel\": \"" << channel << "\",\n"
        << "  \"computer\": \"" << computer << "\",\n"
        << "  \"message\": \"" << message << "\"\n"
        << "}";
    return oss.str();
}

}  // namespace Evtx
