#include "evtx_parser.h"
#include <cstring>
#include <stdexcept>
#include <stack>

namespace Evtx {

EvtxParser::EvtxParser(const std::string& file_path) : file_path_(file_path) {
}

EvtxParser::EvtxParser(const std::wstring& file_path) {
    file_path_ = std::filesystem::path(file_path).string();
}

EvtxParser::~EvtxParser() {
  close();
}

bool EvtxParser::open() {
  // Close any existing open file
  close();

  // Open file in binary mode
  file_stream_.open(file_path_, std::ios::binary);
  
  if (!file_stream_.is_open()) {
    set_error("Failed to open file: " + file_path_);
    return false;
  }

  // Check if file is readable
  if (!file_stream_.good()) {
    set_error("File is not readable: " + file_path_);
    file_stream_.close();
    return false;
  }

  return true;
}

void EvtxParser::close() {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  valid_chunks_.clear();
}

bool EvtxParser::read_file_header() {
  if (!file_stream_.is_open()) {
    set_error("File is not open");
    return false;
  }

  // Reset file position to beginning
  file_stream_.seekg(0, std::ios::beg);

  // Read exactly EVTX_FILE_HEADER_SIZE bytes
  file_stream_.read(reinterpret_cast<char*>(&file_header_), EVTX_FILE_HEADER_SIZE);

  // Check if read was successful
  if (!file_stream_.good() || file_stream_.gcount() != EVTX_FILE_HEADER_SIZE) {
    set_error("Failed to read file header. File may be too small.");
    return false;
  }

  // Validate magic number
  if (!file_header_.validate_magic()) {
    set_error("Invalid file magic. Not a valid .evtx file.");
    return false;
  }

  return true;
}

size_t EvtxParser::validate_chunks() {
  if (!file_stream_.is_open()) {
    set_error("File is not open");
    return 0;
  }

  valid_chunks_.clear();

  // Calculate total file size
  file_stream_.seekg(0, std::ios::end);
  const auto file_size = static_cast<uint64_t>(file_stream_.tellg());
  file_stream_.seekg(0, std::ios::beg);

  // Chunks start at offset 4096 (after file header)
  // Each chunk is 65536 bytes
  const uint64_t first_chunk_offset = EVTX_FILE_HEADER_SIZE;
  
  if (file_size <= first_chunk_offset) {
    // No chunks in the file
    return 0;
  }

  // Calculate number of chunks to check
  const uint64_t chunks_data_size = file_size - first_chunk_offset;
  const size_t num_chunks = static_cast<size_t>(chunks_data_size / EVTX_CHUNK_SIZE);

  EVT_CHUNK_HEADER chunk_header;

  for (size_t i = 0; i < num_chunks; ++i) {
    // Calculate offset for this chunk's header
    const uint64_t chunk_offset = first_chunk_offset + (i * EVTX_CHUNK_SIZE);
    
    // Seek to chunk header position
    file_stream_.seekg(static_cast<std::streampos>(chunk_offset));
    
    if (!file_stream_.good()) {
      // 记录错误但继续检查后续块
      set_error("Failed to seek to chunk " + std::to_string(i) + " at offset " + std::to_string(chunk_offset));
      continue;
    }

    // Read chunk header
    file_stream_.read(reinterpret_cast<char*>(&chunk_header), EVTX_CHUNK_HEADER_SIZE);
    
    if (!file_stream_.good() || file_stream_.gcount() != EVTX_CHUNK_HEADER_SIZE) {
      // Skip corrupted chunks but continue
      continue;
    }

    // Validate chunk magic
    if (chunk_header.validate_magic()) {
      valid_chunks_.push_back(chunk_header);
    }
  }

  return valid_chunks_.size();
}

const EVT_FILE_HEADER& EvtxParser::get_file_header() const noexcept {
  return file_header_;
}

const std::vector<EVT_CHUNK_HEADER>& EvtxParser::get_valid_chunks() const noexcept {
  return valid_chunks_;
}

const std::string& EvtxParser::get_file_path() const noexcept {
  return file_path_;
}

bool EvtxParser::is_open() const noexcept {
  return file_stream_.is_open();
}

const std::string& EvtxParser::get_last_error() const noexcept {
  return last_error_;
}

void EvtxParser::set_error(const std::string& error) {
  last_error_ = error;
}

// Phase 2: Event Record Parsing

uint64_t EvtxParser::get_total_event_count() const noexcept {
    if (valid_chunks_.empty()) {
        return 0;
    }
    return valid_chunks_.back().last_event_record_number;
}

std::vector<EventRecord> EvtxParser::parse_all_events(size_t max_records) {
    std::vector<EventRecord> records;
    
    if (valid_chunks_.empty()) {
        return records;
    }
    
    const uint64_t first_chunk_offset = EVTX_FILE_HEADER_SIZE;
    
    for (size_t i = 0; i < valid_chunks_.size(); ++i) {
        uint64_t chunk_offset = first_chunk_offset + (i * EVTX_CHUNK_SIZE);
        auto chunk_records = parse_chunk_events(i, max_records > 0 ? max_records - records.size() : 0);
        records.insert(records.end(), chunk_records.begin(), chunk_records.end());
        
        if (max_records > 0 && records.size() >= max_records) {
            break;
        }
    }
    
    return records;
}

std::vector<EventRecord> EvtxParser::parse_chunk_events(size_t chunk_index, size_t max_records) {
    std::vector<EventRecord> records;
    
    if (chunk_index >= valid_chunks_.size()) {
        return records;
    }
    
    const uint64_t first_chunk_offset = EVTX_FILE_HEADER_SIZE;
    uint64_t chunk_offset = first_chunk_offset + (chunk_index * EVTX_CHUNK_SIZE);
    const EVT_CHUNK_HEADER& chunk_header = valid_chunks_[chunk_index];
    
    // Events start after chunk header (512 bytes)
    uint64_t current_offset = chunk_offset + EVTX_CHUNK_HEADER_SIZE;
    
    while (current_offset < chunk_offset + EVTX_CHUNK_SIZE && 
           current_offset < chunk_offset + chunk_header.last_event_offset + EVTX_EVENT_RECORD_HEADER_SIZE) {
        
        file_stream_.seekg(static_cast<std::streampos>(current_offset));
        
        if (!file_stream_.good()) {
            break;
        }
        
        EVT_EVENT_RECORD_HEADER record_header;
        file_stream_.read(reinterpret_cast<char*>(&record_header), EVTX_EVENT_RECORD_HEADER_SIZE);
        
        if (!file_stream_.good() || !record_header.validate_magic()) {
            break;
        }
        
        if (record_header.size < EVTX_EVENT_RECORD_HEADER_SIZE) {
            break;
        }
        
        EventRecord record;
        if (parse_event_record(record_header, record)) {
            records.push_back(record);
        }
        
        if (max_records > 0 && records.size() >= max_records) {
            break;
        }
        
        current_offset += record_header.size;
    }
    
    return records;
}

bool EvtxParser::parse_event_record(const EVT_EVENT_RECORD_HEADER& record_header, EventRecord& record) {
    try {
        record.record_id = record_header.record_id;
        record.timestamp = record_header.get_timestamp_string();
        
        // Read binary XML data
        uint32_t data_size = record_header.size - EVTX_EVENT_RECORD_HEADER_SIZE;
        if (data_size == 0) {
            return true;
        }
        
        std::vector<uint8_t> data(data_size);
        file_stream_.read(reinterpret_cast<char*>(data.data()), data_size);
        
        if (!file_stream_.good()) {
            return false;
        }
        
        // Parse binary XML
        parse_binary_xml(data.data(), data_size, record);
        
        return true;
    } catch (...) {
        return false;
    }
}

void EvtxParser::parse_binary_xml(const uint8_t* data, size_t size, EventRecord& record) {
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;
    std::string current_element;
    
    while (ptr < end) {
        parse_bxml_token(ptr, end, record, current_element);
    }
}

uint64_t EvtxParser::read_vlq(const uint8_t*& data) {
    uint64_t result = 0;
    uint8_t byte;
    int shift = 0;
    
    do {
        byte = *data++;
        result |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    return result;
}

std::string EvtxParser::read_bxml_string(const uint8_t*& data, const uint8_t* end) {
    if (data >= end) {
        return "";
    }
    
    // Check string type (byte 0)
    uint8_t type = *data++;
    
    // Type 0x00: Inline ASCII string
    // Type 0x01: Inline Unicode string
    // Type 0x02: Offset to string table
    
    if (type == 0x00) {
        // Inline ASCII string (length-prefixed)
        uint64_t length = read_vlq(data);
        std::string str;
        str.reserve(length);
        for (uint64_t i = 0; i < length && data < end; ++i) {
            str += *data++;
        }
        return str;
    } else if (type == 0x01) {
        // Inline Unicode string (length-prefixed, UTF-16LE)
        uint64_t length = read_vlq(data);
        std::wstring wstr;
        wstr.reserve(length);
        for (uint64_t i = 0; i < length && data + 1 < end; ++i) {
            wchar_t wc = *reinterpret_cast<const wchar_t*>(data);
            wstr += wc;
            data += 2;
        }
        // Convert to UTF-8
        std::string str;
        for (wchar_t wc : wstr) {
            if (wc < 0x80) {
                str += static_cast<char>(wc);
            } else if (wc < 0x800) {
                str += static_cast<char>(0xC0 | (wc >> 6));
                str += static_cast<char>(0x80 | (wc & 0x3F));
            } else {
                str += static_cast<char>(0xE0 | (wc >> 12));
                str += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                str += static_cast<char>(0x80 | (wc & 0x3F));
            }
        }
        return str;
    } else {
        // Type 0x02 or unknown - skip
        return "";
    }
}

void EvtxParser::parse_bxml_token(const uint8_t*& data, const uint8_t* end, EventRecord& record, std::string& current_element) {
    if (data >= end) {
        return;
    }
    
    uint8_t token_type = *data++;
    
    switch (static_cast<BXmlTokenType>(token_type)) {
        case BXmlTokenType::StartOfStream:
        case BXmlTokenType::EndOfStream:
            break;
            
        case BXmlTokenType::OpenStartElement: {
            std::string element_name = read_bxml_string(data, end);
            current_element = element_name;
            
            // Skip attributes
            uint64_t attr_count = read_vlq(data);
            for (uint64_t i = 0; i < attr_count; ++i) {
                std::string attr_name = read_bxml_string(data, end);
                std::string attr_value = read_bxml_string(data, end);
                
                // Extract key attributes
                if (attr_name == "Id" && current_element == "EventID") {
                    try {
                        record.event_id = std::stoul(attr_value);
                    } catch (...) {}
                } else if (attr_name == "Name" && current_element == "Provider") {
                    record.provider_name = attr_value;
                } else if (attr_name == "Level") {
                    record.level = attr_value;
                }
            }
            break;
        }
        
        case BXmlTokenType::CloseStartElement:
        case BXmlTokenType::EndElementTag:
        case BXmlTokenType::CloseElement:
            current_element.clear();
            break;
            
        case BXmlTokenType::Value: {
            std::string value = read_bxml_string(data, end);
            
            // Map element names to fields
            if (current_element == "EventID") {
                try {
                    record.event_id = std::stoul(value);
                } catch (...) {}
            } else if (current_element == "Level") {
                record.level = value;
            } else if (current_element == "Channel") {
                record.channel = value;
            } else if (current_element == "Computer") {
                record.computer = value;
            } else if (current_element == "Message") {
                record.message = value;
            } else if (current_element == "Provider") {
                record.provider_name = value;
            }
            break;
        }
        
        case BXmlTokenType::Attribute: {
            std::string attr_name = read_bxml_string(data, end);
            std::string attr_value = read_bxml_string(data, end);
            
            if (attr_name == "Name") {
                record.provider_name = attr_value;
            }
            break;
        }
        
        case BXmlTokenType::WhiteSpace: {
            uint64_t length = read_vlq(data);
            data += length;
            break;
        }
        
        case BXmlTokenType::CharRef: {
            uint64_t char_code = read_vlq(data);
            // Handle character reference if needed
            break;
        }
        
        default:
            // Skip unknown tokens
            break;
    }
}

}  // namespace Evtx
