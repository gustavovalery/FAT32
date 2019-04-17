/* Project 3
 * Casey Kwatkosky
 * Camila Rios
 * Emilio Sillart */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#define LBA_BEGIN 0


typedef struct{
	char** tokens;
	int numTokens;
	char c;
}instruction;

typedef struct Boot_Sector_Info{
	//this is still a hex value
	unsigned char BS_jmpBoot[3];
	//this is a string
	unsigned char BS_OEMName[8];

	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATS;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;

	/*START OF SECOND TABLE FOR FAT32*/
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint16_t BPB_FSVer;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	//read in 12 bytes of ints as unsigned chars and later print with "%02X"
	unsigned char BPB_Reserved[12];

	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	//this is a string but represented in hex
	unsigned char BS_VolLab[11];
	//this is also a string but represented in hex
	unsigned char BS_FilSysType[8];
} __attribute__((packed)) boot_sector_info;

typedef struct File_Info{

	unsigned char filename[11];
	uint8_t attribute;
	uint8_t skip1[8];
	uint16_t high_cluster;
	uint8_t skip2[4];
	uint16_t low_cluster;
	uint32_t file_size;
} __attribute__((packed)) file_info; 

struct File_Info entries[16];

int i;
uint32_t DWORD;
uint8_t SecBuff[4];
uint32_t nextClusNum;


/* Prototypes! */
int my_exit(int status, FILE *fptr);
int ls_dir();
void info(boot_sector_info BSI);
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
unsigned long fat_begin_lba(boot_sector_info BSI);
unsigned long cluster_begin_lba(boot_sector_info BSI);
int my_exit(int status, FILE *fptr);
void print_file_info(file_info FI);
unsigned long lba_addr(int cluster_number, boot_sector_info BSI);
void print_file_info(file_info FI);
unsigned long find_first_data_sector(boot_sector_info BSI);
unsigned long find_first_sector_of_cluster(boot_sector_info BSI, int N);
int count_sectors_in_data_region(boot_sector_info BSI);
uint32_t find_FAT_sector_number(boot_sector_info BSI, int N);
int find_FAT_entry_offset(boot_sector_info BSI, int N);

/*---------------------------------------------------------*/

int main(int argc, char *argv[]){
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	if(argc < 2){
		printf("ERROR: wrong number of arguments!\n");
		exit(1);
	}

	printf("%s was the argument\n\n",argv[1]);

	FILE *fptr;
	struct Boot_Sector_Info BSI;
	struct File_Info FI;

	fptr = fopen(argv[1],"rb");

	if(!fptr){
		printf("ERROR: fptr is invalid!\n\n");
		my_exit(1, fptr);
	}

	/*using fread() on ((packed)) struct does most of the work for you*/
	fread(&BSI, sizeof(struct Boot_Sector_Info), 1, fptr);

	while(1){
		printf("\nPlease enter an instruction: ");

		do {// loop reads character sequences separated by whitespace

			//scans for next token and allocates token var to size of scanned token
			scanf( "%ms", &token);
			//allocate temp variable with same size as token
			temp = (char*)malloc((strlen(token)+1) * sizeof(char));
			int i;
			int start;

			start = 0;

			for (i = 0; i < strlen(token); i++){
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0){
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
			if (start < strlen(token)){
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);			
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;

		}while('\n' != getchar());    //until end of line is reached

		printTokens(&instr);

		if((strcmp(instr.tokens[0], "info")) == 0){
			info(BSI);
		}
		else if((strcmp(instr.tokens[0], "exit")) == 0){
			my_exit(0, fptr);
		}
		else if((strcmp(instr.tokens[0],"ls")) == 0){
			int N = 2;
			unsigned long FirstSectorofCluster = find_first_data_sector(BSI) + ((N - 2) * BSI.BPB_SecPerClus);


			int flag = 0;
			int clusterNum = 3;

			int offset;
			int j = 0;

			/*
			while(flag == 0){
				nextClusNum = find_FAT_sector_number(BSI,clusterNum) * BSI.BPB_BytesPerSec + find_FAT_entry_offset(BSI, clusterNum); 
				fseek(fptr, nextClusNum, SEEK_SET);
				fread(&SecBuff, 1, 2, fptr);		

				for(i = 0; i < 4; i++){
					printf("%02X", SecBuff[i]);
				}

				printf("\n");			

				clusterNum = SecBuff[0];

				if(SecBuff[0] == 0xF8)
					flag = 1;
			}

			
			nextClusNum = find_FAT_sector_number(BSI,2) * BSI.BPB_BytesPerSec + find_FAT_entry_offset(BSI, 2); 
			fseek(fptr, nextClusNum, SEEK_SET);
			fread(&SecBuff, 1, 2, fptr);

			for(i = 0; i < 4; i++){
				printf("%02X", SecBuff[i]);
			}

			printf("\n");
			*/

			for(offset = 32; offset <= 512; offset += 64){

				fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec + offset, SEEK_SET);
				fread(&entries[j], sizeof(struct File_Info), 1, fptr);

				print_file_info(entries[j]);

				j++;		
			}




			/*
			   unsigned int offset = lba_addr(3, BSI);
			   fseek(fptr,offset, SEEK_SET);
			   fread(&FI,sizeof(struct File_Info),1,fptr);
			   print_file_info(FI);
			   */
		}
		else{
			clearInstruction(&instr);
		}

		clearInstruction(&instr);
	}
	my_exit(0, fptr);
	return 0;
}


/*---------------------------------PART 1: EXIT[2] ---------------------------------------------------*/
int my_exit(int status, FILE *fptr){
	/*exit the program and clean up space*/
	printf("\ncalling my_exit with status: %d\n",status);
	//free up space from fopen
	fclose(fptr);
	exit(status);
}

int ls_dir(){

}

/*----------------------------------------- PART 2: INFO[5]------------------------------------------ */
void info(boot_sector_info BSI){

	size_t n = sizeof(BSI.BS_jmpBoot)/sizeof(BSI.BS_jmpBoot[0]);
	int i;
	printf("Jump Boot (should have EB): ");
	for(i = 0; i < n; i++){
		printf("%02X", BSI.BS_jmpBoot[i]);
	}
	printf("\n");

	printf("BS_OEMName: %s\n", BSI.BS_OEMName);
	printf("BPB_BytesPerSec: %d\n", BSI.BPB_BytesPerSec);
	printf("BPB_SecPerClus: %d\n", BSI.BPB_SecPerClus);
	printf("BPB_RsvdSecCnt: %d\n", BSI.BPB_RsvdSecCnt);
	printf("BPB_NumFATS (must be 2): %d\n", BSI.BPB_NumFATS);
	printf("BPB_RootEntCnt (must be 0): %d\n", BSI.BPB_RootEntCnt);
	printf("BPB_TotSec16: %d\n", BSI.BPB_TotSec16);
	printf("BPB_Media: %d\n", BSI.BPB_Media);
	printf("BPB_FATSz16 (should be 0): %d\n", BSI.BPB_FATSz16);
	printf("BPB_SecPerTrk: %d\n", BSI.BPB_SecPerTrk);
	printf("BPB_NumHeads: %d\n", BSI.BPB_NumHeads);
	printf("BPB_HiddSec: %d\n", BSI.BPB_HiddSec);
	printf("BPB_TotSec32 (must be non-zero): %d\n", BSI.BPB_TotSec32);
	printf("\n");
	printf("BPB_FATSz32: %d\n", BSI.BPB_FATSz32);
	printf("BPB_ExtFlags: %d\n", BSI.BPB_ExtFlags);
	printf("BPB_FSVer: %d\n", BSI.BPB_FSVer);
	printf("BPB_RootClus (usually 2 but not required): %d\n", BSI.BPB_RootClus);
	printf("BPB_FSInfo (usually 1): %d\n", BSI.BPB_FSInfo);
	printf("BPB_BkBootSec (usually 6): %d\n", BSI.BPB_BkBootSec);

	n  = sizeof(BSI.BPB_Reserved)/sizeof(BSI.BPB_Reserved[0]);
	printf("BPB_Reserved (all bytes for FAT32 should be set to 0): ");
	for(i = 0; i < n; i++){
		printf("%02X", BSI.BPB_Reserved[i]);
	}
	printf("\n");

	printf("BS_DrvNum: %d\n", BSI.BS_DrvNum);
	printf("BS_Reserved1: %d\n", BSI.BS_Reserved1);
	printf("BS_BootSig: %d\n", BSI.BS_BootSig);
	printf("BS_VolID: %d\n", BSI.BS_VolID);

	n = sizeof(BSI.BS_VolLab)/sizeof(BSI.BS_VolLab[0]);
	printf("BS_VolLab (should be NO NAME [convert from Hex to String]): ");
	for(i = 0; i < n; i++){
		printf("%02X", BSI.BS_VolLab[i]);
	}
	printf("\n");

	n = sizeof(BSI.BS_FilSysType)/sizeof(BSI.BS_FilSysType[0]);
	printf("BS_FilSysType (should be FAT32 [convert from Hex to String]): ");
	for(i = 0; i < n; i++){
		printf("%02X", BSI.BS_FilSysType[i]);
	}
	printf("\n");

}

void print_file_info(file_info FI){
	printf("\n**PRINTING FILE INFO**\n");

	size_t n = sizeof(FI.filename)/sizeof(FI.filename[0]);
	int i;

	if(FI.filename[0] == 46){
		printf("THIS IS A DIRECTORY\n");
	}

	printf("DIR filename is: ");
	for(i = 0; i < n; i++){
		printf("%c", FI.filename[i]);
	}
	printf("\n");

	printf("DIR attribute is: 0x%02X\n", FI.attribute);
	printf("DIR HI cluster is: %d\n", FI.high_cluster);
	printf("DIR LO cluster is: %d\n", FI.low_cluster);
	printf("DIR file size is: %d\n", FI.file_size);
}

unsigned long fat_begin_lba(boot_sector_info BSI){
	return LBA_BEGIN + BSI.BPB_RsvdSecCnt;
}

unsigned long cluster_begin_lba(boot_sector_info BSI){
	return LBA_BEGIN +(BSI.BPB_RsvdSecCnt * BSI.BPB_BytesPerSec)  + (BSI.BPB_NumFATS * BSI.BPB_FATSz32 * BSI.BPB_BytesPerSec);
}

unsigned long lba_addr(int cluster_number, boot_sector_info BSI){
	return(cluster_begin_lba(BSI) + (cluster_number - 2) * BSI.BPB_SecPerClus);
}

unsigned long find_first_data_sector(boot_sector_info BSI){
	unsigned long RootDirSectors, FirstDataSector;

	/*RootDirSectors = 0 most of the time*/
	RootDirSectors = ((BSI.BPB_RootEntCnt* 32) + (BSI.BPB_BytesPerSec-1)) / BSI.BPB_BytesPerSec;
	
	FirstDataSector = (BSI.BPB_RsvdSecCnt + (BSI.BPB_NumFATS * BSI.BPB_FATSz32) + RootDirSectors);

	printf("FirstDataSector = 0x%02X\n", FirstDataSector);

	return FirstDataSector;
}

unsigned long find_first_sector_of_cluster(boot_sector_info BSI, int N){
	/*N is the cluster number to find the sector of*/
	unsigned long FirstDataSector = find_first_data_sector(BSI);

	unsigned long FirstSectorofCluster = ((N - 2) * BSI.BPB_SecPerClus) + FirstDataSector;

	return FirstSectorofCluster;
}

int count_sectors_in_data_region(boot_sector_info BSI){
	unsigned long DataSec;
	unsigned long RootDirSectors = ((BSI.BPB_RootEntCnt* 32) + (BSI.BPB_BytesPerSec-1)) / BSI.BPB_BytesPerSec;

	DataSec = BSI.BPB_TotSec32 - (BSI.BPB_RsvdSecCnt + (BSI.BPB_NumFATS * BSI.BPB_FATSz32) + RootDirSectors);

	return DataSec;
}

uint32_t find_FAT_sector_number(boot_sector_info BSI, int N){
	/*N is the cluster number*/
	int FATOffset = N * 4;

	uint32_t ThisFATSecNum = BSI.BPB_RsvdSecCnt + (FATOffset / BSI.BPB_BytesPerSec);

	return ThisFATSecNum;
}

int find_FAT_entry_offset(boot_sector_info BSI, int N){
	/*N is the cluster number*/

	int FATOffset = N * 4;

	int ThisFATEntOffset = FATOffset % BSI.BPB_BytesPerSec;

	return ThisFATEntOffset;
}



/**************************************************************************************************************/
void addToken(instruction* instr_ptr, char* tok){
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);
	instr_ptr->numTokens++;

}

void printTokens(instruction* instr_ptr){
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++)
		printf("#%s#\n", (instr_ptr->tokens)[i]);
	printf("\n");
}

void clearInstruction(instruction* instr_ptr){
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);
	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}
