#include <iostream>
#include <fstream>
#include <string>
#include <format>

#include <sdl.h>

#define entry_point 0x200

/**
 * @brief memory layout
 * start   end   name
 * 0x200 - 0xE4F .text,
 * 0xE50 - 0xE9F .data,
 * 0xEA0 - 0xEFF .stack,
 * 0xF00 - 0xFFF .framebuffer (impossible)
 */
static uint8_t mem[4096];
static uint16_t framebuffer[2048];

static uint8_t v[16];
static uint16_t i = 0x0;
static uint16_t pc = entry_point;
static uint16_t sp = 0xEA0;

static uint8_t delay;
static uint8_t sound;
static uint8_t keypad[16];

static uint8_t font[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

static uint16_t fetch();
static void exec(uint16_t opcode);

static std::string disassemble(uint16_t opcode);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: <filepath>" << std::endl;
		return 1;
	}

	std::ifstream f(argv[1], std::ios::binary | std::ios::ate);

	if (!f.is_open()) {
		std::cerr << "Error: invalid file" << std::endl;
		return 1;
	}

	size_t filesize = f.tellg();
	f.seekg(0);

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cerr << "SDL_Init Failed: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("chip8_emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 640, SDL_WINDOW_OPENGL);

	if (window == nullptr) {
		std::cout << "SDL_CreateWindow Failed: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Surface *surface = SDL_GetWindowSurface(window);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 64, 32);

	uint16_t opcode;
	size_t offset = 0;

	// set up font in memory
	memcpy(mem + 0xE50, font, 80);

	while (!f.eof()) {
		f.read((char *)&opcode, 2);
		memcpy(mem + entry_point + offset, &opcode, 2);
		offset += 2;
	}

	while (pc < (entry_point + filesize)) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_1) {
					keypad[0x1] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_2) {
					keypad[0x2] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_3) {
					keypad[0x3] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_4) {
					keypad[0xC] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_q) {
					keypad[0x4] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_w) {
					keypad[0x5] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_e) {
					keypad[0x6] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_r) {
					keypad[0xD] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_a) {
					keypad[0x7] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_s) {
					keypad[0x8] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_d) {
					keypad[0x9] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_f) {
					keypad[0xE] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_z) {
					keypad[0xA] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_x) {
					keypad[0x0] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_c) {
					keypad[0xB] = 0xFF;
				} else if (event.key.keysym.sym == SDLK_v) {
					keypad[0xF] = 0xFF;
				}
			} else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.sym == SDLK_1) {
					keypad[0x1] = 0x0;
				} else if (event.key.keysym.sym == SDLK_2) {
					keypad[0x2] = 0x0;
				} else if (event.key.keysym.sym == SDLK_3) {
					keypad[0x3] = 0x0;
				} else if (event.key.keysym.sym == SDLK_4) {
					keypad[0xC] = 0x0;
				} else if (event.key.keysym.sym == SDLK_q) {
					keypad[0x4] = 0x0;
				} else if (event.key.keysym.sym == SDLK_w) {
					keypad[0x5] = 0x0;
				} else if (event.key.keysym.sym == SDLK_e) {
					keypad[0x6] = 0x0;
				} else if (event.key.keysym.sym == SDLK_r) {
					keypad[0xD] = 0x0;
				} else if (event.key.keysym.sym == SDLK_a) {
					keypad[0x7] = 0x0;
				} else if (event.key.keysym.sym == SDLK_s) {
					keypad[0x8] = 0x0;
				} else if (event.key.keysym.sym == SDLK_d) {
					keypad[0x9] = 0x0;
				} else if (event.key.keysym.sym == SDLK_f) {
					keypad[0xE] = 0x0;
				} else if (event.key.keysym.sym == SDLK_z) {
					keypad[0xA] = 0x0;
				} else if (event.key.keysym.sym == SDLK_x) {
					keypad[0x0] = 0x0;
				} else if (event.key.keysym.sym == SDLK_c) {
					keypad[0xB] = 0x0;
				} else if (event.key.keysym.sym == SDLK_v) {
					keypad[0xF] = 0x0;
				}
			}
		}

		if (delay > 0) {
			delay--;
		}
		if (sound > 0) {
			sound--;
		}

		opcode = fetch();
#ifdef DEBUG
		std::cout << std::hex << pc - 0x2 << " " << disassemble(opcode) << std::endl;
#endif
		exec(opcode);
		SDL_UpdateTexture(texture, NULL, framebuffer, 32 * 4);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(1);
	}

	f.close();
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

uint16_t fetch()
{
	uint16_t opcode = *(uint16_t *)&mem[pc];
	pc += 0x2;
	return opcode;
}

void exec(uint16_t opcode)
{
	opcode = (opcode >> 8) | (opcode << 8); // little-endian to big-endian

	uint8_t nibble = (opcode >> 12);

	if (opcode == 0xEE) {
		pc = mem[--sp] << 8;
		pc |= mem[--sp];
	} else if (opcode == 0xE0) {
		memset(framebuffer, 0x0, 2048 * 2);
	} else if (nibble == 0x0) {
		// return std::format("0NNN");
	}

	if (nibble == 0x1) {
		pc = (opcode & 0xFFF);
	} else if (nibble == 0x2) {
		mem[sp++] = pc & 0xFF;
		mem[sp++] = (pc & 0xFF00) >> 8;
		pc = (opcode & 0xFFF);
	} else if (nibble == 0xa) {
		i = (opcode & 0xFFF);
	} else if (nibble == 0xb) {
		pc = v[0x0] + (opcode & 0xFFF);
	}

	if (nibble == 0x6) {
		v[(opcode & 0xF00) >> 8] = (opcode & 0xFF);
	} else if (nibble == 0x7) {
		v[(opcode & 0xF00) >> 8] += (opcode & 0xFF);
	} else if (nibble == 0xc) {
		v[(opcode & 0xF00) >> 8] = (rand() % 256) & (opcode & 0xFF);
	}

	if (nibble == 0x8) {
		if ((opcode & 0xF) == 0x0) {
			v[(opcode & 0xF00) >> 8] = v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x1) {
			v[(opcode & 0xF00) >> 8] = v[(opcode & 0xF00) >> 8] | v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x2) {
			v[(opcode & 0xF00) >> 8] = v[(opcode & 0xF00) >> 8] & v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x3) {
			v[(opcode & 0xF00) >> 8] = v[(opcode & 0xF00) >> 8] ^ v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x4) {
			if ((v[(opcode & 0xF00) >> 8] + v[(opcode & 0xF0) >> 4]) > 0xFF) {
				v[0xF] = 0x1;
			} else {
				v[0xF] = 0x0;
			}
			v[(opcode & 0xF00) >> 8] += v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x5) {
			if (v[(opcode & 0xF00) >> 8] > v[(opcode & 0xF0) >> 4]) {
				v[0xF] = 0x1;
			} else {
				v[0xF] = 0x0;
			}
			v[(opcode & 0xF00) >> 8] -= v[(opcode & 0xF0) >> 4];
		} else if ((opcode & 0xF) == 0x6) {
			v[0xF] = v[(opcode & 0xF00) >> 8] & 0x1;
			v[(opcode & 0xF00) >> 8] >>= 1;
		} else if ((opcode & 0xF) == 0x7) {
			if (v[(opcode & 0xF00) >> 8] < v[(opcode & 0xF0) >> 4]) {
				v[0xF] = 0x1;
			} else {
				v[0xF] = 0x0;
			}
			v[(opcode & 0xF00) >> 8] = v[(opcode & 0xF0) >> 4] - v[(opcode & 0xF00) >> 8];
		} else if ((opcode & 0xF) == 0xE) {
			v[0xF] = (v[(opcode & 0xF00) >> 8] & 0x80) >> 7;
			v[(opcode & 0xF00) >> 8] <<= 1;
		}
	}

	if (nibble == 0xe) {
		if ((opcode & 0xFF) == 0x9e) {
			if (keypad[v[(opcode & 0xF00) >> 8]] == 0xFF) {
				pc += 0x2;
			}
		} else if ((opcode & 0xFF) == 0xa1) {
			if (keypad[v[(opcode & 0xF00) >> 8]] != 0xFF) {
				pc += 0x2;
			}
		}
	}

	if (nibble == 0xf) {
		if ((opcode & 0xF) == 0xa) {
			if (keypad[0x0] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x0;
			} else if (keypad[0x1] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x1;
			} else if (keypad[0x2] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x2;
			} else if (keypad[0x3] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x3;
			} else if (keypad[0x4] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x4;
			} else if (keypad[0x5] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x5;
			} else if (keypad[0x6] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x6;
			} else if (keypad[0x7] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x7;
			} else if (keypad[0x8] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x8;
			} else if (keypad[0x9] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0x9;
			} else if (keypad[0xA] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xA;
			} else if (keypad[0xB] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xB;
			} else if (keypad[0xC] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xC;
			} else if (keypad[0xD] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xD;
			} else if (keypad[0xE] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xE;
			} else if (keypad[0xF] == 0xFF) {
				v[(opcode & 0xF00) >> 8] = 0xF;
			} else {
				pc -= 0x2;
			}
		} else if ((opcode & 0xFF) == 0x1e) {
			i += v[(opcode & 0xF00) >> 8];
		} else if ((opcode & 0xFF) == 0x7) {
			v[(opcode & 0xF00) >> 8] = delay;
		} else if ((opcode & 0xFF) == 0x15) {
			delay = v[(opcode & 0xF00) >> 8];
		} else if ((opcode & 0xFF) == 0x18) {
			sound = v[(opcode & 0xF00) >> 8];
		} else if ((opcode & 0xFF) == 0x29) {
			i = 0xE50 + v[(opcode & 0xF00) >> 8] * 5;
		} else if ((opcode & 0xFF) == 0x33) {
			uint8_t bcd = v[(opcode & 0xF00) >> 8];
			mem[i + 0x2] = bcd % 10;
			bcd /= 10;
			mem[i + 0x1] = bcd % 10;
			bcd /= 10;
			mem[i + 0x0] = bcd % 10;
		} else if ((opcode & 0xFF) == 0x55) {
			for (size_t j = 0; j <= ((opcode & 0xF00) >> 8); j++) {
				mem[i + j] = v[j];
			}
		} else if ((opcode & 0xFF) == 0x65) {
			for (size_t j = 0; j <= ((opcode & 0xF00) >> 8); j++) {
				v[j] = mem[i + j];
			}
		}
	}

	if (nibble == 0x3) {
		if (v[(opcode & 0xF00) >> 8] == (opcode & 0xFF)) {
			pc += 0x2;
		}
	} else if (nibble == 0x4) {
		if (v[(opcode & 0xF00) >> 8] != (opcode & 0xFF)) {
			pc += 0x2;
		}
	} else if (nibble == 0x5) {
		if (v[(opcode & 0xF00) >> 8] == v[(opcode & 0xF0) >> 4]) {
			pc += 0x2;
		}
	} else if (nibble == 0x9) {
		if (v[(opcode & 0xF00) >> 8] != v[(opcode & 0xF0) >> 4]) {
			pc += 0x2;
		}
	}

	if (nibble == 0xd) {
		size_t offset = 0;
		v[0xF] = 0x0;
		for (size_t j = 0; j < (opcode & 0xF); j++) {
			if (mem[i + offset] & 0x80) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 0) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 0) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x40) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 1) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 1) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x20) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 2) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 2) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x10) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 3) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 3) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x08) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 4) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 4) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x04) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 5) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 5) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x02) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 6) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 6) % 2048] ^= 0xBD8D;
			}
			if (mem[i + offset] & 0x01) {
				if (framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 7) % 2048] & 0xBD8D) {
					v[0xF] = 0x1;
				}
				framebuffer[((v[(opcode & 0xF0) >> 4] + j) * 64 + v[(opcode & 0xF00) >> 8] + 7) % 2048] ^= 0xBD8D;
			}
			offset++;
		}
	}
}

std::string disassemble(uint16_t opcode)
{
	opcode = (opcode >> 8) | (opcode << 8); // little-endian to big-endian

	std::cout << std::format("{:0>2x} {:0>2x} ", (opcode >> 8), (opcode & 0xFF));

	uint8_t nibble = opcode >> 12;

	if (opcode == 0xEE) {
		return std::format("ret");
	} else if (opcode == 0xE0) {
		return std::format("clear");
	} else if (nibble == 0x0) {
		return std::format("0NNN");
	}

	if (nibble == 0x8) {
		if ((opcode & 0xF) == 0x0) {
			return std::format("mov v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x1) {
			return std::format("and v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x2) {
			return std::format("or v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x3) {
			return std::format("xor v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x4) {
			return std::format("add v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x5) {
			return std::format("sub v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
		} else if ((opcode & 0xF) == 0x6) {
			return std::format("shr v{:x}", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xF) == 0x7) {
			return std::format("sub v{:x} v{:x}\n	neg v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4, (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xF) == 0xE) {
			return std::format("shl v{:x}", (opcode & 0xF00) >> 8);
		}
	}

	if (nibble == 0xe) {
		if ((opcode & 0xFF) == 0x9e) {
			return std::format("keq");
		} else if ((opcode & 0xFF) == 0xa1) {
			return std::format("knq");
		}
	}

	if (nibble == 0xf) {
		if ((opcode & 0xF) == 0xa) {
			return std::format("waitk");
		} else if ((opcode & 0xFF) == 0x1e) {
			return std::format("add i v{:x}", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x7) {
			return std::format("mov v{:x} delay", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x15) {
			return std::format("mov delay v{:x}", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x18) {
			return std::format("mov sound v{:x}", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x29) {
			return std::format("mov i, font[v{:x}]", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x33) {
			return std::format("movbcd v{:x}", (opcode & 0xF00) >> 8);
		} else if ((opcode & 0xFF) == 0x55) {
			return std::format("regdump");
		} else if ((opcode & 0xFF) == 0x65) {
			return std::format("regload");
		}
	}

	switch (nibble) {
	case 0x1:
		return std::format("jmp ${:x}", opcode & 0xFFF);
	case 0x2:
		return std::format("call ${:x}", opcode & 0xFFF);
	case 0x3:
		return std::format("beq v{:x} #{:x}", (opcode & 0xF00) >> 8, opcode & 0xFF);
	case 0x4:
		return std::format("bnq v{:x} #{:x}", (opcode & 0xF00) >> 8, opcode & 0xFF);
	case 0x5:
		return std::format("beq v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
	case 0x6:
		return std::format("mov v{:x} #{:x}", (opcode & 0xF00) >> 8, opcode & 0xFF);
	case 0x7:
		return std::format("add v{:x} #{:x}", (opcode & 0xF00) >> 8, opcode & 0xFF);
	case 0x9:
		return std::format("bnq v{:x} v{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4);
	case 0xa:
		return std::format("mov i ${:x}", opcode & 0xFFF);
	case 0xb:
		return std::format("add v0 #{:x}\n	mov pc v0", opcode & 0xFFF);
	case 0xc:
		return std::format("rand v{:x} #{:x}", (opcode & 0xF00) >> 8, opcode & 0xFF);
	case 0xd:
		return std::format("draw v{:x} v{:x} #{:x}", (opcode & 0xF00) >> 8, (opcode & 0xF0) >> 4, opcode & 0xF);
	default:
		break;
	}

	return "undefined";
}
