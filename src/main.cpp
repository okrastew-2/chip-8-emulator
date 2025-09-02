#include <chrono>
#include <fstream>
#include <iostream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <vector>
#include "chip8.h"

Memory mem;
CPU cpu;

std::vector<bool> previousKeyState(16, false);
std::vector<bool> currentKeyState(16, false);

sf::Sound initializeBeep(sf::SoundBuffer& beepBuffer) {
    constexpr uint32_t samples = 44100;
    constexpr uint32_t sampleRate = 44100;
    constexpr uint32_t amplitude = 32767;
    constexpr double period = 44100.0 / 440.0;

    std::vector<int16_t> raw(samples);

    for (size_t i = 0; i < samples; ++i) {
        int t = static_cast<int>(std::floor(i / (period / 2.0f))) % 2;
        raw[i] = t ? 0 : amplitude;
    }

    if (!beepBuffer.loadFromSamples(raw.data(), raw.size(), 1, sampleRate, { sf::SoundChannel::Mono })) {
        std::cerr << "Loading sound failed!" << "\r\n";
    }

    sf::Sound beep(beepBuffer);

    return beep;
}

void updateKeyState(sf::Keyboard::Scancode key, bool state) {
    constexpr std::array<sf::Keyboard::Scancode, 16> keypad = {
        sf::Keyboard::Scancode::X,
        sf::Keyboard::Scancode::Num1,
        sf::Keyboard::Scancode::Num2,
        sf::Keyboard::Scancode::Num3,
        sf::Keyboard::Scancode::Q,
        sf::Keyboard::Scancode::W,
        sf::Keyboard::Scancode::E,
        sf::Keyboard::Scancode::A,
        sf::Keyboard::Scancode::S,
        sf::Keyboard::Scancode::D,
        sf::Keyboard::Scancode::Z,
        sf::Keyboard::Scancode::C,
        sf::Keyboard::Scancode::Num4,
        sf::Keyboard::Scancode::R,
        sf::Keyboard::Scancode::F,
        sf::Keyboard::Scancode::V
    };

    for (uint8_t i = 0; i < 16; ++i) {
        if (key == keypad[i]) {
            currentKeyState[i] = state;
            if (!state) cpu.lastKeyReleased = i;
            break;
        }
    }
}

void processInputs() {
    for (uint8_t i = 0; i < 16; ++i) {
        if (currentKeyState[i] != previousKeyState[i]) {
            cpu.keyState[i] = currentKeyState[i];
        }
        previousKeyState[i] = currentKeyState[i];
    }
}

void getTimeDelta(std::chrono::steady_clock::time_point& currTime, std::chrono::steady_clock::time_point& lastTime, double& accumulator) {
    currTime = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(currTime - lastTime).count();
    lastTime = currTime;
    accumulator += delta;
}

void decrementTimers() {
    if (mem.delayTimer > 0) --mem.delayTimer;
    if (mem.soundTimer > 0) --mem.soundTimer;
}

void renderDisplay(Memory& mem, sf::RenderWindow& window) {
    sf::RectangleShape on({ 20.f, 20.f });
    window.clear();

    for (uint8_t i = 0; i < 32; ++i) {
        for (uint8_t j = 0; j < 64; ++j) {
            if (mem.display[i * 64 + j]) {
                on.setPosition({ j * 20.f, i * 20.f });
                window.draw(on);
            }
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 640 }), "CHIP-8");
    window.setVerticalSyncEnabled(true);

    mem.loadRom("C:\\Users\\omara\\source\\repos\\CHIP-8\\tests\\PONG2.ch8");

    auto currTime = std::chrono::steady_clock::now();
    auto lastTime = currTime;
    double accumulator = 0.0;

    sf::SoundBuffer beepBuffer{};
    sf::Sound beep = initializeBeep(beepBuffer);
    beep.setLooping(true);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    window.close();
                }
                else {
                    updateKeyState(keyPressed->scancode, true);
                }
            }

            else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                updateKeyState(keyReleased->scancode, false);
            }
        }

        processInputs();

        getTimeDelta(currTime, lastTime, accumulator);

        while (accumulator >= cpu.cyclePeriod) {
            decrementTimers();

            for (uint8_t i = 0; i < cpu.instructionsPerCycle; ++i) {
                uint16_t opcode = cpu.fetch(mem);
                cpu.decodeAndExecute(mem, window, opcode);
            }

            accumulator -= cpu.cyclePeriod;
        }

        renderDisplay(mem, window);
        window.display();

        if (mem.soundTimer > 0 && beep.getStatus() == sf::Sound::Status::Stopped) beep.play();
        else if (mem.soundTimer == 0 && beep.getStatus() == sf::Sound::Status::Playing) beep.stop();

        cpu.lastKeyReleased = -1;
    }

    return 0;
}
