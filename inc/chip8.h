#include <SFML/Graphics.hpp>
#include <stack>
#include <string>
#include <vector>

class Memory {
private:
	std::vector<uint8_t> ram;
	std::stack<uint16_t> stack;

public:
	uint8_t delayTimer;
	uint8_t soundTimer;
	std::vector<uint8_t> display;
	Memory();
	void loadRom(std::string fileName);

	friend class CPU;
};

class CPU {
private:
	uint16_t pc;
	uint16_t iReg;
	std::vector<uint8_t> v;
	void decode0(Memory& mem, uint16_t nnn);
	void decode8(uint16_t x, uint16_t y, uint16_t n);
	void drawGraphics(Memory& mem, uint16_t x, uint16_t y, uint16_t n);
	void decodeE(uint16_t x, uint16_t nn);
	void decodeF(Memory& mem, uint16_t x, uint16_t nn);

public:
	static inline constexpr uint32_t cyclePeriod = 16666667;
	static inline constexpr uint8_t instructionsPerCycle = 8;
	int8_t lastKeyReleased;
	std::vector<bool> keyState;
	CPU();
	uint16_t fetch(Memory& mem);
	void decodeAndExecute(Memory& mem, sf::RenderWindow& window, uint16_t opcode);
};