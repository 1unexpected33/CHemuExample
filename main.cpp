#include "vm/c8apu.hpp"
#include <windows.h>
#include <cstdint>
#include <chrono>
#include <thread>

C8Apu myChip8;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file path>" << std::endl;
        return 1;
    }

    try {
        myChip8.load_rom(argv[1]);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    myChip8.fetch_decode_execute();

    return 0;
}
