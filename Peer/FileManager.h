#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Static utility class for reading and writing file chunks.
// Files are split into fixed-size CHUNK_SIZE blocks (defined in Protocol.h).
// Chunks are written at the correct byte offset, allowing random-order writes
// (needed for future parallel multi-peer downloads).
class FileManager {
public:
    // Returns the size of the file in bytes.
    static uint32_t getFileSize(const std::string& filepath);

    // Returns the number of CHUNK_SIZE chunks needed to cover the file.
    static uint32_t getChunkCount(const std::string& filepath);

    // Reads chunk at the given index from filepath and returns its bytes.
    // The last chunk may be smaller than CHUNK_SIZE.
    static std::vector<char> readChunk(const std::string& filepath, uint32_t chunkIndex);

    // Writes data into outputPath at the byte offset for chunkIndex.
    // Creates the file if it doesn't already exist.
    static void writeChunk(const std::string& outputPath,
                           uint32_t chunkIndex,
                           const std::vector<char>& data);
};
