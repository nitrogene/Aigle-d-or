#ifndef FILELOADER_H
#define FILELOADER_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Helper utility to load binary files (ROMs, TAPs) into memory independently from Qt.
 */
class FileLoader {
public:
    /**
     * @brief Loads a binary file entirely into a byte vector.
     * @param filepath The path to the file (e.g., "ROMS/basic11b.rom")
     * @param buffer The vector where the data will be appended/stored.
     * @return True if loading was successful, False otherwise.
     */
    static bool loadBinary(const std::string& filepath, std::vector<uint8_t>& buffer);
};

#endif // FILELOADER_H
