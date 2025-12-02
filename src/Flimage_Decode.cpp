#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdint>

#include "lodepng.h"

static std::vector<unsigned char> readFileAll(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) throw std::runtime_error("Failed to open file");
    std::streampos fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(fileSize);
    if (!ifs.read(reinterpret_cast<char*>(buffer.data()), fileSize)) 
        throw std::runtime_error("Failed to read file");
    return buffer;
}

static void writeFileAll(const std::string& path, const std::vector<unsigned char>& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) throw std::runtime_error("Failed to write file");
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    ofs.close();
}

static std::string bytesToString(const std::vector<unsigned char>& v, size_t start, size_t len) {
    if (start + len > v.size()) throw std::runtime_error("Out of range");
    return std::string(v.begin() + start, v.begin() + start + len);
}

static uint32_t bytesToInt(const std::vector<unsigned char>& data, size_t offset) {
    if (offset + 4 > data.size()) throw std::runtime_error("Out of range");
    uint32_t val = 0;

    val |= (static_cast<uint32_t>(data[offset + 0]) <<  0);
    val |= (static_cast<uint32_t>(data[offset + 1]) <<  8);
    val |= (static_cast<uint32_t>(data[offset + 2]) << 16);
    val |= (static_cast<uint32_t>(data[offset + 3]) << 24);
    return val;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "[Usage] : " << argv[0] << " <png_file>" << std::endl;
            return 0;
        }

        std::string pngPath = argv[1];
        std::vector<unsigned char> pngData = readFileAll(pngPath);

        std::vector<unsigned char> decodedRGBA;
        unsigned width = 0, height = 0;
        unsigned error = lodepng::decode(decodedRGBA, width, height, pngData);
        if (error) {
            throw std::runtime_error("PNG decode error: " + std::string(lodepng_error_text(error)));
        }

        if (decodedRGBA.size() < 4) {
            throw std::runtime_error("Insufficient RGBA data to extract fileSize");
        }

        uint32_t orgFileSize = bytesToInt(decodedRGBA, 0);
        size_t offset = 4;

        if (offset >= decodedRGBA.size()) {
            throw std::runtime_error("Invalid extension length offset");
        }
        uint8_t extLen = decodedRGBA[offset++];
        if (offset + extLen > decodedRGBA.size()) {
            throw std::runtime_error("Extension out of range");
        }
        std::string ext = bytesToString(decodedRGBA, offset, extLen);
        offset += extLen;

        if (offset >= decodedRGBA.size()) {
            throw std::runtime_error("Invalid filename length offset");
        }
        uint8_t nameLen = decodedRGBA[offset++];
        if (offset + nameLen > decodedRGBA.size()) {
            throw std::runtime_error("Name out of range");
        }
        std::string name = bytesToString(decodedRGBA, offset, nameLen);
        offset += nameLen;

        if (offset + orgFileSize > decodedRGBA.size()) {
            throw std::runtime_error("File content out of range");
        }
        std::vector<unsigned char> fileContent(
            decodedRGBA.begin() + offset,
            decodedRGBA.begin() + offset + orgFileSize
        );
        offset += orgFileSize;

        std::string outName = name;
        if (!ext.empty()) {
            outName += "." + ext;
        }
        writeFileAll(outName, fileContent);
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] : " << e.what() << std::endl;
        return -1;
    }
    return 0;
}