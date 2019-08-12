#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo(int changedReg, int changedMem);
unsigned int Fetch(int);
void Decode(unsigned int, DecodedInstr*, RegVals*);
int Execute(DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction(DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer(FILE* filein, int printingRegisters, int printingMemory,
	int debugging, int interactive) {
	int k;
	unsigned int instr;

	/* Initialize registers and memory */

	for (k = 0; k < 32; k++) {
		mips.registers[k] = 0;
	}

	/* stack pointer - Initialize to highest address of data segment */
	mips.registers[29] = 0x00400000 + (MAXNUMINSTRS + MAXNUMDATA) * 4;

	for (k = 0; k < MAXNUMINSTRS + MAXNUMDATA; k++) {
		mips.memory[k] = 0;
	}

	k = 0;
	while (fread(&instr, 4, 1, filein)) {
		/*swap to big endian, convert to host byte order. Ignore this.*/
		mips.memory[k] = ntohl(endianSwap(instr));
		k++;
		if (k > MAXNUMINSTRS) {
			fprintf(stderr, "Program too big.\n");
			exit(1);
		}
	}

	mips.printingRegisters = printingRegisters;
	mips.printingMemory = printingMemory;
	mips.interactive = interactive;
	mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
	return (i >> 24) | (i >> 8 & 0x0000ff00) | (i << 8 & 0x00ff0000) | (i << 24);
}

/*
 *  Run the simulation.
 */
void Simulate() {
	char s[40];  /* used for handling interactive input */
	unsigned int instr;
	int changedReg = -1, changedMem = -1, val;
	DecodedInstr d;

	/* Initialize the PC to the start of the code section */
	mips.pc = 0x00400000;
	while (1) {
		if (mips.interactive) {
			printf("> ");
			fgets(s, sizeof(s), stdin);
			if (s[0] == 'q') {
				return;
			}
		}

		/* Fetch instr at mips.pc, returning it in instr */
		instr = Fetch(mips.pc);

		printf("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

		/*
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
		Decode(instr, &d, &rVals);
		/*Print decoded instruction*/
		PrintInstruction(&d);

		/*
	 * Perform computation needed to execute d, returning computed value
	 * in val
	 */
		val = Execute(&d, &rVals);
		//printf("val=%d",val);

		UpdatePC(&d, val);

		/*
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem,
	 * otherwise put -1 in *changedMem.
	 * Return any memory value that is read, otherwise return -1.
		 */
		val = Mem(&d, val, &changedMem);

		/*
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
		 * put the index of the modified register in *changedReg,
		 * otherwise put -1 in *changedReg.
		 */
		RegWrite(&d, val, &changedReg);

		PrintInfo(changedReg, changedMem);
	}
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo(int changedReg, int changedMem) {
	int k, addr;
	printf("New pc = %8.8x\n", mips.pc);
	if (!mips.printingRegisters && changedReg == -1) {
		printf("No register was updated.\n");
	}
	else if (!mips.printingRegisters) {
		printf("Updated r%2.2d to %8.8x\n",
			changedReg, mips.registers[changedReg]);
	}
	else {
		for (k = 0; k < 32; k++) {
			printf("r%2.2d: %8.8x  ", k, mips.registers[k]);
			if ((k + 1) % 4 == 0) {
				printf("\n");
			}
		}
	}
	if (!mips.printingMemory && changedMem == -1) {
		printf("No memory location was updated.\n");
	}
	else if (!mips.printingMemory) {
		printf("Updated memory at address %8.8x to %8.8x\n",
			changedMem, Fetch(changedMem));
	}
	else {
		printf("Nonzero memory\n");
		printf("ADDR	  CONTENTS\n");
		for (addr = 0x00400000 + 4 * MAXNUMINSTRS;
			addr < 0x00400000 + 4 * (MAXNUMINSTRS + MAXNUMDATA);
			addr = addr + 4) {
			if (Fetch(addr) != 0) {
				printf("%8.8x  %8.8x\n", addr, Fetch(addr));
			}
		}
	}
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch.
 */
unsigned int Fetch(int addr) {
	return mips.memory[(addr - 0x00400000) / 4];
}

/* Decode instr, returning decoded instruction. */
void Rtype(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = R;
	d->regs.r.rd = (instr >> 11) & 31;
	d->regs.r.rs = (instr >> 21) & 31;
	d->regs.r.rt = (instr >> 16) & 31;
	d->regs.r.shamt = (instr >> 6) & 31;
	d->regs.r.funct = instr & 63;
	rVals->R_rs = mips.registers[d->regs.r.rs];
	rVals->R_rt = mips.registers[d->regs.r.rt];
	rVals->R_rd = mips.registers[d->regs.r.rd];
}

void Itype(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = I;
	unsigned int irs = (instr >> 21) & 31;
	unsigned int irt = (instr >> 16) & 31;
	
	d->regs.i.rs = irs;
	d->regs.i.rt = irt;

	d->regs.i.addr_or_immed = (signed short)(instr & 65535);//this is for signed shorts are 16 bits
	rVals->R_rs = mips.registers[d->regs.i.rs];
	rVals->R_rt = mips.registers[d->regs.i.rt];

}

void Jtype(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = J;
	d->regs.j.target = ((instr & 67108863)<<2)+(mips.pc & 4026531840);//needs upper 4 bit of 32 bit adress
}


void Decode(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	/*
		#R type = Opcode(6) 31 - 26, rs(5) 25 - 21, rt(5) 20 - 16, rd(5) 15 - 11, shamt(5) 10 - 6, funct(6) 5 - 0
		#I type = Opcode(6), 31 - 26 rs(5) 25 - 21, rt(5) 20 - 16, immediate(16) 16 - 0
		#J type = Opcode(6) 31 - 26, address(26) 25 - 0
		#32 - opcode(6) = 26
		*/

		/* Your code goes here */
	d->op = instr >> 26;
	switch (d->op) {
	case 0: {
		//this will  be for r types
		Rtype(instr, d, rVals);
		break;
	}
			//all I types
			//addiu
	case 9: {
		Itype(instr, d, rVals);
		break;
	}
			//andi
	case 12: {
		Itype(instr, d, rVals);
		break;
	}
			 //ori
	case 13: {
		Itype(instr, d, rVals);
		break;
	}
			 //lui
	case 15: {
		Itype(instr, d, rVals);
		break;
	}
			 //beq
	case 4: {
		Itype(instr, d, rVals);
		break;

	}
			//bne
	case 5: {
		Itype(instr, d, rVals);
		break;

	}
			//lw
	case 35: {
		Itype(instr, d, rVals);
		break;

	}
			 //sw
	case 43: {
		Itype(instr, d, rVals);
		break;

	}
			 //Time for j types
	case 2: {
		Jtype(instr, d, rVals);
		break;
	}
	case 3: {
		Jtype(instr, d, rVals);
		break;

	}

	}
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
 // i was having errors when i wanted the case numbers to be in decimal value so i made them into hex
void PrintInstruction(DecodedInstr* d) {
	/* Your code goes here */
	switch (d->op) {
		//this is for all r tyoes
	case 0x0: {
		switch (d->regs.r.funct) {
		case 0x21: {
			printf("addu\t$%u, $%u, $%u \n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
			return;
		}
		case 0x23: {
			printf("subu\t$%u, $%u, $%u \n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
			return;
		}
		case 0x00: {
			printf("sll\t$%u, $%u, $%d \n", d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
			return;
		}
		case 0x02: {
			printf("srl\t$%u, $%u, $%d \n", d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
			return;
		}
		case 0x24: {
			printf("and\t$%u, $%u, $%u \n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
			return;
		}
		case 0x25: {
			printf("or\t$%u, $%u, $%u \n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
			return;
		}
		case 0x2a: {
			printf("slt\t$%u, $%u, $%u \n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
			return;
		}
		case 0x8: {
			printf("jr\t$31\n");
			return;
		}
		default: break;
		}
	}
	case 0X9: {
		printf("addiu\t$%u, $%u, %d \n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
		return;
	}
	case 0xc: {
		printf("andi\t$%u, $%u, 0x%x \n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
		return;
	}
	case 0xd: {
		printf("ori\t$%u, $%u, 0x%x \n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
		return;
	}
	case 0xf: {
		printf("lui\t$%u, 0x%x \n", d->regs.i.rt, d->regs.i.addr_or_immed << 16);//shifts by 16 bits 
		return;
	}
	case 0x4: {
		printf("beq\t$%u, $%u, 0x%8.8x \n", d->regs.i.rs, d->regs.i.rt, (mips.pc + 4) + (d->regs.i.addr_or_immed << 2)); //rember to shift to the left by 2 which is #*4
		return;
	}
	case 0x5: {
		printf("bne\t$%u, $%u, 0x%8.8x \n", d->regs.i.rs, d->regs.i.rt, (mips.pc + 4) + (d->regs.i.addr_or_immed << 2));
		return;
	}
	case 0x2: {
		printf("j\t0x%8.8x \n", d->regs.j.target);
		return;
	}
	case 0x3: {
		printf("jal\t0x%8.8x \n", d->regs.j.target);
		return;
	}
	case 0x23: {
		printf("lw\t$%u, %d($%u)\n", d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
		return;
	}
	case 0x2b: {
		printf("sw\t$%u, %d($%u)\n", d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
		return;
	}
	default: exit(1);
	}

}

/* Perform computation needed to execute d, returning computed value */
int Execute(DecodedInstr* d, RegVals* rVals) {
	/* Your code goes here */
	
	switch (d->op) {
		//r type 
	case 0x0: {
		switch (d->regs.r.funct) {
			//addu
		case 0x21: {
			return (unsigned int)rVals->R_rs + (unsigned int)rVals->R_rt;
		}
				   //subu
		case 0x23: {
			return (unsigned int)rVals->R_rs - (unsigned int)rVals->R_rt;
		}
				   //sll
		case 0x00: {
			return rVals->R_rs << d->regs.r.shamt;
		}
				   //srl
		case 0x02: {
			return rVals->R_rt >> d->regs.r.shamt;
		}
				   //and
		case 0x24: {
			return rVals->R_rs&rVals->R_rt;
		}
				   //or
		case 0x25: {
			return rVals->R_rs | rVals->R_rt;
		}
				   //slt
		case 0x2a: {
			if (rVals->R_rs < rVals->R_rt) {
				return 1;
			}
			else {
				return 0;
			}
		}
				   //jr
		case 0x8: {
			return rVals->R_rs;
		}
		default: return 0;
		}
	}
	
			  //i and j type
			  //addiu
	case 0X9: {
		return rVals->R_rs + d->regs.i.addr_or_immed;
	}
			  //andi
	case 0xc: {
		return rVals->R_rs & d->regs.i.addr_or_immed;
	}
			  //ori
	case 0xd: {
		return rVals->R_rs | d->regs.i.addr_or_immed;
	}
			  //lui
	case 0xf: {
		return d->regs.i.addr_or_immed << 16;
	}
			  // beq
	case 0x4: {
		if (rVals->R_rs == rVals->R_rt) {
			return (mips.pc + 4) + (d->regs.i.addr_or_immed << 2);
		}
		else {
			return 0;
		}
	}
			  //bne
	case 0x5: {
		if (rVals->R_rs != rVals->R_rt) {
			return (mips.pc + 4) + (d->regs.i.addr_or_immed << 2);
		}
		else {
			return 0;
		}
	}
			  //j
	case 0x2: {
		return mips.pc+4;
	}
			  //jal
	case 0x3: {
		return mips.pc + 4;
	}
			  //lw
	case 0x23: {
		return rVals->R_rs + d->regs.i.addr_or_immed;
	}
			   //sw
	case 0x2b: {
		return rVals->R_rs + d->regs.i.addr_or_immed;
	}
	default: return(0);
	}
	
	exit(0);
}


/*
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
	void UpdatePC(DecodedInstr* d, int val) {
		/* Your code goes here */
		//j jal bne beq jr
		switch (d->op) {
			//jr
		case 0x0: {
			if (d->op==0x0&&d->regs.r.funct == 0x8) {
				mips.pc = val;
			}
			else {
				mips.pc+=4;
			}
			break;
		}
				  //j
		case 0x2: {
			mips.pc = d->regs.j.target;
			break;

		}
			//jal
		case 0x3: {
			mips.pc = d->regs.j.target;
			break;
		}
				  //beq
		case 0x4: {
			if (val!=0) {
				mips.pc =val;
			}
			else {
				mips.pc += 4;
			}
			break;
		}
				  //bne
		case 0x5: {
			if (val != 0) {
				mips.pc = val;
			}
			else {
				mips.pc += 4;
			}
			break;
		}
		default: mips.pc += 4;

		}
	}

		/*
		 * Perform memory load or store. Place the address of any updated memory
		 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value
		 * that is read, otherwise return -1.
		 *
		 * Remember that we're mapping MIPS addresses to indices in the mips.memory
		 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1]
		 * with address 0x00400004, and so forth.
		 *
		 */
	// professor helped me with my Mem function
	int Mem(DecodedInstr* d, int val, int *changedMem) {
		/* Your code goes here */
		//bounds check
			switch (d->op) {
			case 0x23: {
				if (val<=0x00401000|| val >= 0x00403fff ) {
					printf("Memory Access Exception at: [0x%08x]: address [0x%08x]\n", mips.pc, val);
					exit(0);
				}
				else {
					val = mips.memory[(val - 0x00400000) / 4];
					return val;
				}
				break;
			}
			case 0x2b: {
				if ( val <= 0x00401000|| val >= 0x00403fff) {
					printf("Memory Access Exception at: [0x%08x]: address [0x%08x]\n", mips.pc, val);
					exit(0);
				}
				else {
					mips.memory[(val - 0x00400000) / 4] = mips.registers[d->regs.i.rt];
					*changedMem = val;
					return *changedMem;
				}
				break;
			}
			default: *changedMem = -1;
			}
			return val;
		
	}

		/*
		 * Write back to register. If the instruction modified a register--
		 * (including jal, which modifies $ra) --
		 * put the index of the modified register in *changedReg,
		 * otherwise put -1 in *changedReg.
		 */
		void RegWrite(DecodedInstr* d, int val, int *changedReg) {
			/* Your code goes here */
			switch (d->op) {
				//rtypes
				case 0x0: {
					if (d->regs.r.funct==0x8) {
						*changedReg = -1;
					}
					else {
						*changedReg = d->regs.r.rd;
						mips.registers[d->regs.r.rd] = val;
					}
					break;
				}
				case 0x9: {
					*changedReg = d->regs.i.rt;
					mips.registers[d->regs.i.rt] = val;
					break;

				}
				case 0xc: {

					*changedReg = d->regs.i.rt;
					mips.registers[d->regs.i.rt] = val;
					break;

				}
				case 0xd: {

					*changedReg = d->regs.i.rt;
					mips.registers[d->regs.i.rt] = val;
					break;

				}
				case 0xf: {
					*changedReg = d->regs.i.rt;
					mips.registers[d->regs.i.rt] = val;
					break;

				}
				case 0x23: {
					*changedReg = d->regs.i.rt;
					mips.registers[d->regs.i.rt] = val;
					break;

				}
				case 0x3: {
					*changedReg = 31;
					mips.registers[31] = val;
					break;
				}
				default: *changedReg = -1;
			}
		}
	
