#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>

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

static std::vector<unsigned char> stringToBytes(const std::string& str) {
    return std::vector<unsigned char>(str.begin(), str.end());
}

static std::string getBaseName(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) slashPos = 0;
    else slashPos += 1;
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos < slashPos) {
        return path.substr(slashPos);
    }
    return path.substr(slashPos, dotPos - slashPos);
}

static std::string getExtension(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return "";
    return path.substr(dotPos + 1);
}

static void intToBytes(uint32_t val, std::vector<unsigned char>& out) {
    out.push_back((unsigned char)((val >>  0) & 0xFF));
    out.push_back((unsigned char)((val >>  8) & 0xFF));
    out.push_back((unsigned char)((val >> 16) & 0xFF));
    out.push_back((unsigned char)((val >> 24) & 0xFF));
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) return 0;
        std::string inputFilePath = argv[1];

        std::vector<unsigned char> fileData = readFileAll(inputFilePath);

        std::string name = getBaseName(inputFilePath);
        std::string ext = getExtension(inputFilePath);
        uint32_t fileSize = (uint32_t)fileData.size();

        std::vector<unsigned char> combinedData;

        intToBytes(fileSize, combinedData);

        combinedData.push_back((unsigned char)ext.size());
        {
            auto extBytes = stringToBytes(ext);
            combinedData.insert(combinedData.end(), extBytes.begin(), extBytes.end());
        }

        combinedData.push_back((unsigned char)name.size());
        {
            auto nameBytes = stringToBytes(name);
            combinedData.insert(combinedData.end(), nameBytes.begin(), nameBytes.end());
        }

        combinedData.insert(combinedData.end(), fileData.begin(), fileData.end());

        size_t totalSize = combinedData.size();
        size_t pixelCount = (totalSize + 3) / 4;

        size_t width = (size_t)std::ceil(std::sqrt((double)pixelCount));
        size_t height = (pixelCount + width - 1) / width;
        
        size_t imageBufferSize = width * height * 4;
        std::vector<unsigned char> rawPixels(imageBufferSize, 0);

        {
            size_t dataIndex = 0;
            for (size_t i = 0; i < imageBufferSize; i++) {
                unsigned char value = 0;
                if (dataIndex < totalSize) {
                    value = combinedData[dataIndex++];
                }
                rawPixels[i] = value;
            }
        }

        std::vector<unsigned char> pngData;
        unsigned error = lodepng::encode(pngData, rawPixels, (unsigned)width, (unsigned)height);
        if (error) {
            throw std::runtime_error(
                "PNG encode error: " + std::string(lodepng_error_text(error))
            );
        }

        std::string outPng = getBaseName(inputFilePath) + ".png";
        writeFileAll(outPng, pngData);
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] : " << e.what() << std::endl;
        return -1;
    }
    return 0;
}