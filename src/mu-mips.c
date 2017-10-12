#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
				(MEM_REGIONS[i].mem[offset+2] << 16) |
				(MEM_REGIONS[i].mem[offset+1] <<  8) |
				(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */

	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	uint32_t opcode, function, rd, rt;

	opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	function = MEM_WB.IR & 0x0000003F;
	rt = (MEM_WB.IR & 0x001F0000) >> 16;
	rd = (MEM_WB.IR & 0x0000F800) >> 11;

	if(opcode == 0x00 && MEM_WB.IR != 0){
		switch(function){
			case 0x00: //SLL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //SYSCALL
				if(MEM_WB.ALUOutput == 0xa){
					RUN_FLAG = FALSE;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				NEXT_STATE.LO = (MEM_WB.AA & 0x00000000FFFFFFFF);
				NEXT_STATE.HI = (MEM_WB.AA & 0XFFFFFFFF00000000) >> 32;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				NEXT_STATE.LO = (MEM_WB.AA & 0x00000000FFFFFFFF);
				NEXT_STATE.HI = (MEM_WB.AA & 0XFFFFFFFF00000000) >> 32;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				NEXT_STATE.HI = MEM_WB.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				NEXT_STATE.HI = MEM_WB.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				INSTRUCTION_COUNT--;
				break;
		}
	}
	else{
		switch(opcode){
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				NEXT_STATE.REGS[rt] = ((MEM_WB.LMD & 0x000000FF) & 0x80) > 0 ? (MEM_WB.LMD | 0xFFFFFF00) : (MEM_WB.LMD & 0x000000FF);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //LH
				NEXT_STATE.REGS[rt] = ((MEM_WB.LMD & 0x0000FFFF) & 0x8000) > 0 ? (MEM_WB.LMD | 0xFFFF0000) : (MEM_WB.LMD & 0x0000FFFF);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //LW
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				//for count the instruction
				//print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				//for count the instruction
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				//for count the instruction
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				INSTRUCTION_COUNT--;
				break;
		}
	}
	INSTRUCTION_COUNT++;
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
	MEM_WB.IR = EX_MEM.IR;

	uint32_t opcode, function, data;

	opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	function = MEM_WB.IR & 0x0000003F;

	if(opcode == 0x00 && MEM_WB.IR != 0){
		switch(function){
			case 0x00: //SLL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				// print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //SYSCALL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x10: //MFHI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				MEM_WB.AA = EX_MEM.AA;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				MEM_WB.AA = EX_MEM.AA;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				MEM_WB.A = EX_MEM.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				MEM_WB.A = EX_MEM.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("MEM at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x08: //ADDI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //LH
				MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //LW
				MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				data = mem_read_32( EX_MEM.ALUOutput);
				data = (data & 0xFFFFFF00) | (EX_MEM.B & 0x000000FF);
				mem_write_32(EX_MEM.ALUOutput, data);
				//print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				data = mem_read_32( EX_MEM.ALUOutput);
				data = (data & 0xFFFF0000) | (EX_MEM.B & 0x0000FFFF);
				mem_write_32(EX_MEM.ALUOutput, data);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("MEM at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
	EX_MEM.IR = ID_EX.IR;

	uint32_t opcode, function;
	uint64_t p1, p2;

	opcode = (EX_MEM.IR & 0xFC000000) >> 26;
	function = EX_MEM.IR & 0x0000003F;

	if(opcode == 0x00 && EX_MEM.IR != 0){
		switch(function){
			case 0x00: //SLL
				EX_MEM.ALUOutput = ID_EX.A << ID_EX.imm;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				EX_MEM.ALUOutput = ID_EX.A >> ID_EX.imm;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				if ((ID_EX.A & 0x80000000) == 1){
					EX_MEM.ALUOutput = ~(~ID_EX.A >> ID_EX.imm);
				}
				else{
					EX_MEM.ALUOutput = ID_EX.A >> ID_EX.imm;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //SYSCALL
				EX_MEM.ALUOutput = ID_EX.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x10: //MFHI
				EX_MEM.ALUOutput = ID_EX.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				EX_MEM.ALUOutput = ID_EX.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				EX_MEM.ALUOutput = ID_EX.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				EX_MEM.ALUOutput = ID_EX.A;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				if ((ID_EX.A & 0x80000000) == 0x80000000){
					p1 = 0xFFFFFFFF00000000 | ID_EX.A;
				}else{
					p1 = 0x00000000FFFFFFFF & ID_EX.A;
				}
				if ((ID_EX.B & 0x80000000) == 0x80000000){
					p2 = 0xFFFFFFFF00000000 | ID_EX.B;
				}else{
					p2 = 0x00000000FFFFFFFF & ID_EX.B;
				}
				EX_MEM.AA = p1 * p2;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				EX_MEM.AA = ID_EX.A * ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				if (ID_EX.B != 0){
					EX_MEM.ALUOutput = ID_EX.A / ID_EX.B;
					EX_MEM.A = ID_EX.A % ID_EX.B;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				if (ID_EX.B != 0){
					EX_MEM.ALUOutput = ID_EX.A / ID_EX.B;
					EX_MEM.A = ID_EX.A % ID_EX.B;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				EX_MEM.ALUOutput = ID_EX.A - ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				EX_MEM.ALUOutput = ID_EX.A - ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				EX_MEM.ALUOutput = ID_EX.A & ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				EX_MEM.ALUOutput = ID_EX.A | ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				EX_MEM.ALUOutput = ID_EX.A ^ ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				EX_MEM.ALUOutput = ~(ID_EX.A | ID_EX.B);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				if(ID_EX.A < ID_EX.B){
					EX_MEM.ALUOutput = 0x1;
				}
				else{
					EX_MEM.ALUOutput = 0x0;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("EX at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x08: //ADDI
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				if ( (  ID_EX.A - (int32_t)( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF))) < 0){
					EX_MEM.ALUOutput = 0x1;
				}else{
					EX_MEM.ALUOutput = 0x0;
				}
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				EX_MEM.ALUOutput = ID_EX.A & (ID_EX.imm & 0x0000FFFF);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				EX_MEM.ALUOutput = ID_EX.A | (ID_EX.imm & 0x0000FFFF);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				EX_MEM.ALUOutput = ID_EX.A ^ (ID_EX.imm & 0x0000FFFF);
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				EX_MEM.ALUOutput = ID_EX.imm << 16;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //LH
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //LW
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				EX_MEM.B = ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				EX_MEM.B = ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				EX_MEM.B = ID_EX.B;
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("EX at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*IMPLEMENT THIS*/
	ID_EX.IR = IF_ID.IR;

	uint32_t opcode, function, rs, rt, rd, sa, immediate;

	//printf("[0x%x]\t", CURRENT_STATE.PC);

	opcode = (ID_EX.IR & 0xFC000000) >> 26;
	function = ID_EX.IR & 0x0000003F;
	rs = (ID_EX.IR & 0x03E00000) >> 21;
	rt = (ID_EX.IR & 0x001F0000) >> 16;
	rd = (ID_EX.IR & 0x0000F800) >> 11;
	sa = (ID_EX.IR & 0x000007C0) >> 6;
	immediate = ID_EX.IR & 0x0000FFFF;	
	ID_EX.RegisterRs = rs;
	ID_EX.RegisterRt = rt;
	ID_EX.RegisterRd = 0;	// Set Rd conditionlly based on instr.
	ID_EX.RegWrite = 0;	// RegWrite=1 when Rd != 0

		if(opcode == 0x00 && ID_EX.IR != 0){
			switch(function){
				case 0x00: //SLL
					ID_EX.A = CURRENT_STATE.REGS[rt];
					ID_EX.imm = sa;
					ID_EX.RegisterRs = 0;	// Not used 
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x02: //SRL
					ID_EX.A = CURRENT_STATE.REGS[rt];
					ID_EX.imm = sa;
					ID_EX.RegisterRs = 0;	// Not used 
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x03: //SRA 
					ID_EX.A = CURRENT_STATE.REGS[rt];
					ID_EX.imm = sa;
					ID_EX.RegisterRs = 0;	// Not used 
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0C: //SYSCALL
					ID_EX.A = CURRENT_STATE.REGS[2];
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x10: //MFHI
					ID_EX.A = CURRENT_STATE.HI;
					ID_EX.RegisterRd = rd;
					ID_EX.RegisterRt = 0;	// Not used
					ID_EX.RegisterRs = 0;	// Not used
					ID_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x11: //MTHI
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.RegisterRt = 0;	// Not used
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x12: //MFLO
					ID_EX.A = CURRENT_STATE.LO;
					ID_EX.RegisterRd = rd;
					ID_EX.RegisterRt = 0;	// Not used
					ID_EX.RegisterRs = 0;	// Not used
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x13: //MTLO
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.RegisterRt = 0;	// Not used
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x18: //MULT
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x19: //MULTU
					ID_EX.AA = (int64_t)CURRENT_STATE.REGS[rs];
					ID_EX.BB = (int64_t)CURRENT_STATE.REGS[rt];
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1A: //DIV 
					ID_EX.A = (int32_t)CURRENT_STATE.REGS[rs];
					ID_EX.B = (int32_t)CURRENT_STATE.REGS[rt];
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1B: //DIVU
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //ADD
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //ADDU 
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x22: //SUB
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //SUBU
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x24: //AND
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x25: //OR
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x26: //XOR
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x27: //NOR
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2A: //SLT
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.RegisterRd = rd;
					ID_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					printf("ID at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
		else{
			switch(opcode){
				case 0x08: //ADDI
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x09: //ADDIU
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0A: //SLTI
					ID_EX.A = (int32_t)CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0C: //ANDI
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0D: //ORI
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0E: //XORI
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0F: //LUI
					ID_EX.imm = immediate;
					ID_EX.RegisterRs = 0;	// Not used
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //LB
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //LH
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //LW
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x28: //SB
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.imm = immediate;	
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x29: //SH
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2B: //SW
					ID_EX.A = CURRENT_STATE.REGS[rs];
					ID_EX.B = CURRENT_STATE.REGS[rt];
					ID_EX.imm = immediate;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					// put more things here
					printf("ID at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	/*IMPLEMENT THIS*/
	IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
	IF_ID.PC = CURRENT_STATE.PC + 4;
	NEXT_STATE.PC = IF_ID.PC;
	if (IF_ID.IR == 0){
		printf("NO INSTRUCTIONS FOR IF.\n");
	}
	else{
		print_instruction(CURRENT_STATE.PC);
	}
	//show_pipeline();
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;

	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;

	instruction = mem_read_32(addr);

	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;

	if(opcode == 0x00){
		/*R format instructions here*/

		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	printf("\nCurrent PC:[0x%x]\n", CURRENT_STATE.PC);
	printf("IF_ID.IR:%u\n", IF_ID.IR);
	print_instruction(IF_ID.PC - 4);
	printf("IF_ID.PC:%u\n\n", IF_ID.PC);
	printf("ID_EX.IR:%u\n", ID_EX.IR);
	print_instruction(IF_ID.PC - 8);
	printf("ID_EX.A:%u\n", ID_EX.A);
	printf("ID_EX.B:%u\n", ID_EX.B);
	printf("ID_EX.imm:%u\n\n", ID_EX.imm);
	printf("EX_MEM.IR:%u\n", EX_MEM.IR);
	print_instruction(IF_ID.PC - 12);
	printf("EX_MEM.A:%u\n", EX_MEM.A);
	printf("EX_MEM.B:%u\n", EX_MEM.B);
	printf("EX_MEM.ALUOutput:%u\n\n", EX_MEM.ALUOutput);
	printf("MEM_WB.IR:%u\n", MEM_WB.IR);
	print_instruction(IF_ID.PC - 16);
	printf("MEM_WB.ALUOutput:%u\n", MEM_WB.ALUOutput);
	printf("MEM_WB.LMD:%u\n", MEM_WB.LMD);
	printf("CYCLE %u\n", CYCLE_COUNT);
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
