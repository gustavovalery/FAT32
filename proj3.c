/* Project 3
 * Casey Kwatkosky
 * Camila Rios
 * Emilio Sillart */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include<ctype.h>
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

	unsigned char skip0[32];
	unsigned char filename[11];
	uint8_t attribute;
	uint8_t skip1[8];
	uint16_t high_cluster;
	uint8_t skip2[4];
	uint16_t low_cluster;
	uint32_t file_size;
} __attribute__((packed)) file_info; 


struct File_Info FI;
struct Boot_Sector_Info BSI;
FILE* fptr;

unsigned int i;
uint32_t DWORD;
uint8_t SecBuff[4];
uint32_t nextClusNum;
/*this would be a cluster number*/
uint32_t currDir;

/* Prototypes! */
int my_exit(int status);
int ls_dir(unsigned char* dir_name);
int check_if_dir(unsigned char* dir_name);
int cd_dir(unsigned char* dir_name);
int find_FAT_entry_offset();
uint32_t find_FAT_sector_number();
unsigned long find_first_data_sector();
unsigned long find_next_cluster();
char* deblank(char* input);
void info();
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void print_file_info();
void print_dir_entries();
void print_clus_entries(uint32_t nextClus);

/*---------------------------------------------------------*/

int main(int argc, char *argv[]){
	/*these char pointers are for storing user input*/
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

	/*open the second argument of the executable in read-only binary*/
	fptr = fopen(argv[1],"rb");

	if(!fptr){
		printf("ERROR: fptr is invalid!\n\n");
		my_exit(1);
	}

	/*using fread() on ((packed)) struct does most of the work for you
	 * solves little endian issues
	 * reads boot sector part of fat32.img into struct*/
	fread(&BSI, sizeof(struct Boot_Sector_Info), 1, fptr);
	/*this should set it = 2 at the beginning*/
	currDir = BSI.BPB_RootClus;


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

		/*--------------------------END OF PARSING CODE------------------------------*/

		int flag1;
		
		if((strcmp(instr.tokens[0], "info")) == 0){
			info();
		}
		else if((strcmp(instr.tokens[0], "exit")) == 0){
			my_exit(0);
		}
		else if((strcmp(instr.tokens[0],"ls")) == 0){
			if(instr.numTokens == 1){
				ls_dir(".");
			}			
			else if(instr.numTokens == 2){
				flag1 = ls_dir(instr.tokens[1]);
			}
			else{
				printf("ERROR, too many arguments.");
			}
		}
		else if(strcmp(instr.tokens[0], "cd") == 0){
			if(instr.numTokens == 1){
				cd_dir(".");
			}
			else if(instr.numTokens == 2){
				flag1 = cd_dir(instr.tokens[1]);
			}
			else{
				printf("ERROR, too many arguments");
			}
		}
		else{
			clearInstruction(&instr);
		}

		clearInstruction(&instr);
	}
	my_exit(0);
	return 0;
}


/*---------------------------------PART 1: EXIT[2] ---------------------------------------------------*/

int my_exit(int status){
	/*exit the program and clean up space*/
	printf("\ncalling my_exit with status: %d\n",status);
	//free up space from fopen
	fclose(fptr);
	exit(status);
}

/*----------------------------------------- PART 2: INFO[5]------------------------------------------ */
void info(){
	/*this prints out the contents of the Boot Sector*/

	/*set n like this so it's the size of jmpBoot*/
	size_t n = sizeof(BSI.BS_jmpBoot)/sizeof(BSI.BS_jmpBoot[0]);
	int i;
	/*each index is a 2 digit hex value*/
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

/*----------------------------------------- END OF PART 2: INFO[5] ------------------------------------------ */


/*----------------------------------------- PART 3: LS DIRNAME[5] ------------------------------------------ */

int ls_dir(unsigned char* dir_name){
	int oldCurrDir = currDir;
	int flag;
	uint8_t nextClus;

	/*if the argument is the current directory, simply print entries*/
	if(strcmp(dir_name, ".") == 0){
		/*if not listing root cluster entries*/
		if(currDir != BSI.BPB_RootClus){
			printf(".\n");
		}
		print_dir_entries();
	}
	else{
		flag = check_if_dir(dir_name);		

		if(flag == -1){
			printf("ERROR: entry not a directory.");
		}
		else if (flag == 0){
			printf("ERROR: entry not found.");
		}
		else if(flag == 1){
			if(currDir != 2){
				printf(".\n");
			}			
			print_dir_entries();

			if(strcmp(dir_name, "RED") == 0){
				nextClus = find_next_cluster();	

				printf("DEBUG nextClus is = %d\n", nextClus);

				print_clus_entries(nextClus);
			}


			currDir = oldCurrDir;
		}
	}
	return flag;
}

/*----------------------------------------- END OF PART 3: LS DIRNAME[5] ------------------------------------------ */


/*----------------------------------------- PART 4: CD DIRNAME[5] ------------------------------------------ */

int cd_dir(unsigned char* dir_name){
	int flag;

	/*if the argument is the current directory, do nothing*/
	if(strcmp(dir_name, ".") == 0){

	}
	else{
		flag = check_if_dir(dir_name);		

		if(flag == -1){
			printf("ERROR: entry not a directory.");
		}
		else if (flag == 0){
			printf("ERROR: entry not found.");
		}
	}
	return flag;

}

/*----------------------------------------- END OF PART 4: CD DIRNAME[5] ------------------------------------------ */

void print_dir_entries(){

	/*the first sector of a cluster number is by finding the first Data Sector and then multiplying
	 * the cluster number (minus 2) with the sectors per cluster*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	/*there's 512 bytes per cluster, so multiply the sector/cluster number by those bytes
	 * to properly place the file ptr*/
	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*32 bytes after the first sector of the cluster is where the information lies
	 * every 64 bytes after that is the next DIR/file
	 * do this up to 512 bytes, as that is the bytes per entire cluster */
	for(entryNum = 0; entryNum < 8; entryNum++){

		/*read the information into an array of entries struct*/
		fread(&FI, sizeof(struct File_Info), 1, fptr);

		/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
		if(FI.attribute == 0x00)
			break;

		print_file_info();
	}
}

void print_clus_entries(uint32_t nextClus){

	/*the first sector of a cluster number is by finding the first Data Sector and then multiplying
	 * the cluster number (minus 2) with the sectors per cluster*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((nextClus - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	/*there's 512 bytes per cluster, so multiply the sector/cluster number by those bytes
	 * to properly place the file ptr*/
	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*32 bytes after the first sector of the cluster is where the information lies
	 * every 64 bytes after that is the next DIR/file
	 * do this up to 512 bytes, as that is the bytes per entire cluster */
	for(entryNum = 0; entryNum < 8; entryNum++){

		/*read the information into an array of entries struct*/
		fread(&FI, sizeof(struct File_Info), 1, fptr);

		/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
		if(FI.attribute == 0x00)
			break;

		print_file_info();
	}
}




int check_if_dir(unsigned char* dir_name){
	/*is practically the same as print_dir_entries
	 * but with a check for filename comparisons*/

	/*returns 1 for success and sets currDir,
	 * -1 if entry found but not a directory,
	 *  and 0 on failure*/

	char* tempFileName;

	/*the first sector of a cluster number is by finding the first Data Sector and then multiplying
	 * the cluster number (minus 2) with the sectors per cluster*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*32 bytes after the first sector of the cluster is where the information lies
	 * every 64 bytes after that is the next DIR/file
	 * do this up to 512 bytes, as that is the bytes per entire cluster */
	for(entryNum = 0; entryNum < 8; entryNum++){

		/*read the information into an array of entries struct*/
		fread(&FI, sizeof(struct File_Info), 1, fptr);

		/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
		if(FI.attribute == 0x00)
			break;

		//take out whitespace in filename and set to temp
		tempFileName = deblank(FI.filename);

		/*if filename found and is a directory*/
		if((strcmp(tempFileName, dir_name) == 0) && FI.attribute == 0x10){
			/*clusters that point back to root directory are 0 for some reason, 
			 * so make them 2 for proper navigation.*/
			if(FI.low_cluster == 0){
				currDir = BSI.BPB_RootClus;
			}
			else{
				currDir = FI.low_cluster;
			}
			return 1; 
		}			
		/*if filename found but wasn't a directory*/
		else if((strcmp(tempFileName, dir_name) == 0) && FI.attribute != 0x10){
			return -1;
		}
	}
	return 0;
}

char* deblank(char* input)                                         
{
	int i,j;
	char *output=input;
	for (i = 0, j = 0; i<strlen(input); i++,j++)          
	{
		if (isalpha(input[i]) || isdigit(input[i]) || input[i] == '.')                           
			output[j]=input[i];                     
		else
			j--;                                     
	}
	output[j]=0;
	return output;
}

void print_file_info(){
	/*this prints out the contents of a DIR/File entry*/

	/*set n like this so it's the length of the filename*/
	size_t n = sizeof(FI.filename)/sizeof(FI.filename[0]);
	int i;

	/*each array index is a char*/
	/*printf("DIR filename is: ");*/
	for(i = 0; i < n; i++){
		printf("%c", FI.filename[i]);
	}
	printf("\n");

	/*DEBUG comment this out later*/
	printf("DIR attribute is: 0x%02X\n", FI.attribute);
	printf("DIR HI cluster is: %d\n", FI.high_cluster);
	printf("DIR LO cluster is: %d\n", FI.low_cluster);
	printf("DIR file size is: %d\n", FI.file_size);
}


unsigned long find_next_cluster(){

	uint32_t nextClusNum;

	nextClusNum = find_FAT_sector_number() * BSI.BPB_BytesPerSec + find_FAT_entry_offset(); 

	printf("nextClusNum = %d\n", nextClusNum);

	fseek(fptr, nextClusNum, SEEK_SET);
	fread(&SecBuff, 1, 4, fptr);

	for(i = 0; i < 4; i++){
		printf("%02X", SecBuff[i]);
	}

	printf("\n");

	return SecBuff[0];

	return 0;
}

unsigned long find_first_data_sector(){
	uint32_t RootDirSectors, FirstDataSector;

	/*RootDirSectors = 0 most of the time*/
	RootDirSectors = ((BSI.BPB_RootEntCnt* 32) + (BSI.BPB_BytesPerSec-1)) / BSI.BPB_BytesPerSec;

	FirstDataSector = (BSI.BPB_RsvdSecCnt + (BSI.BPB_NumFATS * BSI.BPB_FATSz32) + RootDirSectors);

	return FirstDataSector;
}

uint32_t find_FAT_sector_number(){
	/*N is the cluster number*/
	int FATOffset = currDir * 4;

	printf("In find_FAT_sector_number, currDir is = %d\n", currDir);

	uint32_t ThisFATSecNum = BSI.BPB_RsvdSecCnt + (FATOffset / BSI.BPB_BytesPerSec);

	return ThisFATSecNum;
}


int find_FAT_entry_offset(){
	/*N is the cluster number*/

	int FATOffset = currDir * 4;

	printf("In find_FAT_entry_offset, currDir is = %d\n", currDir);

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
