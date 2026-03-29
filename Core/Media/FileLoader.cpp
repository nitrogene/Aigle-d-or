#include "FileLoader.h"
#include <fstream>
#include <iostream>

bool FileLoader::loadBinary(const std::string& filepath, std::vector<uint8_t>& buffer) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "[FileLoader] Error: Cannot open file " << filepath << std::endl;
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    buffer.resize(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return true;
    }
    
    std::cerr << "[FileLoader] Error: Failed to read data from " << filepath << std::endl;
    return false;
}
