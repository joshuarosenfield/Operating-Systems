//COP4610 Project 3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> //uint8

/* STRUCT DEFINITIONS */
typedef struct
{
	char** tokens;
	int numTokens;
} instruction;
// unit depends bytes needed
typedef struct
{
	uint8_t BS_jmpBoot[3];
	uint8_t BS_OEMName[8];
	uint16_t BPB_BytsPerSec;
} __attribute__((packed)) boot_sector_struct;
/* END STRUCT DEFINITIONS */

/* 1 TO PRINT TEST PRINTOUTS */
int testPrints = 1;

/* GLOBAL VARIABLES */
boot_sector_struct* bootSector;
char* imagePath;
/* END GLOBAL VARIABLES */

/* FUNCTION DEFINITIONS */
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
void executeCommand(instruction* instr_ptr);
void func_exit();
void info(boot_sector_struct* bs);
boot_sector_struct * bootSectorParse(void);
/* END FUNCTION DEFINITIONS */

int main(int argc, char** argv) {
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
	
	if(!argv[1]){
		printf("arguemnt error\nexiting...\n");
		exit(1);
	}
	// SET IMAGE PATH
	imagePath = argv[1];
	printf("Welcome to the ./FAT32 shell utility\n");
	bootSector = bootSectorParse();
	printf("Image %s, is ready to view\n", argv[1]);
	while (1) {
		printf("/] ");
		do {
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '`') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}
					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';
					addToken(&instr,specialChar);
					start = i + 1;
				}
			}
			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}
			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached
		addNull(&instr);
		//printTokens(&instr);
		
		executeCommand(&instr);
		clearInstruction(&instr);
	}
	free(bootSector);
	return 0;
}

void executeCommand(instruction *instr_ptr){
	//printTokens(instr_ptr);
	if(strcmp(instr_ptr->tokens[0], "exit") == 0){
		if(testPrints)
			printf("call exit function\n");
		func_exit();
	}
	else if(strcmp(instr_ptr->tokens[0], "info") == 0){
                if(testPrints)
			printf("call info function\n");
		info(bootSector);
	}
	else if(strcmp(instr_ptr->tokens[0], "size") == 0){
        	if(testPrints)
		       printf("call size function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "ls") == 0){
        	if(testPrints)
		        printf("call ls function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "cd") == 0){
        	if(testPrints)
		       printf("call cd function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "creat") == 0){
         	if(testPrints)
		      printf("call creat function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "mkdir") == 0){
         	if(testPrints)
 		       printf("call mkdir function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "open") == 0){
        	if(testPrints)
	       		printf("call open function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "close") == 0){
                if(testPrints)
			printf("call close function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "read") == 0){
        	if(testPrints)
			printf("call read function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "write") == 0){
        	if(testPrints)
		       printf("call write function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "rm") == 0){
        	 if(testPrints)
			printf("call rm function\n");
	}
	else if(strcmp(instr_ptr->tokens[0], "rmdir") == 0){
         	if(testPrints)
		      printf("call rmdir function\n");
	}
	else
		printf("input invalid\n");
}
/* PART 1 - 13 */
void info(boot_sector_struct* bs_ptr){
	//TODO::finish
	if(testPrints)
		printf("inside boot function\n");
	int i;
	for(i = 0; i < 3;i++)
		printf("BS_jmpBoot: 0x%X\n", bs_ptr->BS_jmpBoot[i]);
	printf("BS_OEMName: ");
	for(i; i < 11;i++){
		printf("%c", bs_ptr->BS_OEMName[i-3]);
	}
	printf("\n");
	printf("BPB_BytsPerSec: 0x%X\n", bs_ptr->BPB_BytsPerSec);
}

void func_exit(){
	//TODO::finish
	if(testPrints)
                printf("inside exit function\n");
	exit(0);
}
/* END PART 1 - 13 */

/* FUNCTION PARSES BOOT SECTOR */
boot_sector_struct * bootSectorParse(){
	if(testPrints)
		printf("inside parse boot sector function\n");
	int i, c;
	unsigned int byteOne, byteTwo;
	FILE* file = fopen(imagePath, "r+");
	if(!file){
		printf("error opening %s\nexiting...\n",imagePath);
		exit(1);
	}	
	boot_sector_struct * bs_ptr = malloc(sizeof(boot_sector_struct));
	for(i = 0; i < 3;i++){
		if(i < 3){
			bs_ptr->BS_jmpBoot[i] = fgetc(file);	
		}
	}
	for(i; i < 11;i++){
		bs_ptr->BS_OEMName[i-3] = fgetc(file);	
	}
	byteOne = fgetc(file);
	byteTwo = fgetc(file);
	i += 2;	//i = 13
	bs_ptr->BPB_BytsPerSec =((((byteOne) >> 8) & 0x00FF) | (((byteTwo) << 8) & 0xFF00) );
	fclose(file);
	
	return bs_ptr;
}
/* END BOOTSECTORPARSE() FUNCTION*/

/* PARSING FUNCTIONS */
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}
/* END PARSING FUNCTIONS */
