#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <SDL2/SDL.h>

namespace gfx {
    const auto scr_width = 640;
    const auto scr_height = 480;

    SDL_Window* window;
    SDL_Renderer* renderer;

    bool init()
    {
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "sdl init fail : " << SDL_GetError() << "\n";
            return false;
        }
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

        auto wu = SDL_WINDOWPOS_UNDEFINED;
        auto sw = scr_width;
        auto sh = scr_height;
        window = SDL_CreateWindow("chip 8", wu, wu, sw, sh, SDL_WINDOW_SHOWN);
        if(window == nullptr)
        {
            std::cerr << "create window fail : " << SDL_GetError() << "\n";
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if(renderer == nullptr)
        {
            std::cerr << "create renderer fail : " << SDL_GetError() << "\n";
            return false;
        }
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        return true;
    }

    void close()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

const int key_map[0x10] = {
    SDL_SCANCODE_7,
    SDL_SCANCODE_KP_1,
    SDL_SCANCODE_KP_2,
    SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,
    SDL_SCANCODE_KP_5,
    SDL_SCANCODE_KP_6,
    SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,
    SDL_SCANCODE_KP_9,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6
};

static char fontset[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xf0, 0x10, 0xf0, 0x80, 0xf0, // 2
    0xf0, 0x10, 0xf0, 0x10, 0xf0, // 3
    0x90, 0x90, 0xf0, 0x10, 0x10, // 4
    0xf0, 0x80, 0xf0, 0x10, 0xf0, // 5
    0xf0, 0x80, 0xf0, 0x90, 0xf0, // 6
    0xf0, 0x10, 0x20, 0x40, 0x40, // 7
    0xf0, 0x90, 0xf0, 0x90, 0xf0, // 8
    0xf0, 0x90, 0xf0, 0x10, 0xf0, // 9
    0xf0, 0x90, 0xf0, 0x90, 0x90, // a
    0xe0, 0x90, 0xe0, 0x90, 0xe0, // b
    0xf0, 0x80, 0x80, 0x80, 0xf0, // c
    0xe0, 0x90, 0x90, 0x90, 0xe0, // d
    0xf0, 0x80, 0xf0, 0x80, 0xf0, // e
    0xf0, 0x80, 0xf0, 0x80, 0x80  // f
};

class virtual_machine_c
{
    static const auto stack_max_size = 24u;
    unsigned short stack[stack_max_size];
    unsigned stack_size;
    char memory[0x1000];
    char v_register[0x10];
    unsigned short i_register;
    char delay_timer;
    char sound_timer;
    int program_counter;
    unsigned key_register_idx;
    enum class state_c { quitting, waiting_for_key, running };
    state_c state;
    unsigned long last_time;
    unsigned long last_draw_time;

public:
    void init() {
        program_counter = 0x200;
        delay_timer = 0;
        sound_timer = 0;
        stack_size = 0;
        std::fill(memory, memory + 0x1000, 0);
        std::fill(v_register, v_register + 0x10, 0);
        std::copy(fontset, fontset + 0x50, &memory[0x050]);
        state = state_c::running;
        last_time = 0;
        last_draw_time = 0;
    }

    void load_file(const std::string& file) {
        std::ifstream input(file, std::ios::binary);
        if (input.good()) {
            input.read(&memory[program_counter], 0xea0 - program_counter);
        } else {
            std::cout << "failed to load file <" << file << ">\n";
        }
    }

    void execute() {
        auto& pc = program_counter;
        if (pc >= 0xfff || pc < 0) {
            state = state_c::quitting;
            return;
        }

        auto wx = memory[pc];
        auto yz = memory[pc + 1];
        auto w = wx >> 4;
        auto x = wx & 0xfu;
        auto y = yz >> 4;
        auto z = yz & 0xfu;
        auto& vx = v_register[x];
        auto& vy = v_register[y];
        auto& vf = v_register[0xf];
        auto& v0 = v_register[0];
        auto& ri = i_register;

        unsigned short nnn = x;
        nnn = (nnn << 8) | yz;

        pc += 2;

        if (w == 0) {
            if (x == 0) {
                if (yz == 0xe0) {
                    std::fill(memory + 0xf00, memory + 0x1000, 0);
                } else if (yz == 0xee) {
                    if (stack_size > 0) {
                        pc = stack[stack_size - 1];
                        stack_size--;
                    } else {
                        state = state_c::quitting;
                    }
                }
            }
        } else if (w == 1) {
            pc = nnn;
        } else if (w == 2) {
            if (stack_size < stack_max_size) {
                stack[stack_size] = pc;
                stack_size++;
                pc = nnn;
            }
        } else if (w == 3) {
            if (vx == yz) {
                pc += 2;
            }
        } else if (w == 4) {
            if (vx != yz) {
                pc += 2;
            }
        } else if (w == 5) {
            if (vx == vy) {
                pc += 2;
            }
        } else if (w == 6) {
            vx = yz;
        } else if (w == 7) {
            vx += yz;
        } else if (w == 8) {
            unsigned short h;
            if (z == 0) {
                vx = vy;
            } else if (z == 1) {
                vx |= vy;
            } else if (z == 2) {
                vx &= vy;
            } else if (z == 3) {
                vx ^= vy;
            } else if (z == 4) {
                h = vx + vy;
                vf = (h >= 256);
                vx = h;
            } else if (z == 5) {
                vf = (vx >= vy);
                vx -= vy;
            } else if (z == 6) {
                vf = vx & 1;
                vx >>= 1;
            } else if (z == 7) {
                vf = (vy >= vx);
                vx = vy - vx;
            } else if (z == 0xe) {
                vf = vx >> 7;
                vx <<= 1;
            }
        } else if (w == 9) {
            if (vx != vy) {
                pc += 2;
            }
        } else if (w == 0xa) {
            ri = nnn;
        } else if (w == 0xb) {
            pc = v0 + nnn;
        } else if (w == 0xc) {
            vx = rand() % 256;
            vx &= yz;
        } else if (w == 0xd) {
            vf = 0;
            for (char i = 0; i < z; i++) {
                auto r = vx % 8;
                auto& b = memory[ri + i];
                if (r == 0) {
                    auto& a = memory[0xf00 + (vy + i) * 8 + (vx / 8)];
                    if ((a & b) != 0) {
                        vf = 1;
                    }
                    a ^= b;
                } else {
                    auto c = b >> r;
                    auto d = b << (8 - r);
                    auto j = 0xf00 + (vy + i) * 8 + (vx / 8);
                    auto& a0 = memory[j];
                    auto& a1 = memory[j + 1];
                    if ((a0 & c) != 0 || (a1 & d) != 0) {
                        vf = 1;
                    }
                    a0 ^= c, a1 ^= d;
                }
            }
        } else if (w == 0xe) {
            if (yz == 0x9e) {
                if (is_key_pressed(vx)) {
                    pc += 2;
                }
            } else if (yz == 0xa1) {
                if (!is_key_pressed(vx)) {
                    pc += 2;
                }
            }
        } else if (w == 0xf) {
            if (yz == 0x07) {
                vx = delay_timer;
            } else if (yz == 0x0a) {
                key_register_idx = x;
                state = state_c::waiting_for_key;
            } else if (yz == 0x15) {
                delay_timer = vx;
            } else if (yz == 0x18) {
                sound_timer = vx;
            } else if (yz == 0x1e) {
                ri += vx;
            } else if (yz == 0x29) {
                ri = 0x050 + 5 * vx;
            } else if (yz == 0x33) {
                auto u = vx;
                memory[ri + 2] = u % 10;
                u /= 10;
                memory[ri + 1] = u % 10;
                u /= 10;
                memory[ri] = u;
            } else if (yz == 0x55) {
                for (unsigned i = 0; i <= x; i++) {
                    memory[ri + i] = v_register[i];
                }
            } else if (yz == 0x65) {
                for (unsigned i = 0; i <= x; i++) {
                    v_register[i] = memory[ri + i];
                }
            }
        }
        auto time = SDL_GetTicks();
        if ((time - last_time) * 60 > 1000) {
            if (delay_timer > 0) {
                delay_timer--;
            }
            if (sound_timer > 0) {
                sound_timer--;
            }
            last_time = time;
        }
    }

    void draw() {
        SDL_SetRenderDrawColor(gfx::renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gfx::renderer);
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                auto idx = 0xf00 + y * 8 + (x / 8);
                auto gr = gfx::renderer;
                if (memory[idx] & (1u << (7 - (x % 8)))) {
                    SDL_SetRenderDrawColor(gr, 0x00, 0xff, 0x00, 0xff);
                } else {
                    SDL_SetRenderDrawColor(gr, 0x00, 0x00, 0xff, 0xff);
                }
                const auto sw = gfx::scr_width;
                const auto sh = gfx::scr_height;
                SDL_Rect rect = { sw * x / 64, sh * y / 32, sw / 64, sh / 32 };
                SDL_RenderFillRect(gfx::renderer, &rect);
            }
        }
        SDL_RenderPresent(gfx::renderer);
    }

    bool
    is_key_pressed(int key)
    {
        auto ks = SDL_GetKeyboardState(nullptr);
        if (key >= 0 && key < 0x10) {
            return ks[key_map[key]];
        }
        return false;
    }

    void run() {
        while (true) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) {
                    state = state_c::quitting;
                }
                if (state == state_c::waiting_for_key) {
                    if (event.type == SDL_KEYDOWN) {
                        auto sc = event.key.keysym.scancode;
                        for (int key = 0; key < 0x10; key++) {
                            if (key_map[key] == sc) {
                                v_register[key_register_idx] = key;
                                state = state_c::running;
                            }
                        }
                    }
                }
            }
            if (state == state_c::waiting_for_key) {
                continue;
            }
            if (state == state_c::quitting) {
                break;
            }

            execute();
            draw();

            while (true) {
                if (SDL_GetTicks() >= last_draw_time + 1) {
                    break;
                }
            }
            last_draw_time = SDL_GetTicks();
        }
    }
};

int
main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "error : no input file given\n";
        return 1;
    }
    gfx::init();
    virtual_machine_c vm;
    vm.init();
    vm.load_file(argv[1]);
    vm.run();
    gfx::close();
}
