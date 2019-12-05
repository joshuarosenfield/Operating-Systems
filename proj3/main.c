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
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint16_t BPB_FSVer;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	uint8_t BPB_Reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
} __attribute__((packed)) boot_sector_struct;

typedef struct
{
	uint8_t DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
} __attribute__((packed)) directory_struct;
/* END STRUCT DEFINITIONS */

/* 1 TO PRINT TEST PRINTOUTS */
int testPrints = 0;

/* GLOBAL VARIABLES */
boot_sector_struct* bootSector;
char* imagePath;
int clusterLocation;
/* END GLOBAL VARIABLES */

/* FUNCTION DEFINITIONS */
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
void executeCommand(instruction* instr_ptr);
void func_exit();
void info(boot_sector_struct* bs);
void size(char * FILENAME);
void ls(char* DIRNAME);
void cd(char* DIRNAME);
void creat(char* FILENAME);
void mkdir(char* DIRNAME);
void rm(char* FILENAME);
void rmdir(char* DIRNAME);
boot_sector_struct* bootSectorParse(void);
directory_struct* directoryParse(int);
int clusterToValue(int cluster);
int firstDataSector(void);
int FirstSectorofCluster(unsigned int N);
int findOffset(char* NAME);
int offsetOfFreeSpaceInCluster(int cluster);
int littleEndian(int num);
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
	clusterLocation = bootSector->BPB_RootClus;
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
		size(instr_ptr->tokens[1]);
	}
	else if(strcmp(instr_ptr->tokens[0], "ls") == 0){
        	if(testPrints)
		        printf("call ls function\n");
		ls(instr_ptr->tokens[1]);
	}
	else if(strcmp(instr_ptr->tokens[0], "cd") == 0){
        	if(testPrints)
		       printf("call cd function\n");
		cd(instr_ptr->tokens[1]);
	}
	else if(strcmp(instr_ptr->tokens[0], "creat") == 0){
         	if(testPrints)
		      printf("call creat function\n");
		creat(instr_ptr->tokens[1]);
	}
	else if(strcmp(instr_ptr->tokens[0], "mkdir") == 0){
         	if(testPrints)
 		       printf("call mkdir function\n");
		mkdir(instr_ptr->tokens[1]);
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
		rm(instr_ptr->tokens[1]);
	}
	else if(strcmp(instr_ptr->tokens[0], "rmdir") == 0){
         	if(testPrints)
		      printf("call rmdir function\n");
		rmdir(instr_ptr->tokens[1]);
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
	for(i; i < 11;i++)
		printf("%c", bs_ptr->BS_OEMName[i-3]);
	printf("\n");
	printf("BPB_BytsPerSec: 0x%X\n", bs_ptr->BPB_BytsPerSec);
	printf("BPB_SecPerClus: 0x%X\n", bs_ptr->BPB_SecPerClus);
	printf("BPB_RsvdSecCnt: 0x%X\n", bs_ptr->BPB_RsvdSecCnt);
	printf("BPB_NumFATs: 0x%X\n", bs_ptr->BPB_NumFATs);
	printf("BPB_RootEntCnt: 0x%X\n", bs_ptr->BPB_RootEntCnt);
	printf("BPB_TotSec16: 0x%X\n", bs_ptr->BPB_TotSec16);
	printf("BPB_Media: 0x%X\n", bs_ptr->BPB_Media);
	printf("BPB_FATSz16: 0x%X\n", bs_ptr->BPB_FATSz16);
	printf("BPB_SecPerTrk: 0x%X\n", bs_ptr->BPB_SecPerTrk);
	printf("BPB_NumHeads: 0x%X\n", bs_ptr->BPB_NumHeads);
	printf("BPB_HiddSec: 0x%X\n", bs_ptr->BPB_HiddSec);
	printf("BPB_TotSec32: 0x%X\n", bs_ptr->BPB_TotSec32);
	printf("BPB_FATSz32: 0x%X\n", bs_ptr->BPB_FATSz32);
	printf("BPB_ExtFlags: 0x%X\n", bs_ptr->BPB_ExtFlags);
	printf("BPB_FSVer: 0x%X\n", bs_ptr->BPB_FSVer);
	printf("BPB_RootClus: 0x%X\n", bs_ptr->BPB_RootClus);
	printf("BPB_FSInfo: 0x%X\n", bs_ptr->BPB_FSInfo);
	printf("BPB_BkBootSec: 0x%X\n", bs_ptr->BPB_BkBootSec);
	for(i = 0; i < 12;i++)
                printf("BPB_Reserved: 0x%X\n", bs_ptr->BPB_Reserved[i]);
	printf("BS_DrvNum: 0x%X\n", bs_ptr->BS_DrvNum);
	printf("BS_Reserved1: 0x%X\n", bs_ptr->BS_Reserved1);
        printf("BS_BootSig: 0x%X\n", bs_ptr->BS_BootSig);
        printf("BS_VolID: 0x%X\n", bs_ptr->BS_VolID);
	for(i = 0; i < 11;i++)
                printf("BS_VolLab: 0x%X\n", bs_ptr->BS_VolLab[i]);
	printf("BS_OEMName: ");
        for(i = 0; i < 8;i++)
                printf("%c", bs_ptr->BS_FilSysType[i]);
	printf("\n");
}

void func_exit(){
	//TODO::finish
	if(testPrints)
                printf("inside exit function\n");
	exit(0);
}

void size(char * FILENAME){
	//TODO::add capabilites for directory searching
	if(testPrints)
                printf("inside size function with %s as input\n", FILENAME);	
	if(FILENAME == '\0'){
                printf("ERROR NO IMPUT\n");
		return;
        }
	int offset = findOffset(FILENAME);
    	if(offset == -1){
		printf("FILE %s NOT FOUND\n", FILENAME);
		return;
	}
	directory_struct * dir_ptr = directoryParse(offset);	
        printf("Size of %s: %X\n", FILENAME, dir_ptr->DIR_FileSize);
       	free(dir_ptr);    	
}


void ls(char* DIRNAME){
	//TODO::add capabilites for directory searching
	if(testPrints)
                printf("inside ls function with %s as input\n", DIRNAME);
	int i, offset, total, nextCluster;
	nextCluster = clusterLocation;
	directory_struct* dir_ptr = malloc(sizeof(directory_struct));
	if(DIRNAME == NULL){
    		do{
			offset = FirstSectorofCluster(nextCluster) * bootSector->BPB_BytsPerSec;
    			total = offset + bootSector->BPB_BytsPerSec;
			if(testPrints)
				printf("offset = %d\n", offset);
    			while(offset < total){
        			dir_ptr = directoryParse(offset);
//<<<<<<< HEAD
				if(dir_ptr->DIR_Attr == 0x10 ||
						dir_ptr->DIR_Attr == 0x20){
        				if(dir_ptr->DIR_Attr == 0)
            					break;		
        				for(i = 0;i < 11; i++)
            					printf("%c",dir_ptr->DIR_Name[i]);
        				printf("\n");

				}
				offset += 32;
    			}
//=======
//>>>>>>> a5929b3de03fa6ad403d2a4ea67a75fd8e9a2d94
			nextCluster = clusterToValue(nextCluster);
		}while(nextCluster != 0x0FFFFFFFF);
	free(dir_ptr);
	}
}

void cd(char* DIRNAME){
	if(testPrints)
                printf("inside cd function with %s as input\n", DIRNAME);
	if(DIRNAME == '\0'){
		clusterLocation = 2;
		return;
	}
	int offset = findOffset(DIRNAME);
        if(offset == -1){
                printf("DIRECTORY %s NOT FOUND\n", DIRNAME);
                return;
        }
        directory_struct * dir_ptr = directoryParse(offset);
        if (dir_ptr->DIR_Attr != 16){
		printf("DIRECTORY %s NOT FOUND\n", DIRNAME);
                free(dir_ptr);
		return;
	}
	clusterLocation = dir_ptr->DIR_FstClusHI << 16 | dir_ptr->DIR_FstClusLO;
	if(testPrints)
		printf("cluster location %d\n", clusterLocation);
	free(dir_ptr);
	
}
//TODO::EXPAND IF NO SPACE
void creat(char* FILENAME){
	if(testPrints)
                printf("inside creat function with %s as input\n", FILENAME);
	//check if file already exists
	if(findOffset(FILENAME) != -1){
		printf("FILE %s ALREADY EXISTS\n", FILENAME);
		return;
	}
	//check if avaliable sector
	int fileLength, end, sectorOffset, clusterReplace, offset, i, FATIndex;
        directory_struct * dir_ptr = malloc(sizeof(directory_struct));
	fileLength = strlen(FILENAME);
	//gets a cluster location
	i = bootSector->BPB_RootClus;
        end = 0x0FFFFFF8; //Changed to match example
        while (clusterToValue(i) != 0)
                i++;
        clusterReplace = FATIndex = i;
	//offsettoindex = start of fat + the cluster offset
        int offsetToIndex = (bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec) + (4 * FATIndex);
        FILE* file_ptr = fopen(imagePath, "r+b");
        fseek(file_ptr, offsetToIndex, SEEK_SET);
        fwrite(&end, 4, 1, file_ptr);
        fclose(file_ptr);
        //end a cluster location
        //using clusterReplace below
	

	sectorOffset = offsetOfFreeSpaceInCluster(clusterLocation);
	if(testPrints)
		printf("free sector offset is %d\n", sectorOffset);
	if(sectorOffset == -1){
		//TODO::need to expand	
		printf("CANT ADD BECAUSE FULL\n");
		return;
	}
	else
		offset = sectorOffset;
	
	FILE* file_ptr2 = fopen(imagePath, "r+b");
        fseek(file_ptr2, offset, SEEK_SET);
	
	i = 0;
	while(i != 11){
		if (i < fileLength)
			dir_ptr->DIR_Name[i] = toupper(FILENAME[i]);
		else
			dir_ptr->DIR_Name[i] = 0x20;	//0x20 = .
		i += 1;
	}

	dir_ptr->DIR_FstClusLO = clusterReplace & 0x0000FFFF;
        dir_ptr->DIR_FstClusHI = (clusterReplace & 0xFFFF0000) >> 16;
        dir_ptr->DIR_FileSize = 0;
        dir_ptr->DIR_Attr = 0x20;
	dir_ptr->DIR_CrtDate = 0x0;

	fwrite(dir_ptr, sizeof(directory_struct), 1, file_ptr2);
        
	fclose(file_ptr2);
        free(dir_ptr);
		
	//printf("ERROR CREATING FILE %s\n", FILENAME);
}
void mkdir(char* DIRNAME){
	if(testPrints)
                printf("inside mkdir function with %s as input\n", DIRNAME);
        //check if file already exists
        if(findOffset(DIRNAME) != -1){
                printf("DIRECTORY %s ALREADY EXISTS\n", DIRNAME);
                return;
        }
        //check if avaliable sector
        int fileLength, end, sectorOffset, clusterReplace, dotCluster, dotdotCluster, offset, i, FATIndex;
        directory_struct * dir_ptr = malloc(sizeof(directory_struct));
        fileLength = strlen(DIRNAME);
        //gets a cluster location
        i = bootSector->BPB_RootClus;
        end = 0x0FFFFFF8; //Changed to match example
        while (clusterToValue(i) != 0)
                i++;
        clusterReplace = FATIndex = i;
	++i;
	//Get clusters for . and ..
	while(clusterToValue(i) != 0)
		++i;
	dotCluster = i;
	++i;
	while(clusterToValue(i) != 0)
		++i;
	dotdotCluster = i;
        //offsettoindex = start of fat + the cluster offset
        int offsetToIndex = (bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec) + (4 * FATIndex);
	int dotOffset = (bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec) + (4 * dotCluster);
	int dotdotOffset = (bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec) + (4 * dotdotCluster);

        FILE* file_ptr = fopen(imagePath, "r+b");
        fseek(file_ptr, offsetToIndex, SEEK_SET);
        fwrite(&dotCluster, 4, 1, file_ptr); //Point to .
	fseek(file_ptr, dotOffset, SEEK_SET);
	fwrite(&dotdotCluster, 4, 1, file_ptr); //Point to ..
	fseek(file_ptr, dotdotOffset, SEEK_SET);
	fwrite(&end, 4, 1, file_ptr); //End
        fclose(file_ptr);

        //end a cluster location
        //using clusterReplace below
	sectorOffset = offsetOfFreeSpaceInCluster(clusterLocation);
        if(testPrints)
                printf("free sector offset is %08X\n", sectorOffset);
        if(sectorOffset == -1){
                //TODO::need to expand  
                printf("CANT ADD FILE BECAUSE FULL\n");
                return;
        }
        else
                offset = sectorOffset;
        FILE* file_ptr2 = fopen(imagePath, "r+b");
        fseek(file_ptr2, offset, SEEK_SET);
        i = 0;

        while(i != 11){
                if (i < fileLength)
                        dir_ptr->DIR_Name[i] = toupper(DIRNAME[i]);
                else
                        dir_ptr->DIR_Name[i] = 0x20;
                i += 1;
        }

        dir_ptr->DIR_FstClusLO = clusterReplace & 0x0000FFFF;
        dir_ptr->DIR_FstClusHI = (clusterReplace & 0xFFFF0000) >> 16;
        dir_ptr->DIR_FileSize = 0;
        dir_ptr->DIR_Attr = 0x10;

        fwrite(dir_ptr, sizeof(directory_struct), 1, file_ptr2);

	//Create . directory
	sectorOffset = offsetOfFreeSpaceInCluster(dotCluster);
	if(sectorOffset == -1){
                //TODO::need to expand
                printf("CANT ADD FILE BECAUSE FULL\n");
                return;
        }
        else
                offset = sectorOffset;


	fseek(file_ptr2, offset, SEEK_SET);

	i = 0;
	while(i != 11){
		dir_ptr->DIR_Name[i] = 0x20;
		++i;
	}
	dir_ptr->DIR_Name[0] = (char)'.';
	dir_ptr->DIR_FstClusLO = dotCluster & 0x0000FFFF;
        dir_ptr->DIR_FstClusHI = (dotCluster & 0xFFFF0000) >> 16;
	dir_ptr->DIR_FileSize = 0;
	dir_ptr->DIR_Attr = 0x10;

	fwrite(dir_ptr, sizeof(directory_struct), 1, file_ptr2);

	//Create .. directory
	sectorOffset = offsetOfFreeSpaceInCluster(dotdotCluster);
        if(sectorOffset == -1){
                //TODO::need to expand
                printf("CANT ADD FILE BECAUSE FULL\n");
                return;
        }
        else
                offset = sectorOffset;


        fseek(file_ptr2, offset, SEEK_SET);

        i = 0;
        while(i != 11){
                dir_ptr->DIR_Name[i] = 0x20;
                ++i;
        }
        dir_ptr->DIR_Name[0] = (char)'.';
	dir_ptr->DIR_Name[1] = (char)'.';
	//Change to parent directory
        dir_ptr->DIR_FstClusLO = dotCluster & 0x0000FFFF;
        dir_ptr->DIR_FstClusHI = (dotCluster & 0xFFFF0000) >> 16;
        dir_ptr->DIR_FileSize = 0;
        dir_ptr->DIR_Attr = 0x10;

        fwrite(dir_ptr, sizeof(directory_struct), 1, file_ptr2);

        fclose(file_ptr2);
	free(dir_ptr);
}
void rm(char* FILENAME){
	if(testPrints)
                printf("inside rm function with %s as input\n", FILENAME);
	int offset, i;
        directory_struct * dir_ptr = malloc(sizeof(directory_struct));

	offset = findOffset(FILENAME);
        //printf("%d\n", offset);
	if(offset == -1){
		printf("INPUT %s NOT FOUND\n", FILENAME);
                return;
	}
        dir_ptr = directoryParse(offset);
	if(dir_ptr->DIR_Attr == 16){
		printf("INPUT IS NOT FILE\n");
        	free(dir_ptr);
		return;
	}
	
	FILE* file_ptr = fopen(imagePath, "r+b");
	fseek(file_ptr, offset, SEEK_SET);	
        
	i = 0;

       while(i < 64){
		fseek(file_ptr, offset, SEEK_SET);
		fputc(0x0, file_ptr);
		i += 1;
		offset += 1;
	}
	fclose(file_ptr);
        free(dir_ptr);

}

void rmdir(char* DIRNAME){
	if(testPrints)
                printf("inside rmdir function with %s as input\n", DIRNAME);
	int offset, i;
        directory_struct * dir_ptr = malloc(sizeof(directory_struct));


        offset = findOffset(DIRNAME);
        //printf("%d\n", offset);
        if(offset == -1){
                printf("INPUT %s NOT FOUND\n", DIRNAME);
                return;
        }
        dir_ptr = directoryParse(offset);
	if(dir_ptr->DIR_Attr != 16){
                printf("INPUT IS NOT DIRECTORY\n");
                free(dir_ptr);
                return;
        }
        FILE* file_ptr = fopen(imagePath, "r+b");
        fseek(file_ptr, offset, SEEK_SET);

        i = 0;

       while(i < 64){
                fseek(file_ptr, offset, SEEK_SET);
                fputc(0x0, file_ptr);
                i += 1;
                offset += 1;
        }
        fclose(file_ptr);
        free(dir_ptr);
}
/* END PART 1 - 13 */

/*GET LOCATION OF FREE SPACE FROM INPUTTED CLUSTER, -1 IF NONE*/
int offsetOfFreeSpaceInCluster(int cluster){
    	int result, offset, nextCluster, total;
	nextCluster = cluster;
	directory_struct* dir_ptr = malloc(sizeof(directory_struct));
	do{
		//find first free cluster and save in result
       		//result = offsetFreeSpaceInClusterNonrecursive(cluster);
        
		//int targetedOffset = offsetOfFirstSectorOfCluster(clusterNumber);
   		offset = FirstSectorofCluster(nextCluster) * bootSector->BPB_BytsPerSec;
    		total = offset + bootSector->BPB_BytsPerSec;
		result = -1;
		while(offset < total){
       			dir_ptr = directoryParse(offset);
       			if(dir_ptr->DIR_Attr == 0){
            			result = offset;
        			break;
			}
        		else{
            			offset += 64;
       			}
    		}

		if(result > 0){
        		free(dir_ptr);
			return result;

		}
		nextCluster = clusterToValue(nextCluster);
    	}while(nextCluster != 0x0FFFFFFFF);
	free(dir_ptr);
		//use result
	//		printf("free sector offset is %d\n", result);
	return result;
}
/*END */

/*FIND OFFSET TO NAME*/
int findOffset(char* NAME){
 	int offset, i, total;
	offset = FirstSectorofCluster(clusterLocation) * bootSector->BPB_BytsPerSec;
	directory_struct* dir_ptr = malloc(sizeof(directory_struct));   
	total = offset + bootSector->BPB_BytsPerSec;
	while(offset < total){
		dir_ptr = directoryParse(offset);
        	if(dir_ptr->DIR_Attr == 0){
            		offset += 32;
			continue;
        	}
        	for(i = 0;i < 11; i++){				
			if(NAME[i] == '\0' && dir_ptr->DIR_Name[i] == 32){
                		free(dir_ptr);
				return offset;
            		}
			else if(toupper(NAME[i]) != dir_ptr->DIR_Name[i]){
				break;
			}
        	}
        	offset += 32; 	
    	}

        free(dir_ptr);
	return -1;
}
/*END OF FINDOFFSET() */

/*FIRST SECTOR OF CLUSTER FUNCTION */
int FirstSectorofCluster(unsigned int N){
    return firstDataSector() + ((N - 2) * bootSector->BPB_SecPerClus);
}
/*END OF FIRSTSECTOROFCLUSTERFUNCTION() */

/*FUNCTION RETURNS SECTOR WHERE DATA STARTS*/
int firstDataSector(){
	if(testPrints)
                printf("inside of firstDataSector() have to multiple by SecSize to get offset\n");
	return(bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz32));
}
/*END OF FIRSTDATASECTOR()*/
 
/*FUNCTION TAKES A CLUSTER NUMBER AND RETURNS ITS VALUE */
int clusterToValue(int cluster){
	if(testPrints)
		printf("inside of clusterToValue()\n");
	int i, start, FATOffset, value, itr, shift;
	
	value = 0;
        start = bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec; //start of FAT at reseveredsec * size
        FATOffset = cluster * 4; 	//N * 4
        itr = start + FATOffset;		//start of inputed cluster at beginning of FAT + FATOffset

        FILE* file = fopen(imagePath, "r+");
        fseek(file, itr, SEEK_SET);	//SEEK_SET IS BEGINNING OF FILE
        for(i = 0; i < 4; i++){
		shift = fgetc(file) << (i * 8);
                value = value | shift;
        }

	fclose(file);
	return value;
}
/*END OF CLUSTERTOVALUE() FUNCTION*/
int littleEndian(int val){
	int b1, b2, b3, b4;
	b1 = (val & 0x000000FF) << 24;
	b2 = (val & 0x0000FF00) << 8;
	b3 = (val & 0x00FF0000) >> 8;
	b4 = (val & 0xFF000000) >> 24;
	return b1 | b2 | b3 | b4;
}
/* FUNCTION PARSES BOOT SECTOR */
boot_sector_struct * bootSectorParse(){
	if(testPrints)
		printf("inside parse boot sector function\n");
	int i, c;
	unsigned int byteOne, byteTwo, byteThree, byteFour;
	FILE* file = fopen(imagePath, "r+");
	if(!file){
		printf("error opening %s\nexiting...\n",imagePath);
		exit(1);
	}	
	boot_sector_struct * bs_ptr = malloc(sizeof(boot_sector_struct));
	
	for(i = 0; i < 3;i++)
			bs_ptr->BS_jmpBoot[i] = fgetc(file);	
	
	for(i = 0; i < 8;i++)
		bs_ptr->BS_OEMName[i] = fgetc(file);	
	
	
	byteOne = fgetc(file);
	byteTwo = fgetc(file);
	bs_ptr->BPB_BytsPerSec = ((byteOne) | ((byteTwo) << 8));//( (((byteOne) >> 8) & 0x00FF) | (((byteTwo) << 8) & 0xFF00) );
	
	bs_ptr->BPB_SecPerClus = fgetc(file);
	
	byteOne = fgetc(file);
        byteTwo = fgetc(file);
        bs_ptr->BPB_RsvdSecCnt = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	
	bs_ptr->BPB_NumFATs = fgetc(file);	//16
	
	byteOne = fgetc(file);//17
        byteTwo = fgetc(file);//18
        bs_ptr->BPB_RootEntCnt = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//19
        byteTwo = fgetc(file);//20
        bs_ptr->BPB_TotSec16 = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	bs_ptr->BPB_Media = fgetc(file);//22

	byteOne = fgetc(file);//22
        byteTwo = fgetc(file);//23
        bs_ptr->BPB_FATSz16 = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//24
        byteTwo = fgetc(file);//25
        bs_ptr->BPB_SecPerTrk = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//26
        byteTwo = fgetc(file);//27
        bs_ptr->BPB_NumHeads = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//28
        byteTwo = fgetc(file);//29
	byteThree = fgetc(file);//30
	byteFour = fgetc(file);//31
        bs_ptr->BPB_HiddSec = (byteFour<<24) | (byteThree<<16) | (byteTwo<<8) | byteOne; 

	byteOne = fgetc(file);//32
        byteTwo = fgetc(file);//33
        byteThree = fgetc(file);//34
        byteFour = fgetc(file);//35
        bs_ptr->BPB_TotSec32 = (byteFour<<24) | (byteThree<<16) | (byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//36
        byteTwo = fgetc(file);//37
        byteThree = fgetc(file);//38
        byteFour = fgetc(file);//39
        bs_ptr->BPB_FATSz32 = (byteFour<<24) | (byteThree<<16) | (byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//40
        byteTwo = fgetc(file);//41
        bs_ptr->BPB_ExtFlags = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	
	byteOne = fgetc(file);//42
        byteTwo = fgetc(file);//43
        bs_ptr->BPB_FSVer = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//44
        byteTwo = fgetc(file);//45
        byteThree = fgetc(file);//46
        byteFour = fgetc(file);//47
        bs_ptr->BPB_RootClus = (byteFour<<24) | (byteThree<<16) | (byteTwo<<8) | byteOne;

	byteOne = fgetc(file);//48
        byteTwo = fgetc(file);//49
        bs_ptr->BPB_FSInfo = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	
	byteOne = fgetc(file);//50
        byteTwo = fgetc(file);//51
        bs_ptr->BPB_BkBootSec = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;

	for(i = 0; i < 12;i++)
                bs_ptr->BPB_Reserved[i] = fgetc(file);//52-63

	bs_ptr->BS_DrvNum = fgetc(file);      //64

	bs_ptr->BS_Reserved1 = fgetc(file); //65

	bs_ptr->BS_BootSig = fgetc(file); //66

	byteOne = fgetc(file);//67
        byteTwo = fgetc(file);//68
        byteThree = fgetc(file);//69
        byteFour = fgetc(file);//70
        bs_ptr->BS_VolID = (byteFour<<24) | (byteThree<<16) | (byteTwo<<8) | byteOne;
	
	for(i = 0; i < 11;i++)
                bs_ptr->BS_VolLab[i] = fgetc(file);//71-81

	for(i = 0; i < 8;i++)
                bs_ptr->BS_FilSysType[i] = fgetc(file);	

	fclose(file);
	
	return bs_ptr;
}
/* END BOOTSECTORPARSE() FUNCTION*/

/* FUNCTION PARSES DIRECTORY*/
directory_struct * directoryParse(int offset){
	if(testPrints)
                printf("inside parse directory function\n");
	int i;
	unsigned int byteOne, byteTwo, byteThree, byteFour, ignore;
	directory_struct * dir_ptr = malloc(sizeof(directory_struct));
	FILE* file = fopen(imagePath, "r+");
        if(!file){
                printf("error opening %s\nexiting...\n",imagePath);
                exit(1);
        }
	fseek(file, offset, SEEK_SET);
	for(i = 0;i < 32;i++)
		ignore = fgetc(file);
	for(i = 0; i < 11;i++)
                dir_ptr->DIR_Name[i] = fgetc(file);	//0-10
	dir_ptr->DIR_Attr = fgetc(file);		//11
	dir_ptr->DIR_NTRes = fgetc(file);		//12
	dir_ptr->DIR_CrtTimeTenth = fgetc(file);	//13
	byteOne = fgetc(file);				//14
	byteTwo = fgetc(file);				//15
	dir_ptr->DIR_CrtTime = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);                          //16
        byteTwo = fgetc(file);                          //17
        dir_ptr->DIR_CrtDate = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);                          //18
        byteTwo = fgetc(file);                          //19
        dir_ptr->DIR_LstAccDate = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);                          //20
        byteTwo = fgetc(file);                          //21
        dir_ptr->DIR_FstClusHI = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);                          //22
        byteTwo = fgetc(file);                          //23
        dir_ptr->DIR_WrtTime = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
       	byteOne = fgetc(file);                          //24
        byteTwo = fgetc(file);                          //25
        dir_ptr->DIR_WrtDate = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);                          //26
        byteTwo = fgetc(file);                          //27
        dir_ptr->DIR_FstClusLO = ((byteOne) | ((byteTwo) << 8)); //(byteTwo<<8) | byteOne;
	byteOne = fgetc(file);				//28
        byteTwo = fgetc(file);				//29
        byteThree = fgetc(file);			//30
        byteFour = fgetc(file);				//31
       	dir_ptr->DIR_FileSize = (byteFour << 24) | (byteThree << 16) | (byteTwo << 8) | byteOne;

	fclose(file);
	return dir_ptr;
}
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
