#include "VM.h"

int warn = 1;
int warnb = 1;
int warnca = 0;
int warncb = 1;

void insert(char file[BUFSIZ], int entryPoint) //ROM insterted for autoinsert
{
	FILE *fpa;
	fpa = fopen(file, "r");
	
	if (fpa == NULL) {
        printf("\nERROR: ROM file not found in current directory\n");
        exit(1);
    }
	
	if (entryPoint > 16382 | entryPoint < 0) {
		printf("\nERROR: Entry point is out of range\n");
		exit(1);
	}
	
	int i;
	
	for (i = entryPoint; i < 16383; i++) { //file reader
		fscanf(fpa, "%d%*[^\n]\n", &memory[i]); 
        if (memory[i] == 31) {
            warn = 0;
        }
        if (memory[i] == 25) {
            warnb = 0;
        }
        if (memory[i] == 255) {
            warnca = 1;
        }
        if (memory[i] >= 58 && memory[i] <= 65) {
            warncb = 0;
        }
    }
	
	if (warn == 1 | warnb == 1 | (warnca == 0 && warncb == 0)) { //error checker
        printf("\n");
    
        if (warn == 1) {
            printf("WARNING: Machine code file does not contain a HALT instruction\n");
        }
   
        if (warnb == 1) {
            printf("WARNING: Machine code does not assign an accumulator\n");
        }
        
        if (warnca == 0 && warncb == 0) {
            printf("WARNING: Machine code refrences the stack but does not set it\n");
        }
        
        printf("Continue?: Y/N\n");
        char a;
		a = getch();
        if (a == 'y' | a == 'Y') {
            return;
        }
        else {
            exit(0);
        }
    }
	fclose(fpa);
}

void manualRead()
{
	start:
    printf("\nEnter name of .mcpu file to load as ROM: ");
    char file[BUFSIZ];
    scanf("%s", file);
    
    FILE *fp;
    fp = fopen(file, "r");
    
    if (fp == NULL) {
        printf("\nERROR: ROM file not found in current directory\n");
        goto start;
    }
   
    int i;
    char a;
    char b;
    
    int entry;
    
    addrenter:
    printf("\nInput address entry point for this file (default 0, max 16382): ");
    scanf("%d", &entry);
    if (entry > 16382 | entry < 0) {
        printf("\nERROR: Entry point is out of range\n");
        goto addrenter;
    }
    
    for (i = entry; i < 16383; i++) { 
        fscanf(fp, "%d%*[^\n]\n", &memory[i]); 
        if (memory[i] == 31) {
            warn = 0;
        }
        if (memory[i] == 25) {
            warnb = 0;
        }
        if (memory[i] == 255) {
            warnca = 1;
        }
        if (memory[i] >= 58 && memory[i] <= 65) {
            warncb = 0;
        }
    }
    
    //error detector
    if (warn == 1 | warnb == 1 | (warnca == 0 && warncb == 0)) {
        printf("\n");
    
        if (warn == 1) {
            printf("WARNING: Machine code file does not contain a HALT instruction\n");
        }
   
        if (warnb == 1) {
            printf("WARNING: Machine code does not assign an accumulator\n");
        }
        
        if (warnca == 0 && warncb == 0) {
            printf("WARNING: Machine code refrences the stack but does not set it\n");
        }
        
        printf("Continue?: Y/N\n");
        a = getch();
        if (a == 'y' | a == 'Y') {
            goto success;
        }
        else {
            exit(0);
        }
    }
    //---------------
    
    success:
    printf("\nROM inserted successfully, load another file?: Y/N\n");
    b = getch();
    if (b == 'y' | b == 'Y') {
        goto start;
    }
    else {
        printf("\nMachine code fetching complete, press ENTER to run\n\n");
        getch();
        fclose(fp);
    }
}

void autoRead() //reads .mins file for inserting ROMs easily 
{
	start:
	printf("\n\nEnter name of .mins file to use as ROM locator: ");
	char file[BUFSIZ];
    scanf("%s", file);
	
	FILE *fp;
    fp = fopen(file, "r");
	
	if (fp == NULL) {
        printf("\nERROR: .mins file not found in current directory\n");
        goto start;
    }
	
	char filename[BUFSIZ];
	int startAddress;
	
	while(fscanf(fp, "%s %d", filename, &startAddress) != EOF) {
		insert(filename, startAddress);
	}
	
	printf("\nAutomatic ROM fetching complete, press ENTER to run\n\n");
	getch();
    fclose(fp);
}

void readMem() //needs .mins
{
	char a;
	printf("\nSelect machine code insertion mode: M = manual A = automatic (requires .mins file)");
	a = getch();
	if (a == 'm' | a == 'M') {
		manualRead();
	}
	else if (a == 'a' | a == 'A') {
		autoRead(); //call autoreader
	}
}