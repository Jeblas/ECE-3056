/***************************************************************/
/*                                                             */
/* LC-3b Simulator (Adapted from Prof. Yale Patt at UT Austin) */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%04x\n", BUS);
    printf("MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%04x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%04x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%04x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%04x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%04x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%04x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%04x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) { 
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* --------- DO NOT MODIFY THE CODE ABOVE THIS LINE -----------*/
/***************************************************************/

/***************************************************************/
/* You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/

// GLOBAL VARIABLES /////////////////////////////////////////////

// memCycle variable to keep track of memory read cycles
int memCycles = 1;

// Variables to hold values before driving the bus
int Gate_MARMUX;
int Gate_SHF;
int Gate_ALU;
int Gate_MDR;
int Gate_PC;

// WE Logic Variables
int WE1;
int WE0;

////////////////////////////////////////////////////////////////

// HELPER FUNCTIONS ////////////////////////////////////////////

// get_bits is used to extract values from IR
int get_bits(int value, int start, int end)
{
	int result;
	//assert(start >= end);
	result = value >> end;
	result = result % (1 << ( start - end + 1 ) );
	return result;
}

// Sign extend
int SEXT(int value, int topbit)
{
	int shift = sizeof(int) * 8 - topbit;
	return (value << shift) >> shift;
}

// ZEXT()
int ZEXT(int value, int topbit)
{
	return (value & ( (1<<(topbit+1))-1));
}

// Left shift
int LSHF(int value, int amount)
{
	return (value << amount) & 0xFFFF;
}

////////////////////////////////////////////////////////////////

void eval_micro_sequencer() {

  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */

    // Next State depends if IRD = 0
    if(GetIRD(CURRENT_LATCHES.MICROINSTRUCTION) == 0)
    {
	// Next state depends on the condition code
	switch (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION))
	{
		case 0:
			NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION); break;
		case 1:
			NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION) | (CURRENT_LATCHES.READY << 1); break;
                case 2: 
			NEXT_LATCHES.STATE_NUMBER = 18 | CURRENT_LATCHES.BEN << 2; break;
		case 3: 
			NEXT_LATCHES.STATE_NUMBER = 20 | get_bits(CURRENT_LATCHES.IR, 11, 11); break;
	}
    }else
    {
	// get next state from OPCODE in the Instruction Register for state 32
    	NEXT_LATCHES.STATE_NUMBER = get_bits(CURRENT_LATCHES.IR, 15, 12);
	
    }
    // Latch the state's next microinstruction
    int i;
    for (i = 0; i<35; ++i)
    {
    	NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER][i];
    }
}


void cycle_memory() {
 
  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */

   // Memory and WE logic
   // TODO Data is still calculated during a memory access state 
   if((GetR_W(CURRENT_LATCHES.MICROINSTRUCTION) == 0) || (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == 0) || (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 1))
   {
	// assuming data_size = 1 means byte
   	if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 0)
	{
		//compare MAR[0] which
		if(get_bits(CURRENT_LATCHES.MAR, 0, 0) == 0)
		{
			WE1 = 0;
			WE0 = 1;
		}
		else
		{
			WE1 = 1;
			WE0 = 0;
		}
	}
	else
	{
		WE1 = 1;
		WE0 = 1;
	}
   }
   else
   {
   	WE1 = 0;
	WE0 = 0;
   }

   // Check if current state accesses memory
   if (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
   	if (memCycles == 4)
	{
	    NEXT_LATCHES.READY = 1;
	    memCycles = 0;
	}else{
	    NEXT_LATCHES.READY = 0;	
	    ++memCycles;
	}
   }
}

void eval_bus_drivers() {

  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */

    int SR1_OUT;
    int GATE_ALU_VALUE = GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION);
    int GATE_SHF_VALUE = GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION);
    int ADDR1MUX_VALUE = GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);

   // Get SR1_OUT or conditions for ADDR1MUX if ALU or SHF is used or ADDR1MUX needs
   if(GATE_ALU_VALUE == 1 || GATE_SHF_VALUE == 1 || ADDR1MUX_VALUE == 1)
   {
   	if (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   	{
	   	SR1_OUT = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 8, 6)];
   	}
   	else
   	{
   	   	SR1_OUT = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 11, 9)];
   	}

   }

   // Value to be sent to BUS from ALU
   if(GATE_ALU_VALUE == 1)
   {
	int ALUK_VALUE = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION); 
	int SR2_MUX_OUT;
       //calculate SR2_MUX_OUT
       if(ALUK_VALUE != 3)
       {
       		if(get_bits(CURRENT_LATCHES.IR, 5, 5) == 1)
       		{
       	  		// load immediate SEXT into SR2_MUX_OUT
			SR2_MUX_OUT = SEXT(get_bits(CURRENT_LATCHES.IR, 4, 0), 5);
      		}else
       		{
       	  		//load sr2 into SR2_MUX_OUT
			SR2_MUX_OUT = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 2, 0)];
       		}
	}
       // Depending on ALUK perform an operation on SR1_OUT and SR2_MUX_OUT
	switch (ALUK_VALUE)
	{
		case 0:
			Gate_ALU = SR2_MUX_OUT + SR1_OUT; break;
		case 1:
			Gate_ALU = SR2_MUX_OUT & SR1_OUT; break;
		case 2:
			Gate_ALU = SR2_MUX_OUT ^ SR1_OUT; break;
		case 3:
			Gate_ALU = SR1_OUT; break; // PASSA
	}
   }

   // Value to be sent to BUS from MARMUX
   else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION))
   {
	int MARMUX_VALUE = GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION);
   	if(MARMUX_VALUE == 0)
	{
		Gate_MARMUX = LSHF(ZEXT(get_bits(CURRENT_LATCHES.IR, 7, 0), 8),1);
	}
	else
	{
		int ADDR_IN_LEFT;
		int ADDR2MUX_VALUE = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
		// Get ADDR_IN_LEFT value depending on ADDR2MUX_VALUE
		switch (ADDR2MUX_VALUE)
		{
			case 0:
				ADDR_IN_LEFT = 0; break;
			case 1:
				ADDR_IN_LEFT = SEXT(get_bits( CURRENT_LATCHES.IR, 5, 0), 6); break;
			case 2:
				ADDR_IN_LEFT = SEXT(get_bits( CURRENT_LATCHES.IR, 8, 0), 9); break;
			case 3:
				ADDR_IN_LEFT = SEXT(get_bits( CURRENT_LATCHES.IR, 10, 0), 11); break;
		}
		if (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
		{
			ADDR_IN_LEFT = LSHF(ADDR_IN_LEFT,1);
		}

		if (ADDR1MUX_VALUE == 1)
		{
			Gate_MARMUX = ADDR_IN_LEFT + SR1_OUT;
		}
		else
		{
			Gate_MARMUX = ADDR_IN_LEFT + CURRENT_LATCHES.PC;
		}
	}
   }

   // Value to be sent to BUS from SHF
   else if(GATE_SHF_VALUE == 1)
   {
	// Already calculated SR1_OUT by this point
  	int SHF_amount = get_bits(CURRENT_LATCHES.IR, 3, 0);
 
        if(get_bits(CURRENT_LATCHES.IR, 4,4) == 0)
        {
            // Left shift SR1_OUT by SHF_amount
            Gate_SHF = SR1_OUT << SHF_amount;
        }
        else
        {
            if(get_bits(CURRENT_LATCHES.IR,5,5) == 0)
            {
                // Right shift Logical
                Gate_SHF = SR1_OUT >> SHF_amount;
            }
            else
            {
                // Right shift Arith
                Gate_SHF = SEXT(SR1_OUT, 16) >> SHF_amount;
            }
        }
   }

   // Value to be sent to BUS from MDR
   // TODO Account for logic between MDR and GateMDR
   else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
   	Gate_MDR = CURRENT_LATCHES.MDR;
   }

   // Value to be sent to BUS from PC
   else if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
   	Gate_PC = CURRENT_LATCHES.PC;
   }

}


void drive_bus() {

  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   *
   */
   if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
       BUS = Low16bits(Gate_PC);
   }
   else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
	BUS = Low16bits(Gate_MDR);
   }
   else if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
	BUS = Low16bits(Gate_ALU);
   }
   else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
	BUS = Low16bits(Gate_MARMUX);
   }
   else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
   {
	BUS = Low16bits(Gate_SHF);
   }
   else
   {
        BUS = 0;
   }

}


void latch_datapath_values() {

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */       						
	
	// Latch IR
	if (GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		// IR <- MDR
		NEXT_LATCHES.IR = Low16bits(BUS);
	}
	else
	{
		NEXT_LATCHES.IR = CURRENT_LATCHES.IR;
	}

	// Latch MAR
	if (GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
    		NEXT_LATCHES.MAR = Low16bits(BUS);
	}
	else
	{
		// No change to MAR
		NEXT_LATCHES.MAR = CURRENT_LATCHES.MAR;
	}

	// Latch PC
	if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		int PCMUX_VALUE = GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
		if(PCMUX_VALUE == 0)
		{
			// PC <- PC +2
			NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + 2);
		}
		else if(PCMUX_VALUE == 2)
		{
			int ADDR1MUX_VALUE = GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);

			if(ADDR1MUX_VALUE == 0)
			{
				// PC <- PC + LSHF(offset,1)
				int ADDR2MUX_VALUE = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
				switch (ADDR2MUX_VALUE)
				{
					case 0: // Never Happens
						NEXT_LATCHES.PC = CURRENT_LATCHES.PC; break;
					case 1: // Never Happens
						NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + LSHF(SEXT(get_bits(CURRENT_LATCHES.IR, 5, 0), 6),1)); break;
					case 2:
						NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + LSHF(SEXT(get_bits(CURRENT_LATCHES.IR, 8, 0), 9),1)); break;
					case 3:
						NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + LSHF(SEXT(get_bits(CURRENT_LATCHES.IR, 10, 0), 11),1)); break;
				} 
			}
			else
			{
				// Never a case where PC = BaseR + Immediate
				// PC <- BaseR
				if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) 
				{
					NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR , 11, 9)];
				}
				else
				{
					NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 8, 6)];
				}
			}
		}
		else if (PCMUX_VALUE == 1)
		{
			// PC <- MDR
			NEXT_LATCHES.PC = Low16bits(BUS);
		}
	}
	else
	{
		// No change to PC
		NEXT_LATCHES.PC = CURRENT_LATCHES.PC;
	}
	
	// Latch CC
	if(GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		if (BUS == 0)
		{
			NEXT_LATCHES.N = 0;
			NEXT_LATCHES.Z = 1;
			NEXT_LATCHES.P = 0;
		}
		else if (SEXT(BUS, 16) > 0)
		{
			NEXT_LATCHES.N = 0;
			NEXT_LATCHES.Z = 0;
			NEXT_LATCHES.P = 1;
		}
		else if (SEXT(BUS, 16) < 0)
		{
			NEXT_LATCHES.N = 1;
			NEXT_LATCHES.Z = 0;
			NEXT_LATCHES.P = 0;
		}
		else
		{
			// Should never reach this
			NEXT_LATCHES.N = 0;
			NEXT_LATCHES.Z = 0;
			NEXT_LATCHES.P = 0;
		}
	
	}
	else
	{
		// Copy CC from current latches to next -NO CHANGE-
		NEXT_LATCHES.N = CURRENT_LATCHES.N;
		NEXT_LATCHES.Z = CURRENT_LATCHES.Z;
		NEXT_LATCHES.P = CURRENT_LATCHES.P;
	}

	// Latch BEN
	if (GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		// If IR[11] & N || IR[10] & Z || IR[9] & P  BEN = 1
		if((get_bits(CURRENT_LATCHES.IR, 11, 11) & CURRENT_LATCHES.N) || (get_bits(CURRENT_LATCHES.IR, 10, 10) & CURRENT_LATCHES.Z) || ( get_bits(CURRENT_LATCHES.IR, 9, 9) & CURRENT_LATCHES.P))
		{
			NEXT_LATCHES.BEN = 1;
		}
		else
		{
			NEXT_LATCHES.BEN = 0;
		}
	}
	else
	{
		// Copy BEN to the next latch -NO CHANGE-
		NEXT_LATCHES.BEN = CURRENT_LATCHES.BEN;
	}

	// Latch REGS
	// Copy all of CURRENT_REGS to NEXT_REGS
	int i;
	for(i=0; i < 8; ++i)
	{
		NEXT_LATCHES.REGS[i] = CURRENT_LATCHES.REGS[i];
	}

	if (GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		// Update register (Depends on DRMUX) with value from BUS
		if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0)
		{
			NEXT_LATCHES.REGS[get_bits(CURRENT_LATCHES.IR, 11, 9)] = Low16bits(BUS);
		}
		else
		{
			NEXT_LATCHES.REGS[7] = Low16bits(BUS);
		}
	}

	// Latch MDR
	if (GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == 0)
		{
                        // get data from bus
			if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
			{
				NEXT_LATCHES.MDR = Low16bits(BUS);
			}
			else
			{ 
                                NEXT_LATCHES.MDR = BUS & 0x00FF;
			}
		}
		else
		{
			// Assuming other option is always reading from memory
			int MEM_ADDR = CURRENT_LATCHES.MAR >> 1;
			if ((WE1 & WE0) == 1)
			{
				NEXT_LATCHES.MDR = (MEMORY[MEM_ADDR][1] << 8) | (MEMORY[MEM_ADDR][0] & 0x00FF);
			}
			else
			{
				if(WE1 == 1)
				{
					NEXT_LATCHES.MDR = Low16bits(SEXT(MEMORY[MEM_ADDR][1], 8));
				}
				else
				{
					NEXT_LATCHES.MDR = Low16bits(SEXT(MEMORY[MEM_ADDR][0], 8));
				}
			}
		}
	}
	else
	{
		NEXT_LATCHES.MDR = CURRENT_LATCHES.MDR;
	}

	// Load Memory assuming R.W = 1  is Writing
	if (GetR_W(CURRENT_LATCHES.MICROINSTRUCTION) == 1)
	{
		int MEM_ADDR_W = CURRENT_LATCHES.MAR >> 1;
		if((WE1 & WE0) == 1)
		{
			MEMORY[MEM_ADDR_W][1] = (CURRENT_LATCHES.MDR >> 8) & 0x00FF;
			MEMORY[MEM_ADDR_W][0] = CURRENT_LATCHES.MDR & 0x00FF;
		}
		else
		{
			// Assume when ldb you store a byte of data to mdr
			int BYTE_STORE_VALUE = CURRENT_LATCHES.MDR & 0x00FF;
			if(get_bits(CURRENT_LATCHES.MAR, 0, 0) == 1)
			{
				MEMORY[MEM_ADDR_W][1] = BYTE_STORE_VALUE;
			}
			else
			{
				MEMORY[MEM_ADDR_W][0] = BYTE_STORE_VALUE;
			}
		}
	}
}
