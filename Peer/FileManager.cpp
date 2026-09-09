#include "FileManager.h"
#include "Protocol.h"
#include <fstream>
#include <stdexcept>

uint32_t FileManager::getFileSize(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open file: " + filepath);
    return static_cast<uint32_t>(file.tellg());
}

uint32_t FileManager::getChunkCount(const std::string& filepath) {
    uint32_t size = getFileSize(filepath);
    // Round up: even a 1-byte file needs 1 chunk
    return (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
}

std::vector<char> FileManager::readChunk(const std::string& filepath, uint32_t chunkIndex) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + filepath);

    // Seek to the start of this chunk
    uint64_t offset = static_cast<uint64_t>(chunkIndex) * CHUNK_SIZE;
    file.seekg(static_cast<std::streamoff>(offset));

    std::vector<char> buf(CHUNK_SIZE);
    file.read(buf.data(), CHUNK_SIZE);
    // Shrink to actual bytes read (last chunk is usually smaller)
    buf.resize(static_cast<size_t>(file.gcount()));
    return buf;
}

void FileManager::writeChunk(const std::string& outputPath,
                              uint32_t chunkIndex,
                              const std::vector<char>& data) {
    // Open existing file for random write without truncation.
    // Create the file first if it doesn't exist yet.
    std::fstream file(outputPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        std::ofstream create(outputPath, std::ios::binary);
        create.close();
        file.open(outputPath, std::ios::binary | std::ios::in | std::ios::out);
    }
    if (!file) throw std::runtime_error("Cannot open output file: " + outputPath);

    uint64_t offset = static_cast<uint64_t>(chunkIndex) * CHUNK_SIZE;
    file.seekp(static_cast<std::streamoff>(offset));
    file.write(data.data(), static_cast<std::streamsize>(data.size()));

    if (!file) throw std::runtime_error("Write failed for chunk " + std::to_string(chunkIndex));
}
