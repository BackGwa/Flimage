#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>

static std::vector<unsigned char> readFileAll(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) throw std::runtime_error("파일을 열 수 없음");
    std::streampos fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(fileSize);
    if (!ifs.read(reinterpret_cast<char*>(buffer.data()), fileSize)) throw std::runtime_error("파일 읽기 실패");
    return buffer;
}

static void writeFileAll(const std::string& path, const std::vector<unsigned char>& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) throw std::runtime_error("파일을 쓸 수 없음");
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    ofs.close();
}

static std::vector<unsigned char> stringToBytes(const std::string& str) {
    return std::vector<unsigned char>(str.begin(), str.end());
}

static std::string getBaseName(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) slashPos = 0; else slashPos += 1;
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos < slashPos) return path.substr(slashPos);
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
        size_t rowSize = width * 4;
        size_t imageSize = rowSize * height;
        size_t fileSizeBmp = 54 + imageSize;
        std::vector<unsigned char> bmpData(fileSizeBmp, 0);

        bmpData[0] = 'B'; bmpData[1] = 'M';
        {
            uint32_t fsize = (uint32_t)fileSizeBmp;
            bmpData[2] = (unsigned char)(fsize & 0xFF);
            bmpData[3] = (unsigned char)((fsize >> 8) & 0xFF);
            bmpData[4] = (unsigned char)((fsize >> 16) & 0xFF);
            bmpData[5] = (unsigned char)((fsize >> 24) & 0xFF);
        }
        bmpData[10] = 54;
        bmpData[14] = 40;
        {
            uint32_t w = (uint32_t)width;
            bmpData[18] = (unsigned char)(w & 0xFF);
            bmpData[19] = (unsigned char)((w >> 8) & 0xFF);
            bmpData[20] = (unsigned char)((w >> 16) & 0xFF);
            bmpData[21] = (unsigned char)((w >> 24) & 0xFF);
        }
        {
            uint32_t h = (uint32_t)height;
            bmpData[22] = (unsigned char)(h & 0xFF);
            bmpData[23] = (unsigned char)((h >> 8) & 0xFF);
            bmpData[24] = (unsigned char)((h >> 16) & 0xFF);
            bmpData[25] = (unsigned char)((h >> 24) & 0xFF);
        }
        bmpData[26] = 1;
        bmpData[28] = 32;
        {
            uint32_t isize = (uint32_t)imageSize;
            bmpData[34] = (unsigned char)(isize & 0xFF);
            bmpData[35] = (unsigned char)((isize >> 8) & 0xFF);
            bmpData[36] = (unsigned char)((isize >> 16) & 0xFF);
            bmpData[37] = (unsigned char)((isize >> 24) & 0xFF);
        }

        size_t dataIndex = 0;
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                for (int c = 0; c < 4; ++c) {
                    unsigned char value = 0;
                    if (dataIndex < totalSize) value = combinedData[dataIndex++];
                    size_t pos = 54 + (y * rowSize) + (x * 4) + c;
                    bmpData[pos] = value;
                }
            }
        }

        std::string outBmp = getBaseName(inputFilePath) + ".bmp";
        writeFileAll(outBmp, bmpData);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}