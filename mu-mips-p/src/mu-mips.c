#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

int stall = 0;
int ForwardA = 0;
int ForwardB = 0;

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
		case 'f':
			if (scanf("%d", &ENABLE_FORWARDING) != 1) {
				break;
			}
			
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
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

	NEXT_STATE = CURRENT_STATE;
	if (stall > 0){
		stall = stall - 1;	//Decrement stall back to 0	
	}
	printf("Handle Pipeline: Stall = %d\n", stall);
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
	//Fifth stage
	
	
	uint32_t opcode = (MEM_WB.IR & 0xFC000000) >> 26;	//Shift left to get opcode bits 26-31
	uint32_t funct = MEM_WB.IR & 0x0000003F;	//Get first 6 bits for function code
	uint32_t rt = (MEM_WB.IR & 0x001F0000) >> 16;
	uint32_t rd = (MEM_WB.IR & 0x0000F800) >> 11;
	    
	if (opcode == 0x00) {	 //R-type instruction
		switch(funct) {
			case 0x00:	//SLL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x02:	//SRL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput; 
				INSTRUCTION_COUNT++;
				break;
				
			case 0x03:	//SRA
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x08:	//JR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x09:	//JALR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0C:	//SYSCALL
				break;
				
			case 0x10:	//MFHI
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;  
				INSTRUCTION_COUNT++;
				break;
				
			case 0x11:	//MTHI
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x12:	//MFLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;	
				INSTRUCTION_COUNT++;
				break;
				
			case 0x13:	//MTLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x18:	//MULT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x19:	//MULTU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x1A:	//DIV
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x1B:	//DIVU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x20:	//ADD
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x21:	//ADDU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x22:	//SUB
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x23:	//SUBU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x24:	//AND
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x25:	//OR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x26:	//XOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x27:	//NOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x2A:	//SLT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			default:
				printf("R-type instruction not handled in wb\n");
				break;
		}
		stall = 0;
	}
	else{
		switch(opcode){
			case 0x01:	//BLTZ OR BGEZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x02:	//J
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x03:	//JAL
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x04:	//BEQ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x05:	//BNE
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x06:	//BLEZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x07:	//BGTZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x08:	//ADDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x09:	//ADDIU
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0A:	//SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0C:	//ANDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0D:	//ORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0E:	//XORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0F:	//LUI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x20:	//LB
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x21:	//LH
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
					
			case 0x23:	//LW
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x28:	//SB
//				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//INSTRUCTION_COUNT++;
				break;
				
			case 0x29:	//SH
//				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				//INSTRUCTION_COUNT++;
				break;
				
			case 0x2B:	//SW
			//	NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
			//	INSTRUCTION_COUNT++;
				break;
				
			default:
                printf("\ninstruction not handled in wb");
				break;
		}
	stall = 0;
	}
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
	//Fourth stage
	//Load/Store only?
	MEM_WB.IR = EX_MEM.IR;
	MEM_WB.PC = EX_MEM.PC;
	MEM_WB.A = EX_MEM.A;
	MEM_WB.B = EX_MEM.B;
	MEM_WB.imm = EX_MEM.imm;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.LMD = 0;
	
	uint32_t opcode;
	
	opcode = (MEM_WB.IR & 0xFC000000) >> 26;	//Shift to get opcode bits 26-31
	
	if (opcode == 0x00){
		return;	//Don't need r type	
	}
	
	else{
		switch(opcode){
			case 0x20:	//LB
				MEM_WB.LMD = 0x000000FF & mem_read_32(MEM_WB.ALUOutput);	//Get first 8 bits from memory and place in lmd
				break;
				
			case 0x21:	//LH
				MEM_WB.LMD = 0x0000FFFF & mem_read_32(MEM_WB.ALUOutput);	//Get first 16 bits from memory and place in lmd
				break;
				
			case 0x23:	//LW
				MEM_WB.LMD = 0xFFFFFFFF & mem_read_32(MEM_WB.ALUOutput);	//Get first 32 bits from memory and place in lmd
				printf("lw mem address = %X\n", MEM_WB.ALUOutput);
                break;
				
			case 0x28:	//SB
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			case 0x29:	//SH
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			case 0x2B:	//SW
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			default:
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
	//Third stage
	//Initialize EX pipeline registers
	
	if (ID_EX.stall == 1){
		printf("Stalled in EX stage\n");
		EX_MEM.stall = 1;
		EX_MEM.IR = 0;
		EX_MEM.PC = 0;
		EX_MEM.A = 0;
		EX_MEM.B = 0;
		EX_MEM.imm = 0;
		EX_MEM.ALUOutput = 0;
		EX_MEM.RegisterRS = 0;
		EX_MEM.RegisterRT = 0;
		EX_MEM.RegisterRD = 0;
		EX_MEM.RegWrite = 0;
	}
	
	if (ID_EX.stall == 0) {
		printf("Running EX stage\n");
		EX_MEM.IR = ID_EX.IR;
		EX_MEM.PC = ID_EX.PC;
		EX_MEM.A = ID_EX.A;
		EX_MEM.B = ID_EX.B;
		EX_MEM.imm = ID_EX.imm;
		EX_MEM.ALUOutput = 0;
		EX_MEM.RegisterRS = ID_EX.RegisterRS;
		EX_MEM.RegisterRT = ID_EX.RegisterRT;
		EX_MEM.RegisterRD = ID_EX.RegisterRD;
		EX_MEM.RegWrite = ID_EX.RegWrite;
		EX_MEM.stall = ID_EX.stall;
		EX_MEM.Mem = ID_EX.Mem;
		
		if (EX_MEM.IR == 0){
			return;	
		}
		
		uint32_t opcode, funct, sa;
		opcode = (EX_MEM.IR & 0xFC000000) >> 26;	//Shift left to get opcode bits 26-31
		funct = EX_MEM.IR & 0x0000003F;	//Get first 6 bits for function code
		sa = (EX_MEM.IR & 0x000007C0) >> 6;	//Get shift amount
		
		if (opcode == 0x00){	//R-type instruction
			switch(funct){
				case 0x20:	//ADD
					EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.B;	//ADD rd(ALUOutput), rs(A), rt(B)
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x24:	//AND
					EX_MEM.ALUOutput = EX_MEM.A & EX_MEM.B;	//AND rd(ALUOutput), rs(A), rt(B)
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x25:	//OR
					EX_MEM.ALUOutput = EX_MEM.A | EX_MEM.B;	//OR rd(ALUOutput), rs(A), rt(B)
					break;
					
				case 0x26:	//XOR	
					EX_MEM.ALUOutput = EX_MEM.A ^ EX_MEM.B;	//XOR rd(ALUOutput), rs(A), rt(B)
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x0C:	//SYSCALL
					if(CURRENT_STATE.REGS[2] == 0xa){
                   				RUN_FLAG = FALSE;
                    			}
                    			print_instruction(CURRENT_STATE.PC-8);
					break; 
			}
		}
		else {	//I/J type
			switch(opcode){
				case 0x09:	//ADDIU
					EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.imm;	//ADDIU rt(aluoutput), rs(A), immediate
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x0E:	//XORI
					EX_MEM.ALUOutput = EX_MEM.A ^ EX_MEM.imm;	//XORI rt(aluotput), rs(A), immediate
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x0F:	//LUI
					EX_MEM.ALUOutput = EX_MEM.imm << 16;	//Shift immediate left 16 bits and place in ALUOutput
					print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x23:	//LW
					EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
					EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
					EX_MEM.B = ID_EX.B;
					EX_MEM.imm = ID_EX.imm;
                			print_instruction(CURRENT_STATE.PC-8);
					break;
					
				case 0x2B:	//SW
					EX_MEM.ALUOutput = ID_EX.A + ((ID_EX.imm & 0x8000)>0?(ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));	//aluoutput = a + immediate
					EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
					EX_MEM.B = ID_EX.B;
					EX_MEM.imm = ID_EX.imm;
                			print_instruction(CURRENT_STATE.PC-8);
					break;
			}
		}
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{	
	if(stall == 0){
		printf("Executing ID stage\n");
		ID_EX.IR = IF_ID.IR;
		ID_EX.PC = IF_ID.PC;
		ID_EX.A = 0;
		ID_EX.B = 0;
		ID_EX.imm = 0;
		ID_EX.RegWrite = 0;
		ID_EX.RegisterRD = 0;
		ID_EX.RegisterRS = 0;
		ID_EX.RegisterRT = 0;
		
		uint32_t opcode, funct, rs, rt, rd, imm, sa;
		
		opcode = (IF_ID.IR & 0xFC000000) >> 26;
		funct = IF_ID.IR & 0x0000003F;
		rs = (IF_ID.IR & 0x03E00000) >> 21;
		rt = (IF_ID.IR & 0x001F0000) >> 16;
		rd = (IF_ID.IR & 0x0000F800) >> 11;
		sa = (IF_ID.IR & 0x000007C0) >> 6;
		imm = IF_ID.IR & 0x0000FFFF;
		
		if(opcode == 0){
			switch(funct){
				case 0x20:	//ADD
					ID_EX.A = NEXT_STATE.REGS[rs];
					ID_EX.B = NEXT_STATE.REGS[rt];
					ID_EX.RegisterRD = rd;
					ID_EX.RegWrite = 1;
					break;
					
				case 0x24:	//AND
					ID_EX.A = NEXT_STATE.REGS[rs];
					ID_EX.B = NEXT_STATE.REGS[rt];
					ID_EX.RegisterRD = rd;
					ID_EX.RegWrite = 1;
					break;
					
				case 0x25:	//OR
					ID_EX.A = NEXT_STATE.REGS[rs];
					ID_EX.B = NEXT_STATE.REGS[rt];
					ID_EX.RegisterRD = rd;
					ID_EX.RegWrite = 1;
					break;
					
				case 0x26:	//XOR	
					ID_EX.A = NEXT_STATE.REGS[rs];
					ID_EX.B = NEXT_STATE.REGS[rt];
					ID_EX.RegisterRD = rd;
					ID_EX.RegWrite = 1;
					break;
					
				case 0x0C:	//SYSCALL
					ID_EX.RegWrite = 1;
					break;
					
				default:
					printf("Instruction not handled in ID stage\n");
			}
		}
		else {	//I or J type
			ID_EX.A = NEXT_STATE.REGS[rs];
			ID_EX.B = NEXT_STATE.REGS[rt];
			ID_EX.imm = imm;
			ID_EX.RegisterRS = rs;
			ID_EX.RegisterRT = rt;
			
			switch (opcode){
				case 0x23:	//LW
					ID_EX.Mem = 1;
					break;
				case 0x2B:	//SW
					ID_EX.RegWrite = 0;
					break;
				default:
					ID_EX.RegWrite = 1;
			}
			
		}
		
	}
	
	else {
		ID_EX.stall = 1;
		IF_ID.IR = ID_EX.IR:
		printf("Stall is needed\n");
		return;
	}
	
	ForwardData();	//Check for data hazard and see if we can forward
	
	if (stall != 0){
		printf("Data Hazard in ID stage\n");
		ID_EX.stall = 1;
	}
	else{
		ID_EX.stall = 0;	
	}
	
	if (ForwardA == 1){
		ID_EX.A = EX_MEM.ALUOutput;
		ForwardA = 0;
	}
	if (ForwardB == 1){
		ID_EX.B = EX_MEM.ALUOutput;
		ForwardB = 0;
	}
	if (ForwardA == 2){
		ID_EX.A = MEM_WB.LMD;
		ForwardA = 0;
	}
	if (ForwardB == 2){
		ID_EX.B = MEM_WB.LMD;
		ForwardB = 0;
	}
	
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{	//something with memread
	/*IMPLEMENT THIS*/
	//First stage
	
	/*while(stall > 0) {
		
	} */
	
	if (stall == 0){	//Fetch instruction if there's no stall
		IF_ID.IR = mem_read_32(CURRENT_STATE.PC);	//Get current value in memory
		IF_ID.PC = CURRENT_STATE.PC + 4;	//Increment counter
		NEXT_STATE.PC = IF_ID.PC;	//Store incremented counter into pc's next state
	}
	else{
		printf("Stalled in IF Stage\n");	
	}
}

// Check for Data Hazard and forward under conditions given to us in lab assignment
void ForwardData()
{
	//Forward from EX stage for A
	if (EX_MEM.RegWrite && (EX_MEM.RegisterRD != 0) && (EX_MEM.RegisterRD == ID_EX.RegisterRS)){
		if (ENABLE_FORWARDING == 1){
			ForwardA = 2;	
		}
		else{
			stall = 2;	
		}
	}
	
	if (EX_MEM.RegWrite && (EX_MEM.RegisterRD != 0) && (EX_MEM.RegisterRD == ID_EX.RegisterRT)){
		if (ENABLE_FORWARDING == 1){
			ForwardB = 2;	
		}
		else{
			stall = 2;	
		}	
	}
	
	if (MEM_WB.RegWrite && (MEM_WB.RegisterRD != 0) && !(EX_MEM.RegWrite && (EX_MEM.RegisterRD != 0)) && (EX_MEM.RegisterRD == ID_EX.RegisterRT) && (MEM_WB.RegisterRD == ID_EX.RegisterRT) {
		if (ENABLE_FORWARDING == 1){
			ForwardA = 1;	
		}
		else{
			stall = 1;	
		}
	}
	    
	if (MEM_WB.RegWrite && (MEM_WB.RegisterRD != 0) && !(EX_MEM.RegWrite && (EX_MEM.RegisterRD != 0)) && (EX_MEM.RegisterRD == ID_EX.RegisterRT) && (MEM_WB.RegisterRD == ID_EX.RegisterRT) {
		if (ENABLE_FORWARDING == 1){
			ForwardB = 1;	
		}
		else{
			stall = 1;	
		}	
	}
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
	/*IMPLEMENT THIS*/
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
	printf("\nCurrent PC: %X", CURRENT_STATE.PC);
	printf("\nIF/ID.IR:  %X  instruction:   ", IF_ID.IR);
	print_instruction(CURRENT_STATE.PC);
	printf("\nIF/ID.PC: %X\n\n", IF_ID.PC );
	
	printf("\nID/EX.IR: %X    instruction:   ", ID_EX.IR);
	print_instruction(CURRENT_STATE.PC);
	printf("\nID/EX.IR:  %X", ID_EX.IR);
	printf("\nID/EX.A:  %X",ID_EX.A);
	printf("\nID/EX.B:  %X\n\n",ID_EX.B);
	
	printf("\nEX/MEM.IR:  %X", EX_MEM.IR);
	printf("\nEX/MEM.A:  %X", EX_MEM.A);
	printf("\nEX/MEM.B:  %X",EX_MEM.B);
	printf("\nEX/MEM.ALUOutput:  %X\n\n",EX_MEM.ALUOutput);
	
	printf("\nMEM/WB.IR:  %X",MEM_WB.IR);
	printf("\nMEM/WB.ALUOutput:  %X",MEM_WB.ALUOutput);
	printf("\nMEM/WB.LMD:  %X\n\n",MEM_WB.LMD );
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
