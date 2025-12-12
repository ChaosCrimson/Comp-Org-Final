#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <sstream>

using namespace std;

const int MEMORY_SIZE = 4096;
const int NUM_REGISTERS = 32;

class CPU {
private:
	uint32_t PC;
	uint32_t IR;
	int32_t registers[NUM_REGISTERS];
	uint8_t memory[MEMORY_SIZE];
	uint64_t clockCycles;
	bool running;
	uint32_t opcode, rs, rt, rd, shamt, funct;
	int16_t immediate;

	string getRegisterName(int reg) {
		switch(reg) {
			case 0: return "$zero";
			case 1: return "$at";
			case 2: return "$v0";
			case 3: return "$v1";
			case 4: return "$a0";
			case 5: return "$a1";
			case 6: return "$a2";
			case 7: return "$a3";
			case 8: return "$t0";
			case 9: return "$t1";
			case 10: return "$t2";
			case 11: return "$t3";
			case 12: return "$t4";
			case 13: return "$t5";
			case 14: return "$t6";
			case 15: return "$t7";
			case 16: return "$s0";
			case 17: return "$s1";
			case 18: return "$s2";
			case 19: return "$s3";
			case 20: return "$s4";
			case 21: return "$s5";
			case 22: return "$s6";
			case 23: return "$s7";
			case 24: return "$t8";
			case 25: return "$t9";
			case 26: return "$k0";
			case 27: return "$k1";
			case 28: return "$gp";
			case 29: return "$sp";
			case 30: return "$fp";
			case 31: return "$ra";
			default: return "$?";
		}
	}

	string getInstructionName() {
		if (opcode == 0x00) {
			if (funct == 0x20) return "ADD";
			if (funct == 0x22) return "SUB";
			return "R-type";
		}
		else if (opcode == 0x08) return "ADDI";
		else if (opcode == 0x04) return "BEQ";
		else if (opcode == 0x2B) return "SW";
		else if (opcode == 0x23) return "LW";
		return "UNKNOWN";
	}

	void printMemory(uint32_t address) {
		cout << "Memory[0x" << hex << setw(4) << setfill('0') << address << "]: ";
		if (address + 3 < MEMORY_SIZE) {
			uint32_t value = (memory[address] << 24) | (memory[address + 1] << 16) |
			                 (memory[address + 2] << 8) | memory[address + 3];
			cout << "0x" << hex << setw(8) << setfill('0') << value << dec;
		} else {
			cout << "OUT OF BOUNDS";
		}
		cout << endl;
	}

public:
	CPU() : PC(0), IR(0), clockCycles(0), running(true) {
		for (int i = 0; i < NUM_REGISTERS; i++) {
			registers[i] = 0;
		}
		for (int i = 0; i < MEMORY_SIZE; i++) {
			memory[i] = 0;
		}
	}

	bool loadProgram(const string& filename) {
		ifstream file(filename);
		if (!file.is_open()) {
			cerr << "Error: Could not open file " << filename << endl;
			return false;
		}

		string line;
		uint32_t address = 0;

		while (getline(file, line)) {
			if (line.empty() || line[0] == '#' || line[0] == ';') {
				continue;
			}

			size_t pos = line.find("0x");
			if (pos != string::npos) {
				line = line.substr(pos + 2);
			}
			pos = line.find("0X");
			if (pos != string::npos) {
				line = line.substr(pos + 2);
			}

			uint32_t instruction;
			stringstream ss;
			ss << hex << line;
			ss >> instruction;

			if (ss.fail()) {
				continue;
			}

			if (address + 3 < MEMORY_SIZE) {
				memory[address]     = (instruction >> 24) & 0xFF;
				memory[address + 1] = (instruction >> 16) & 0xFF;
				memory[address + 2] = (instruction >> 8) & 0xFF;
				memory[address + 3] = instruction & 0xFF;
				address += 4;
			}
		}

		file.close();
		cout << "Program loaded successfully (" << address << " bytes)" << endl;
		return true;
	}

	void fetch() {
		if (PC >= MEMORY_SIZE - 3) {
			running = false;
			return;
		}

		IR = (memory[PC] << 24) | (memory[PC + 1] << 16) |
		     (memory[PC + 2] << 8) | memory[PC + 3];

		PC += 4;
	}

	void decode() {
		opcode = (IR >> 26) & 0x3F;
		rs = (IR >> 21) & 0x1F;
		rt = (IR >> 16) & 0x1F;
		rd = (IR >> 11) & 0x1F;
		shamt = (IR >> 6) & 0x1F;
		funct = IR & 0x3F;
		immediate = IR & 0xFFFF;
	}

	void execute() {
		switch (opcode) {
		case 0x00:
			executeRType();
			break;
		case 0x08:
			executeADDI();
			break;
		case 0x04:
			executeBEQ();
			break;
		case 0x2B:
			executeSW();
			break;
		case 0x23:
			executeLW();
			break;
		default:
			cout << "Unknown opcode: 0x" << hex << opcode << endl;
			running = false;
			break;
		}

		registers[0] = 0;
	}

	void executeRType() {
		switch (funct) {
		case 0x20:
			registers[rd] = registers[rs] + registers[rt];
			cout << "ADD " << getRegisterName(rd) << ", " << getRegisterName(rs) 
			     << ", " << getRegisterName(rt) << endl;
			break;
		case 0x22:
			registers[rd] = registers[rs] - registers[rt];
			cout << "SUB " << getRegisterName(rd) << ", " << getRegisterName(rs) 
			     << ", " << getRegisterName(rt) << endl;
			break;
		default:
			cout << "Unknown R-type funct: 0x" << hex << funct << endl;
			break;
		}
	}

	void executeADDI() {
		int32_t signExtImm = (int16_t)immediate;
		registers[rt] = registers[rs] + signExtImm;
		cout << "ADDI " << getRegisterName(rt) << ", " << getRegisterName(rs) 
		     << ", " << signExtImm << endl;
	}

	void executeBEQ() {
		cout << "BEQ " << getRegisterName(rs) << ", " << getRegisterName(rt);

		if (registers[rs] == registers[rt]) {
			int32_t signExtImm = (int16_t)immediate;
			PC = PC + (signExtImm << 2);
			cout << " -> BRANCH TAKEN to PC = 0x" << hex << PC << dec << endl;
		} else {
			cout << " -> NOT TAKEN" << endl;
		}
	}

	void executeSW() {
		int32_t signExtImm = (int16_t)immediate;
		uint32_t address = registers[rs] + signExtImm;

		if (address + 3 < MEMORY_SIZE) {
			int32_t value = registers[rt];
			memory[address]     = (value >> 24) & 0xFF;
			memory[address + 1] = (value >> 16) & 0xFF;
			memory[address + 2] = (value >> 8) & 0xFF;
			memory[address + 3] = value & 0xFF;

			cout << "SW " << getRegisterName(rt) << ", " << signExtImm 
			     << "(" << getRegisterName(rs) << ")" << endl;
		}
	}

	void executeLW() {
		int32_t signExtImm = (int16_t)immediate;
		uint32_t address = registers[rs] + signExtImm;

		if (address + 3 < MEMORY_SIZE) {
			int32_t value = (memory[address] << 24) | (memory[address + 1] << 16) |
			                (memory[address + 2] << 8) | memory[address + 3];
			registers[rt] = value;

			cout << "LW " << getRegisterName(rt) << ", " << signExtImm 
			     << "(" << getRegisterName(rs) << ")" << endl;
		}
	}

	void run(int maxCycles = 1000) {
		cout << "\n=== Starting CPU Simulation ===" << endl;

		while (running && clockCycles < maxCycles) {
			uint32_t currentPC = PC;
			
			cout << "\n--- Cycle " << clockCycles + 1 << " ---" << endl;
			cout << "PC = 0x" << hex << setw(4) << setfill('0') << PC << dec << endl;

			fetch();
			if (!running) break;

			decode();
			
			cout << "IR = 0x" << hex << setw(8) << setfill('0') << IR << dec 
			     << " (" << getInstructionName() << ")" << endl;
			
			execute();

			clockCycles++;
			
			printRegisters();
			printMemory(currentPC);
		}

		cout << "\n=== Simulation Complete ===" << endl;
		cout << "Total clock cycles: " << clockCycles << endl;
	}

	void printRegisters() {
		cout << "Registers: ";
		bool first = true;
		for (int i = 1; i < NUM_REGISTERS; i++) {
			if (registers[i] != 0) {
				if (!first) cout << " ";
				cout << "[" << getRegisterName(i) << " = " << registers[i] << "]";
				first = false;
			}
		}
		if (first) {
			cout << "[All registers zero]";
		}
		cout << endl;
	}
};

int main() {
	CPU cpu;

	if (!cpu.loadProgram("program.txt")) {
		return 1;
	}
	cpu.run();
	return 0;
}