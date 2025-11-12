#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 3) {

        std::cerr << "Usage: " << argv[0] << " <CombatMaster.exe> <CrlyMod vNext (main-dirty)>\n";
        return 1;
    }

    const std::filesystem::path target = argv[1];
    const std::string watermark = argv[2];
    const char MAGIC[] = "CRLYWMK1";
    constexpr size_t MAGIC_SIZE = 8;

    if (!std::filesystem::exists(target)) {
        std::cerr << "Target file does not exist: " << target << '\n';
        return 2;
    }

    std::ifstream in(target, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading\n";
        return 3;
    }
    in.seekg(0, std::ios::end);
    std::streamoff fsize = in.tellg();

    if (fsize >= static_cast<std::streamoff>(MAGIC_SIZE + sizeof(uint32_t))) {
        in.seekg(fsize - static_cast<std::streamoff>(MAGIC_SIZE));
        char bufMagic[MAGIC_SIZE];
        in.read(bufMagic, MAGIC_SIZE);
        if (in && std::memcmp(bufMagic, MAGIC, MAGIC_SIZE) == 0) {
            in.seekg(fsize - static_cast<std::streamoff>(MAGIC_SIZE + sizeof(uint32_t)));
            uint32_t len = 0;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (in && len > 0 && fsize >= static_cast<std::streamoff>(MAGIC_SIZE + sizeof(uint32_t) + len)) {
                in.seekg(fsize - static_cast<std::streamoff>(MAGIC_SIZE + sizeof(uint32_t) + len));
                std::string existing(len, '\0');
                in.read(&existing[0], len);
                if (in) {
                    std::cout << "Existing watermark found: " << existing << '\n';
                    return 0;
                }
            }
        }
    }
    in.close();

    std::ofstream out(target, std::ios::binary | std::ios::app);
    if (!out) {
        std::cerr << "[Component/Watermark] Failed to open file for appending\n";
        return 4;
    }

    uint32_t len = static_cast<uint32_t>(watermark.size());
    out.write(watermark.data(), watermark.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(MAGIC, MAGIC_SIZE);
    out.flush();
    out.close();

    std::cout << "Watermark appended successfully.\n";
    return 0;
}
