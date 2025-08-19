// CHIP-8 APU header file.
#pragma once
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <random>

class C8Apu {
public:
	C8Apu();
	~C8Apu() = default;

	void load_rom(const std::string& path);
	void fetch_decode_execute();

	[[nodiscard]] void print_V() const noexcept {
		for (size_t i = 0; i < 16; i++) {
			std::cout << "m_V[" << i  << "]: " << m_V[i] << std::endl;
		}
	}
	[[nodiscard]] void print_PC() const noexcept {
		std::cout << "m_pc: " << m_pc << std::endl;
	}
	[[nodiscard]] const uint8_t* get_gfx() const noexcept { return m_gfx; }

private:
	// Memory
	uint8_t   m_memory[4096]; // 4096 bytes of memory. 4 kilobytes.
	uint8_t   m_V[16]; // 16 8-bit registers (from V0 to VF)
	uint16_t  m_I; // Register-index I
	uint16_t  m_pc; // Program counter
	// Stack
	uint16_t  m_stack[16]; // Stack
	uint8_t	  m_sp; // Stack pointer
	// Timers
	uint8_t   m_delay_timer; 
	uint8_t   m_sound_timer;
	// Graphic
	uint8_t   m_gfx[64 * 32]; // Videomemory for screen 64x32
	// Keyboard
	uint8_t   m_key[16]; 
private:
	std::mt19937 m_rand_engine;
	std::uniform_int_distribution<unsigned int> m_rand_dist;
};
