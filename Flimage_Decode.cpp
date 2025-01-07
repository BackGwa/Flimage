#include <iostream>
#include <fstream>
#include <vector>
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

static std::string bytesToString(const std::vector<unsigned char>& v, size_t start, size_t len) {
    if (start + len > v.size()) throw std::runtime_error("범위 오류");
    return std::string(v.begin() + start, v.begin() + start + len);
}

static uint32_t bytesToInt(const std::vector<unsigned char>& data, size_t offset) {
    if (offset + 4 > data.size()) throw std::runtime_error("범위 오류");
    uint32_t val = 0;
    val |= (data[offset + 0] << 0);
    val |= (data[offset + 1] << 8);
    val |= (data[offset + 2] << 16);
    val |= (data[offset + 3] << 24);
    return val;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) return 0;
        std::string bmpPath = argv[1];
        std::vector<unsigned char> bmpData = readFileAll(bmpPath);
        if (bmpData.size() < 54) throw std::runtime_error("BMP 헤더 오류");
        uint32_t width = 0, height = 0, pixelDataOffset = 0;
        width |= bmpData[18];
        width |= (bmpData[19] << 8);
        width |= (bmpData[20] << 16);
        width |= (bmpData[21] << 24);
        height |= bmpData[22];
        height |= (bmpData[23] << 8);
        height |= (bmpData[24] << 16);
        height |= (bmpData[25] << 24);
        pixelDataOffset |= bmpData[10];
        pixelDataOffset |= (bmpData[11] << 8);
        pixelDataOffset |= (bmpData[12] << 16);
        pixelDataOffset |= (bmpData[13] << 24);
        size_t rowSize = width * 4;
        size_t imageSize = rowSize * height;
        if (pixelDataOffset + imageSize > bmpData.size()) throw std::runtime_error("BMP 픽셀 범위 오류");
        std::vector<unsigned char> extracted;
        extracted.reserve(imageSize);
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (int c = 0; c < 4; ++c) {
                    size_t pos = pixelDataOffset + (y * rowSize) + (x * 4) + c;
                    extracted.push_back(bmpData[pos]);
                }
            }
        }
        if (extracted.size() < 4) throw std::runtime_error("추출 데이터 부족");
        uint32_t orgFileSize = bytesToInt(extracted, 0);
        size_t offset = 4;
        if (offset >= extracted.size()) throw std::runtime_error("확장자 길이 오류");
        uint8_t extLen = extracted[offset++];
        if (offset + extLen > extracted.size()) throw std::runtime_error("확장자 범위 오류");
        std::string ext = bytesToString(extracted, offset, extLen);
        offset += extLen;
        if (offset >= extracted.size()) throw std::runtime_error("이름 길이 오류");
        uint8_t nameLen = extracted[offset++];
        if (offset + nameLen > extracted.size()) throw std::runtime_error("이름 범위 오류");
        std::string name = bytesToString(extracted, offset, nameLen);
        offset += nameLen;
        if (offset + orgFileSize > extracted.size()) throw std::runtime_error("파일 본문 범위 오류");
        std::vector<unsigned char> fileContent(extracted.begin() + offset, extracted.begin() + offset + orgFileSize);
        std::string outName = name;
        if (!ext.empty()) outName += "." + ext;
        writeFileAll(outName, fileContent);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}