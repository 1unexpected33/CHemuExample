// CHIP-8 code realisation file
#include "c8apu.hpp"

C8Apu::C8Apu() : m_pc(0x200), m_I(0), m_sp(0), m_delay_timer(0), m_sound_timer(0), m_rand_engine(std::random_device{}()), m_rand_dist(0, 255){
	memset(m_memory, 0, sizeof(m_memory));
	memset(m_V, 0, sizeof(m_V));
	memset(m_stack, 0, sizeof(m_stack));
	memset(m_gfx, 0, sizeof(m_gfx));
	memset(m_key, 0, sizeof(m_key));
}

void C8Apu::load_rom(const std::string& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) throw std::runtime_error("Cannot open the file by path: " + path);

	std::streampos size = file.tellg();
	
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(m_memory) + 0x200, size);

	file.close();

	printf("Successfully read data from the file to m_memory.");
}
void C8Apu::fetch_decode_execute() {
	uint16_t opcode = m_memory[m_pc] << 8 | m_memory[m_pc + 1];

	switch (opcode & 0xF000) {
	case 0x0000: {
		switch (opcode & 0x00FF) {
		case 0x00E0: { // CLS
			memset(m_gfx, 0, sizeof(m_gfx));
			m_pc += 2;
			break;
		}
		case 0x00EE: { // RET
			m_sp--;
			m_pc = m_stack[m_sp]; // Loading address to return from stack to PC
			m_pc += 2;
			break;
		}
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
			m_pc += 2;
		} // end of the second switch
		break; // break for second switch
	} // end of the case 0x0000
	case 0x1000: { // JP
		uint16_t address = opcode & 0x0FFF;
		m_pc = address;
		break;
	}
	case 0xA000: { // LD
		uint16_t address = opcode & 0x0FFF;
		m_I = address;
		m_pc += 2;
		break;
	}
	case 0x2000: { // CALL
		m_stack[m_sp] = m_pc;
		m_sp++;

		uint16_t address = opcode & 0x0FFF;
		m_pc = address;
		break;
	}
	case 0x3000: { // SE
		uint8_t reg_index = (opcode & 0x0F00) >> 8;
		uint8_t value = opcode & 0x00FF;

		if (m_V[reg_index] == value)
			m_pc += 4;
		else m_pc += 2;

		break;
	}
	case 0x4000: { // SNE
		uint8_t reg_index = (opcode & 0x0F00) >> 8;
		uint8_t value = opcode & 0x00FF;

		if (m_V[reg_index] != value)
			m_pc += 4;
		else m_pc += 2;

		break;
	}
	case 0x6000: { // LD Vx, byte
		uint8_t reg_index = (opcode & 0x0F00) >> 8; // Register index (mask 0x0F00)
		uint8_t value = opcode & 0x00FF; // Value (mask 0x00FF)

		m_V[reg_index] = value; // Loading value to register (useful)
		m_pc += 2;

		break;
	}
	case 0x7000: { // ADD Vx, byte
		uint8_t reg_index = (opcode & 0x0F00) >> 8; // Register index (mask 0x0F00)
		uint8_t value = opcode & 0x00FF;

		m_V[reg_index] += value;
		m_pc += 2;

		break;
	}
	case 0x8000: {
		uint8_t reg_index_x = (opcode & 0x0F00) >> 8;
		uint8_t reg_index_y = (opcode & 0x00F0) >> 4;

		switch (opcode & 0x000F) {
		case 0x0000: // 8xy0 - LD Vx, Vy
			m_V[reg_index_x] = m_V[reg_index_y];
			m_pc += 2;
			break;

		case 0x0001: // 8xy1 - OR Vx, Vy
			m_V[reg_index_x] |= m_V[reg_index_y];
			m_pc += 2;
			break;

		case 0x0002: // 8xy2 - AND Vx, Vy
			m_V[reg_index_x] &= m_V[reg_index_y];
			m_pc += 2;
			break;

		case 0x0003: // 8xy3 - XOR Vx, Vy
			m_V[reg_index_x] ^= m_V[reg_index_y];
			m_pc += 2;
			break;

		case 0x0004: { // 8xy4 - ADD Vx, Vy
			if (m_V[reg_index_y] > (0xFF - m_V[reg_index_x])) {
				m_V[0xF] = 1; // carry
			}
			else {
				m_V[0xF] = 0;
			}
			m_V[reg_index_x] += m_V[reg_index_y];
			m_pc += 2;
			break;
		}

		case 0x0005: { // 8xy5 - SUB Vx, Vy
			if (m_V[reg_index_x] > m_V[reg_index_y]) {
				m_V[0xF] = 1;
			}
			else {
				m_V[0xF] = 0; // borrow
			}
			m_V[reg_index_x] -= m_V[reg_index_y];
			m_pc += 2;
			break;
		}

		case 0x0006: { // 8xy6 - SHR Vx {, Vy}
			m_V[0xF] = m_V[reg_index_x] & 0x1;
			m_V[reg_index_x] >>= 1;
			m_pc += 2;
			break;
		}

		default:
			printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
			m_pc += 2;
		}
		break;
	}
	case 0xD000: { // DRW Vx, Vy
		uint8_t op_x = (opcode & 0x0F00) >> 8; // Index X
		uint8_t op_y = (opcode & 0x00F0) >> 4; // Index Y
		uint8_t height = opcode & 0x000F;
		uint8_t coord_x = m_V[op_x];
		uint8_t coord_y = m_V[op_y];
		m_V[0xF] = 0;

		for (int yline = 0; yline < height; yline++) {
			uint8_t sprite_byte = m_memory[m_I + yline];

			for (int xline = 0; xline < 8; xline++) {
				if ((sprite_byte & (0x80 >> xline)) != 0) {
					int screen_x = (coord_x + xline) % 64;
					int screen_y = (coord_y + yline) % 32;
					int pixel_index = screen_x + (screen_y * 64);

					if (m_gfx[pixel_index] == 1) {
						m_V[0xF] = 1;
					}

					m_gfx[pixel_index] ^= 1;
				}
			}
		}

		m_pc += 2;
		break;
	}
	case 0xC000: { // RND
		uint8_t reg_index = (opcode & 0x0F00) >> 8;
		uint8_t value = opcode & 0x00FF;

		m_V[reg_index] = m_rand_dist(m_rand_engine) & value;

		m_pc += 2;
		break;
	}
	case 0xE000: {
		uint8_t reg_index = (opcode & 0x0F00) >> 8;
		uint8_t key_code = m_V[reg_index];

		switch (opcode & 0x00FF) {
		case 0x009E: // SKP Vx
			if (m_key[key_code] != 0) { // Pressed
				m_pc += 4;
			}
			else {
				m_pc += 2;
			}
			break;

		case 0x00A1: // ExA1: SKNP Vx
			if (m_key[key_code] == 0) { // NOT pressed
				m_pc += 4;
			}
			else {
				m_pc += 2;
			}
			break;

		default:
			printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
			m_pc += 2;
		}
		break;
	}
	case 0xF000: {
		uint8_t reg_index = (opcode & 0x0F00) >> 8;

		switch (opcode & 0x00FF) {
		case 0x0007: { // Fx07: LD Vx, DT
			m_V[reg_index] = m_delay_timer;
			m_pc += 2;
			break;
		}
		case 0x0015: {// Fx15: LD DT, Vx
			m_delay_timer = m_V[reg_index];
			m_pc += 2;
			break;
		}
		case 0x001E: { // Fx1E: ADD I, Vx
			m_I += m_V[reg_index];
			m_pc += 2;
			break;
		}

		default:
			printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
			m_pc += 2;
		}

		break;
	} // 0xF000

	default:
		printf("Unknown opcode: 0x%X\n", opcode);
		m_pc += 2;
	}
}
