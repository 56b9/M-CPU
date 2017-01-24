#include "Registers.h"

#define MAX 255
#define MIN 0

/*
* 0 - 32767 = EEPROM (32k)
* 32768 - 49151 = RAM (16k)
* 58000 - 58005 = OUTPUT (2bytes)
* 61440 - 61447 = UNIVERSAL PORTS (8bytes)
* 65533 - 65534 = STATE SAVE (2bytes)
*/
unsigned char memory[65535];
unsigned short trap[] = {0x08, 0x10, 0x18, 0x20}; //0 = Bad Address, 1 = Stack Overflow
unsigned short vector[] = {0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60};

enum operations {NONE, ADD, SUB, AND, OR, XOR, NOT}; 

unsigned char ALU(unsigned char a, unsigned char b, char opcode) //arithmetic logic unit
{
    int out = MIN;
    
    switch(opcode) {
        case 1:
            out = a + b; //ADD
            break;
        case 2:
            out = a - b; //SUB
            break;
        case 3:
            out = a & b; //AND
            break;
        case 4:
            out = a | b; //OR
            break;
        case 5:
            out = a ^ b; //XOR
            break;
        case 6:
            out = ~a; //NOT
            break;
    }
   
   //conditional branching
    /*FLAGS:
    * 1 = true, 0 = false
    * Byte: |7|6|5|4|3|2|1|0|
    * 0: Parity
    * 1: Overflow
    * 2: Underflow
    * 3: Zero
    * 4: Unused
    * 5: User-defined (unused)
    * 6: User-defined (unused)
    * 7: Interrupt Mask Flag
    */

    if (out % 2 == 1) { //Parity
        F = ((0 << F) | 1);
    } 
    if (out > MAX) { //Overflow
        F = ((1 << F) | 1);
    }
    if (out < MIN) { //Underflow
        F = ((2 << F) | 1);
    }
    if (out == 0) { //Zero
        F = ((3 << F) | 1);
    }

    //Overflow Handling
    if (out > MAX) {out = MAX;} 
    else if (out < MIN) {out = MIN;}
    
    return (unsigned char) out;
}

char testBit(unsigned char byte, char n)
{
    char out = byte & (1 << n);
    return out;
}

unsigned char testBitZero(unsigned char byte, unsigned char nval)
{
    unsigned char out = (byte & nval);
    return out;
}

//Accumulator Handling
void assignAccumulator(char acc) 
{
    if (acc == 1) {
        AP = &A;
        BP = &AB;
    }
    else if (acc == 2) {
        AP = &AB;
        BP = &A;
    }
    else {
        AP = &A;
        BP = &AB;
    }
}

void errorHalt(int flag) //stop with error 
{
    showCursor();
    printf("\n\nCPU Unsafely halted at PC %d with error code %d\n", PC, flag);
    getch();
    exit(flag);
}

char spCheck() //make sure SP does not overflow
{
    if (SP < 32767 | SP >= 49151) {
		printf("\n\nBAD STACK");
		errorHalt(1);
    }
}

void jump(unsigned short address) //Jump PC to given address
{
    PC = address ;
    MDR = memory[PC];vxd
    jmpHld = 1;
}

void trapInt(int line) //traps cannot be masked
{
	//save PC state in known memory
	memory[65533] = (PC >> 8)&0xff; //high
    memory[65534] = PC&0xff; //low
	//push FLAGs
	SP--;
	memory[SP] = F;
	spCheck();
	//set mask bit
	F = (F|128);
	//jump to vector
	jump(trap[line]);
}

//functions
void readData(unsigned short address, unsigned char *reg) //new and improve getData with pointer
{ 
    if (address < 32767) {
        trapInt(0);
    }
    else {
        MAR = address;
        MDR = memory[MAR];
        *reg = MDR;
    }
} //because this is C after all

void writeData(unsigned short address, unsigned char reg) //write byte to address (store)
{
    if (address < 32767) {
        trapInt(0);
    }
    else {
        MAR = address;
        MDR = reg;
        memory[MAR] = MDR;
    }
}

unsigned short combine(unsigned char a, unsigned char b) //combine bytes into short (high, low)
{
    a = (unsigned short)a << 8;
    (unsigned short)b;
    
    unsigned char out;
    out = (a|b);
    return out;
}

void incPC() //increment PC and put info into MDR
{
    if (jmpHld == 0) {
        PC++;
        MDR = memory[PC];
    }
}

void halt() //stop VM
{
	showCursor();
	printf("\n\n======================================================\n");
    printf("\n\nCPU Safely halted at PC %d\n\n", PC);
	registerDump();
    getch();
    exit(0);
}

//Interrupt handler 
void interrupt(int line) //interrput controller 
{
	if (testBit(F, 7) == 0) { //if interrupt mask is off
		//save PC state in known memory
		memory[65533] = (PC >> 8)&0xff; //high
        memory[65534] = PC&0xff; //low
		//push FLAGs
		SP--;
		memory[SP] = F;
		//set mask bit
		F = (F|128);
		//jump to vector
		jump(vector[line]);
	}
	else if (testBit(F,7) != 0 && FETCH == 0) { //if mask is on but interrput is unmaskable
		//save PC state in known memory
		memory[65533] = (PC >> 8)&0xff; //high
        memory[65534] = PC&0xff; //low
		//push FLAGs
		SP--;
		memory[SP] = F;
		//set mask bit
		F = (F|128);
		//jump to vector
		jump(vector[line]);
	}
	else {
		return;
	}
}

/*
* INSTRUCTION HANDLING:
* Addressing: 24-bit word: stops at 3 fetch cycles
* Non-addressing: 16-bit word: stops at 2 fetch cycles
* Non-operand: 8-bit word: stops at 1 fetch cycle
*/

//instruction functions
void fetch() //instruction fetch function
{
    if (PC == 0) {
        MDR = memory[PC];
    } 
    
    IR[0] = MDR;
    
    if (IR[0] <= 127 | IR[1] >= 240) { //get one byte
        WM = 1;
        return;
    }
    else if (IR[0] <= 192) { //get two bytes
        WM = 2;
        incPC();
        IR[1] = MDR;
        return;
    }
    else if (IR[0] > 192) { //get three bytes
        WM = 3;
        incPC();
        IR[1] = MDR;
        incPC();
        IR[2] = MDR;
        return;
    }
}

void decode() //decode stage
{   
    switch(WM) {
        case 1:
            return;
            break;
        case 2:
            FETCH = IR[1];
            break;
        case 3:
            AH = ((short)(IR[1] << 8) | IR[2]);
            break;
    }
}

void execute() //execute stage
{
    char temp;
	unsigned short ta;
	unsigned short tb;
    switch (IR[0]) {
    //One-byte operations
        case 0: //NOP
            return;
            break;
        case 1: //MOV A X (src, dest)
            X = A;
            break;
        case 2: //MOV A Y 
            Y = A;
            break;
        case 3: //MOV A Z
            Z = A;
            break;
        case 4: //MOV A F
            F = A;
            break;
        case 5: //MOV X A
            A = X;
            break;
        case 6: //MOV X Y
            Y = X;
            break;
        case 7: //MOV X Z
            Z = X;
            break;
        case 8: //MOV Y A
            A = Y;
            break;
        case 9: //MOV Y X
            X = Y;
            break;
        case 10: //MOV Y Z
            Z = Y;
            break;
        case 11: //ACC Z
            A = Z;
            break;
        case 12: //MOV Z X
            X = Z;
            break;
        case 13: //MOV Z Y
            Y = Z;
            break;
        case 14: //MOV F A
            A = F;
            break;
        case 15: //MOV A AB
            AB = A;
            break;
        case 16: //MOV X AB
            AB = X;
            break;
        case 17: //MOV Y AB
            AB = Y;
            break;
        case 18: //MOV Z AB
            AB = Z;
            break;
    //ALU Operations
        case 19: //ADD
            *AP = ALU(*AP, *BP, ADD);
            break;
        case 20: //SUB
            *AP = ALU(*AP, *BP, SUB);
            break;
        case 21: //AND
            *AP = ALU(*AP, *BP, AND);
            break;
        case 22: //OR
            *AP = ALU(*AP, *BP, OR);
            break;
        case 23: //XOR
            *AP = ALU(*AP, *BP, XOR);
            break;
        case 24: //NOT
            *AP = ALU(*AP, *BP, NOT);
            break;
    //Accumulator Switching
        case 25: //SET A
            assignAccumulator(1);
            break;
        case 26: //SET AB
            assignAccumulator(2);
            break;
    //MORE MOV
        case 27: //MOV AB A
            A = AB;
            break;
        case 28: //MOV AB X
            X = AB;
            break;
        case 29: //MOV AB Y
            Y = AB;
            break;
        case 30: //MOV AB Z
            Z = AB;
            break;
    //HALT
        case 31: //HLT
            halt();
            break;
    //OTHER MOV
        case 32: //MOV IX PC
            PC = IX;
            break;
        case 33: //MOV PC IX
            IX = PC;
            break;
        case 34: //MOV IX SP
            SP = IX;
            break;
        case 35: //MOV SP IX
            IX = SP;
            break;
        case 36: //MOV IY PC
            PC = IY;
            break;
        case 37: //MOV PC IY
            IY = PC;
            break;
        case 38: //MOV IY SP
            SP = IY;
            break;
        case 39: //MOV SP IY
            IY = SP;
            break;
        case 40: //MOV IX IY
            IY = IX;
            break;
        case 41: //MOV IY IX
            IX = IY;
            break;
        case 42: //MOV PC SP
            SP = PC;
            break;
        case 43: //MOV SP PC
            PC = SP;
            break;
    //16 bit MOV
        case 44: //MOV FX IX
            IX = combine(F, X);
            break;
        case 45: //MOV YZ IX
            IX = combine(Y, Z);
            break;
        case 46: //MOV FX SP
            SP = combine(F, X);
            break;
        case 47: //MOV YZ SP
            SP = combine(Y, Z);
            break;
        case 48: //MOV FX PC
            PC = combine(F, X);
            break;
        case 49: //MOV YZ PC
            PC = combine(Y, Z);
            break;
        case 50: //MOV IX FX
            F = (IX >> 8)&0xff; //high 
            X = IX&0xff; //low
            break;
        case 51: //MOV IX YZ
            Y = (IX >> 8)&0xff;
            Z = IX&0xff;
            break;
        case 52: //MOV IY FX
            F = (IY >> 8)&0xff;
            X = IY&0xff;
            break;
        case 53: //MOV IY YZ
            Y = (IY >> 8)&0xff;
            Z = IY&0xff;
            break;
        case 54: //MOV SP FX
            F = (SP >> 8)&0xff;
            X = SP&0xff;
            break;
        case 55: //MOV SP YZ
            Y = (SP >> 8)&0xff;
            Z = SP&0xff;
            break;
        case 56: //MOV PC FX
            F = (PC >> 8)&0xff;
            X = PC&0xff;
            break;
        case 57: //MOV PC YZ
            Y = (PC >> 8)&0xff;
            Z = PC&0xff;
            break;
    //STACK
        case 58: //PUSH F
            SP--;
            spCheck();
            memory[SP] = F;
            break;
        case 59: //POP F
            F = memory[SP];
            SP++;
            break;
        case 60: //PUSH X
            SP--;
            spCheck();
            memory[SP] = X;
            break;
        case 61: //POP X
            X = memory[SP];
            SP++;
            break;
        case 62: //PUSH Y
            SP--;
            spCheck();
            memory[SP] = Y;
            break;
        case 63: //POP Y
            Y = memory[SP];
            SP++;
            break;
        case 64: //PUSH Z
            SP--;
			spCheck();
            memory[SP] = Z;
            break;
        case 65: //POP Z
            Z = memory[SP];
            SP++;
            break;
    //INC
        case 66: //INC A
            A++;
            break;
        case 67: //INC AB
            AB++;
            break;
        case 68: //INC X
            X++;
            break;
        case 69: //INC Y
            Y++;
            break;
        case 70: //INC Z
            Z++;
            break;
        case 71: //INC IX
            IX++;
            break;
        case 72: //INC IY
            IY++;
            break;
    //DEC
        case 73: //DEC ACC
            A--;
            break;
        case 74: //DEC AB
            AB--;
            break;
        case 75: //DEC X
            X--;
            break;
        case 76: //DEC Y
            Y--;
            break;
        case 77: //DEC Z
            Z--;
            break;
        case 78: //DEC IX
            IX--;
            break;
        case 79: //DEC IY
            IY--;
            break;
    //IX BRANCHING
        case 80: //JMP 
            jump(IX);
            break;
        case 81: //JMP P
            if (testBit(F, 0) != 0) {jump(IX);} //parity
            else {return;}
            break;
        case 82: //JMP O
            if (testBit(F, 1) != 0) {jump(IX);} //Overflow
            else {return;}
            break;
        case 83: //JMP U
            if (testBit(F, 2) != 0) {jump(IX);} //Underflow
            else {return;}
            break;
        case 84: //JMP Z
            if (testBit(F, 3) != 0) {jump(IX);} //Zero
            else {return;}
            break;
        case 85: //JMP UD1
            if (testBit(F, 5) != 0) {jump(IX);} //User-defined
            else {return;}
            break;
        case 86: //JMP UD2
            if (testBit(F, 6) != 0) {jump(IX);} //User-defined
            else {return;}
            break;
    //inverse conditional branching
        case 87: //JMP NP
            if (testBitZero(F, 1) == 0) {jump(IX);} //parity
            else {return;}
            break;
        case 88: //JMP NO
            if (testBitZero(F, 2) == 0) {jump(IX);} //Overflow
            else {return;}
            break;
        case 89: //JMP NU
            if (testBitZero(F, 4) == 0) {jump(IX);} //Underflow
            else {return;}
            break;
        case 90: //JMP NZ
            if (testBitZero(F, 8) == 0) {jump(IX);} //Zero
            else {return;}
            break;
        case 91: //JMP NUD1
            if (testBitZero(F, 32) == 0) {jump(IX);} //User-defined
            else {return;}
            break;
        case 92: //JMP NUD2
            if (testBitZero(F, 64) == 0) {jump(IX);} //User-defined
            else {return;}
            break;
    //Indexed addressing
        case 93: //IX MST A
            writeData(IX, A); //write to RAM
            break;
        case 94: //IX MLD A
            readData(IX, &A); //read from RAM
            break;
        case 95: //IX MST AB
            writeData(IX, AB); 
            break;
        case 96: //IX MLD AB
            readData(IX, &AB); 
            break;
        case 97: //IX MST X
            writeData(IX, X); 
            break;
        case 98: //IX MLD X
            readData(IX, &X); 
            break;
        case 99: //IX MST Y
            writeData(IX, Y); 
            break;
        case 100: //IX MLD Y
            readData(IX, &Y); 
            break;
        case 101: //IX MST Z
            writeData(IX, Z); 
            break;
        case 102: //IX MLD Z
            readData(IX, &Z); 
            break;
        case 103: //IX MST F
            writeData(IX, F); 
            break;
        case 104: //IX MLD F
            readData(IX, &F); 
            break;
        case 105: //IX MLD SP LOW (moves lowest byte of SP into memory)
            writeData(IX, SP&0xff);
            break;
        case 106: //IX MLD SP HIGH (moves highest byte of SP into memory)
            writeData(IX, (SP >> 8)&0xff);
            break;
        case 107: //IX MLD IY LOW
            writeData(IX, IY&0xff);
            break;
        case 108: //IX MLD IY HIGH 
            writeData(IX, (IY >> 8)&0xff);
            break;
    //Index + Base addressing
        case 109: //IB MST A
            writeData(IX + IY, A); //write to RAM
            break;
        case 110: //IB MLD A
            readData(IX + IY, &A); //read from RAM
            break;
        case 111: //IB MST AB
            writeData(IX + IY, AB); 
            break;
        case 112: //IB MLD AB
            readData(IX + IY, &AB); 
            break;
        case 113: //IB MST X
            writeData(IX + IY, X); 
            break;
        case 114: //IB MLD X
            readData(IX + IY, &X); 
            break;
        case 115: //IB MST Y
            writeData(IX + IY, Y); 
            break;
        case 116: //IB MLD Y
            readData(IX + IY, &Y); 
            break;
        case 117: //IB MST Z
            writeData(IX + IY, Z); 
            break;
        case 118: //IB MLD Z
            readData(IX + IY, &Z); 
            break;
        case 119: //IB MST F
            writeData(IX + IY, F); 
            break;
        case 120: //IB MLD F
            readData(IX + IY, &F); 
            break;
        case 121: //IB MLD SP LOW (moves lowest byte of SP into memory)
            writeData(IX + IY, SP&0xff);
            break;
        case 122: //IB MLD SP HIGH (moves highest byte of SP into memory)
            writeData(IX + IY, (SP >> 8)&0xff);
            break;
    //STACK ACCUMULATORS
        case 123: //PUSH A
            SP--;
            spCheck();
            memory[SP] = A;
            break;
        case 124: //POP A
            A = memory[SP];
            SP++;
            break;
        case 125: //PUSH AB
            SP--;
            spCheck();
            memory[SP] = AB;
            break;
        case 126: //POP AB
            AB = memory[SP];
            SP++;
            break;
    //CLEAR
        case 127: //CLR ALL
            A = 0;
            AB = 0;
            X = 0;
            Y = 0;
            Z = 0;
            IX = 0;
            IY = 0;
    //two-byte instructions
        case 128: //LD A
            A = FETCH;
            break;
        case 129: //LD AB
            AB = FETCH;
            break;
        case 130: //LD X
            X = FETCH;
            break;
        case 131: //LD Y
            Y = FETCH;
            break;
        case 132: //LD Z
            Z = FETCH;
            break;
        case 133: //LD F
            F = FETCH;
            break;
    //Interrupts (software and hardware)
        /*Vectors:
        * 0: 0x08
        * 1: 0x10
        * 2: 0x18
        * 3: 0x20
        * 4: 0x28
        * 5: 0x30
        * 6: 0x38
        * 7: 0x40
        */
        case 134: //INT A
			interrupt(0);
			break;
        case 135: //INT B
            interrupt(1);
            break;
        case 136: //INT C
			interrupt(2);
            break;
        case 137: //INT D
			interrupt(3);
            break;
        case 138: //INT E
			interrupt(4);
            break;
        case 139: //INT F
			interrupt(5);
            break;
        case 140: //INT G
			interrupt(6);
            break;
        case 141: //INT H
			interrupt(7);
            break;
    //TRAPs 
        case 142: //TRP A
            trapInt(0);
            break;
        case 143: //TRP B
            trapInt(1);
            break;
        case 144: //TRP C
            trapInt(2);
            break;
        case 145: //TRP D
            trapInt(3);
            break;
    //Three byte instructions
    //AH BRANCHING
        case 192: //JMP 
            jump(AH);
            break;
        case 193: //JMP P
            if (testBit(F, 0) != 0) {jump(AH);} //parity
            else {return;}
            break;
        case 194: //JMP O
            if (testBit(F, 1) != 0) {jump(AH);} //Overflow
            else {return;}
            break;
        case 195: //JMP U
            if (testBit(F, 2) != 0) {jump(AH);} //Underflow
            else {return;}
            break;
        case 196: //JMP Z
            if (testBit(F, 3) != 0) {jump(AH);} //Zero
            else {return;}
            break;
        case 197: //JMP UD1
            if (testBit(F, 5) != 0) {jump(AH);} //User-defined
            else {return;}
            break;
        case 198: //JMP UD2
            if (testBit(F, 6) != 0) {jump(AH);} //User-defined
            else {return;}
            break;
    //inverse conditional branching
        case 199: //JMP NP
            if (testBitZero(F, 1) == 0) {jump(AH);} //parity
            else {return;}
            break;
        case 200: //JMP NO
            if (testBitZero(F, 2) == 0) {jump(AH);} //Overflow
            else {return;}
            break;
        case 201: //JMP NU
            if (testBitZero(F, 4) == 0) {jump(AH);} //Underflow
            else {return;}
            break;
        case 202: //JMP NZ
            if (testBitZero(F, 8) == 0) {jump(AH);} //Zero
            else {return;}
            break;
        case 203: //JMP NUD1
            if (testBitZero(F, 32) == 0) {jump(AH);} //User-defined
            else {return;}
            break;
        case 204: //JMP NUD2
            if (testBitZero(F, 64) == 0) {jump(AH);} //User-defined
            else {return;}
            break;
    //Load/Store 
        case 205: //MST ACC
            writeData(AH, A); //write to RAM
            break;
        case 206: //MLD ACC
            readData(AH, &A); //read from RAM
            break;
        case 207: //MST X
            writeData(AH, X); 
            break;
        case 208: //MLD X
            readData(AH, &X); 
            break;
        case 209: //MST Y
            writeData(AH, Y); 
            break;
        case 210: //MLD Y
            readData(AH, &Y); 
            break;
        case 211: //MST Z
            writeData(AH, Z); 
            break;
        case 212: //MLD Z
            readData(AH, &Z); 
            break;
        case 213: //MST F
            writeData(AH, F); 
            break;
        case 214: //MLD F
            readData(AH, &F); 
            break;
        case 215: //MLD SP LOW (moves lowest byte of SP into memory)
            writeData(AH, SP&0xff);
            break;
        case 216: //MLD SP HIGH (moves highest byte of SP into memory)
            writeData(AH, (SP >> 8)&0xff);
            break;
        case 217: //MLD IY LOW
            writeData(AH, IY&0xff);
            break;
        case 218: //MLD IY HIGH 
            writeData(AH, (IY >> 8)&0xff);
            break;
        case 219: //MLD IX LOW
            writeData(AH, IX&0xff);
            break;
        case 220: //MLD IX HIGH 
            writeData(AH, (IX >> 8)&0xff);
            break;
    //16 bit loads
        case 221: //LD IX 
            IX = AH;
            break;
        case 222: //LD IY
            IY = AH;
            break;
        case 223: //LD FX
            F = (AH >> 8)&0xff; //high
            X = AH&0xff; //low
            break;
        case 224: //LD YZ
            Y = (AH >> 8)&0xff;
            Z = AH&0xff;
            break;
	//Address clearing
		case 225: //CLA
			writeData(AH, 0);
			break;
	//Set stack
        case 226: //SETSTK
            SP = AH;
            spCheck();
            break;
	//PC operations
		//three bytes
		case 237: //RST PC (restore PC from memory location)
			ta = memory[65533];
			tb = memory[65534];
			PC = combine(ta, tb);
			break;
		case 238: //MLD PC LOW (moves lowest byte of PC into memory)
            writeData(AH, PC&0xff);
            break;
        case 239: //MLD PC HIGH (moves highest byte of PC into memory)
            writeData(AH, (PC >> 8)&0xff);
            break;
		//one byte
		case 240: //IX MLD PC LOW (moves lowest byte of PC into memory)
            writeData(IX, PC&0xff);
            break;
        case 241: //IX MLD PC HIGH (moves highest byte of PC into memory)
            writeData(IX, (PC >> 8)&0xff);
            break;
	    case 242: //IB MLD PC LOW (moves lowest byte of PC into memory)
            writeData(IX + IY, PC&0xff);
            break;
        case 243: //IB MLD PC HIGH (moves highest byte of PC into memory)
            writeData(IX + IY, (PC >> 8)&0xff);
            break;
    }
}

int hardwareInt(int line, int execute)
{
	//hardware interrupts cannot override masking 
	if(execute != 0) {
		if (testBit(F, 7) == 0) { //if interrupt mask is off
			//save PC state in known memory
			memory[65533] = (PC >> 8)&0xff; //high
			memory[65534] = PC&0xff; //low
			//push FLAGs
			SP--;
			memory[SP] = F;
			//set mask bit
			F = (F|128);
			//jump to vector
			jump(vector[line]);
			return 1;
		}
	}
	else {
		return 0;
	}
}

void output() 
{	
	//58000 = enable
	//58001 = data
	//58005 = clear
	int i;	
    if (memory[58000] != 0 && memory[58001] != 0) {
		printf("%c", memory[58001]);
		memory[58001] = 0;
		if (memory[58005] != 0) {
			system("CLS");
		}
	}
	else {
		return;
	}
}

void run() 
{
	fetch();
    decode();
	execute();
    incPC();
    jmpHld = 0;
    ticks++;
}