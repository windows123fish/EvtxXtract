#include "evtx_parser.h"
#include <cstring>
#include <stdexcept>

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
      set_error("Failed to seek to chunk " + std::to_string(i) + " at offset " + std::to_string(chunk_offset));
      break;
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

}  // namespace Evtx
