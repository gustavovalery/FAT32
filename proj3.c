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

typedef struct Temp_File_Info{
	unsigned char skip0[32];
	unsigned char filename[11];
	uint8_t attribute;
	uint8_t skip1[8];
	uint16_t high_cluster;
	uint8_t skip2[4];
	uint16_t low_cluster;
	uint32_t file_size;
} __attribute__((packed)) temp_file_info;

typedef struct Temp_Dir_Info{
	unsigned char skip0[32];
	unsigned char dirname[11];
	uint8_t attribute;
	uint8_t skip1[8];
	uint16_t high_cluster;
	uint8_t skip2[4];
	uint16_t low_cluster;
	uint32_t dir_size;
} __attribute__((packed)) temp_dir_info;


/*these are globally used throughout the program*/
struct File_Info FI;
struct Boot_Sector_Info BSI;
struct Temp_File_Info TFI;
struct Temp_Dir_Info TDI;
FILE* fptr;
FILE* fptr1;
/*this would be a cluster number*/
uint32_t currDir;
uint8_t secBuff[4];
unsigned int i;

/* Prototypes! */
int my_exit(int status);
int check_if_dir(unsigned char* dir_name);
int check_if_file(unsigned char* file_name);
int find_FAT_entry_offset();
int find_empty_cluster();
int creat_file(unsigned char* file_name);
int mkdir(unsigned char* dir_name);
void info();
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void print_file_info();
void print_dir_entries();
void ls_dir(unsigned char* dir_name);
void cd_dir(unsigned char* dir_name);
void size_dir(unsigned char* file_name);
void find_next_cluster(uint32_t clusterNum);
void fill_temp_file_info(unsigned char* file_name, int clusterNum);
void fill_temp_file_info1(unsigned char* file_name, int clusterNum);
void fill_temp_dir_info(unsigned char* dir_name, int clusterNum);
void write_data(int clusterNum, unsigned char* file_name);
void write_data1(int clusterNum, unsigned char* file_name);
void write_dir(int flag, unsigned char* file_name);
void overwrite_last_cluster(int newCluster);
char* deblank(char* input);
uint32_t find_FAT_sector_number();
unsigned long find_first_data_sector();

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
	fptr = fopen(argv[1],"rb+");

	if(!fptr){
		printf("ERROR: fptr is invalid!\n\n");
		my_exit(1);
	}

	/*open the second argument of the executable in read-only binary
	 * this file pointer is for FAT region*/
	fptr1 = fopen(argv[1],"rb+");

	if(!fptr1){
		printf("ERROR: fptr1 is invalid!\n\n");
		my_exit(1);
	}

	/*using fread() on ((packed)) struct does most of the work for you
	 * solves little endian issues
	 * reads boot sector part of fat32.img into struct*/
	fread(&BSI, sizeof(struct Boot_Sector_Info), 1, fptr);
	/*this should set it = 2 at the beginning*/
	currDir = BSI.BPB_RootClus;


	/*this parsing code taken from code provided in project 1*/
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
				ls_dir(instr.tokens[1]);
			}
			else{
				printf("ERROR, too many arguments.\n");
			}
		}
		else if(strcmp(instr.tokens[0], "cd") == 0){
			if(instr.numTokens == 1){
				cd_dir(".");
			}
			else if(instr.numTokens == 2){
				cd_dir(instr.tokens[1]);
			}
			else{
				printf("ERROR, too many arguments.\n");
			}
		}
		else if(strcmp(instr.tokens[0], "size") == 0){
			if(instr.numTokens == 1){
				printf("ERROR, please provide an argument.\n");
			}	
			else if(instr.numTokens == 2){
				size_dir(instr.tokens[1]);	
			}
			else{
				printf("ERROR too many arguments.\n");
			}
		}
		else if(strcmp(instr.tokens[0], "creat") == 0){
			if(instr.numTokens == 1){
				printf("ERROR, please provide an argument.\n");
			}
			else if(instr.numTokens == 2){
				creat_file(instr.tokens[1]);
			}
			else{
				printf("ERROR, too many arguments.\n");
			}
		}
		else if(strcmp(instr.tokens[0], "mkdir") == 0){
			if(instr.numTokens == 1){
				printf("ERROR: please provide an argument.\n");
			}
			else if(instr.numTokens == 2){
				mkdir(instr.tokens[1]);
			}
			else{
				printf("ERROR: too many arguments.\n");
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
	fclose(fptr1);
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

void ls_dir(unsigned char* dir_name){
	int oldCurrDir = currDir;
	int flag;

	/*if the argument is the current directory, simply print entries*/
	if(strcmp(dir_name, ".") == 0){
		/*if not listing root cluster entries*/
		if(currDir != BSI.BPB_RootClus){
			printf(".\n");
		}
		print_dir_entries();

		while(1) {

			/*find next cluster for current directory*/
			find_next_cluster(currDir);	

			/*if there is no other cluster, break out of loop*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				break;
			}

			/*set currDir to new cluster number*/
			currDir = secBuff[1] << 8 | secBuff[0];

			/*print out all entries in new cluster number*/
			print_dir_entries();
		}
	}
	else{
		/*check if name is an entry in the current cluster*/
		flag = check_if_dir(dir_name);		

		/*if name wasn't found, should check rest of entries 
		 * in remaining clusters if there are any*/
		while(flag == 0){
			/*find next cluster for current cluster*/
			find_next_cluster(currDir);	

			/*break out of loop if there isn't one*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				break;
			}

			/*change cluster number to new one*/
			currDir = secBuff[1] << 8 | secBuff[0];	

			/*now check if name exists in this new cluster*/
			flag = check_if_dir(dir_name);
		}
		/*if flag set to -1, means name was found but wasn't a directory*/
		if(flag == -1){
			printf("ERROR: entry not a directory.\n");
		}
		/*if flag never set, was never found*/
		else if (flag == 0){
			printf("ERROR: entry not found.\n");
		}
		/*if flag set to one, name was found*/
		else if(flag == 1){
			if(currDir != 2){
				printf(".\n");
			}			
			/*print entries in cluster of found file*/
			print_dir_entries();

			/*keeps printing info for each cluster
			 * as long as there are more clusters to follow*/
			while(1) {
				/*check for more clusters in this cluster*/
				find_next_cluster(currDir);	

				/*if none can break*/
				if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
					break;
				}

				/*set currDir to new cluster*/
				currDir = secBuff[1] << 8 | secBuff[0];

				/*and print everything in new cluster*/
				print_dir_entries();
			}
		}
	}
	/*when all is said and done, set currDir back to original value*/
	currDir = oldCurrDir;
}

/*----------------------------------------- END OF PART 3: LS DIRNAME[5] ------------------------------------------ */


/*----------------------------------------- PART 4: CD DIRNAME[5] ------------------------------------------ */

void cd_dir(unsigned char* dir_name){
	int flag;
	int oldCurrDir = currDir;

	/*if the argument is the current directory, do nothing*/
	if(strcmp(dir_name, ".") == 0){

	}
	else{
		/*first check if name entered is a directory*/
		flag = check_if_dir(dir_name);		

		/*while the name isn't found in current cluster, check other clusters if there are more*/
		while(flag == 0){
			find_next_cluster(currDir);	

			/*break if no other clusters*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				break;
			}

			/*change currDir to new cluster*/
			currDir = secBuff[1] << 8 | secBuff[0];

			/*now check if name is available in this new cluster*/
			flag = check_if_dir(dir_name);
		}
		/*if -1, means name was found but wasn't a directory, set back currDir to original*/
		if(flag == -1){
			currDir = oldCurrDir;
			printf("ERROR: entry not a directory.\n");
		}
		/*if still 0 after while loop, name was never found, set back currDir to original*/
		else if (flag == 0){
			currDir = oldCurrDir;
			printf("ERROR: entry not found.\n");
		}
	}
}

/*----------------------------------------- END OF PART 4: CD DIRNAME[5] ------------------------------------------ */


/*----------------------------------------- PART 5: SIZE FILENAME[5] ------------------------------------------ */

void size_dir(unsigned char* file_name){
	int oldCurrDir = currDir;
	int flag;

	/*check if filename is in current cluster*/
	flag = check_if_file(file_name);	

	/*while the filename hasn't been found yet*/
	while(flag == 0){
		find_next_cluster(currDir);	

		/*break cluster following if there are no other clusters to follow*/
		if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
			break;
		}

		/*change currDir to new cluster*/
		currDir = secBuff[1] << 8 | secBuff[0];

		/*check if file is available in new cluster*/
		flag = check_if_file(file_name);	
	
	}
	/*if flag is -1, means entry was found but wasn't a file*/
	if(flag == -1){
		printf("ERROR: entry not a file.\n");
	}
	/*is flaf is still 0, means entry was never found*/
	else if(flag == 0){
		printf("ERROR: entry not found.\n");
	}
	/*if flag = 1, means entry was found and FI is set to that entry*/
	else if(flag == 1){
		printf("file size is %d\n", FI.file_size);
	}
	currDir = oldCurrDir;
}

/*----------------------------------------- END OF PART 5: SIZE FILENAME[5] ------------------------------------------ */


/*----------------------------------------- PART 6: CREAT FILENAME[5]  ------------------------------------------ */

int creat_file(unsigned char* file_name){
	int flag;
	int fileFlag;
	int oldCurrDir = currDir;
		
	/*check if file is in current cluster*/	
	fileFlag = check_if_file(file_name);

	/*while the filename hasn't been found yet*/
	while(fileFlag == 0){
		find_next_cluster(currDir);	

		/*break cluster following if there are no other clusters to follow*/
		if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
			break;
		}

		/*change currDir to new cluster*/
		currDir = secBuff[1] << 8 | secBuff[0];

		/*check if file is available in new cluster*/
		fileFlag = check_if_file(file_name);	
	}
	/*if flag is 1, means entry was found and was a file*/
	if(fileFlag == 1){
		printf("ERROR: filename already exists.\n");
		currDir = oldCurrDir;
		return -1;
	}

	/*This checks for the first empty cluster
	 * if found, flag is set to empty cluster number*/
	flag = find_empty_cluster();

	if(flag == -1)
		return flag;
	else
		write_data(flag, file_name);
	/*write data linking to the cluster number found*/
}

/*----------------------------------------- END OF PART 6: CREAT FILENAME[5]  ------------------------------------------ */

/*----------------------------------------- PART 7: MKDIR DIRNAME[5]  ------------------------------------------ */

int mkdir(unsigned char* dir_name){
	int flag;
	int dirFlag;
	int oldCurrDir = currDir;

	/*check if dir is in current cluster*/	
	dirFlag = check_if_dir(dir_name);

	/*while the dirname hasn't been found yet*/
	while(dirFlag == 0){
		find_next_cluster(currDir);	

		/*break cluster following if there are no other clusters to follow*/
		if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
			break;
		}

		/*change currDir to new cluster*/
		currDir = secBuff[1] << 8 | secBuff[0];

		/*check if dir is available in new cluster*/
		dirFlag = check_if_dir(dir_name);	
	}
	/*if flag is 1, means entry was found and was a dir*/
	if(dirFlag == 1){
		printf("ERROR: directory name already exists.\n");
		currDir = oldCurrDir;
		return -1;
	}

	/*check for the first empty cluster
	 * if found, flag is set to empty cluster number*/
	flag = find_empty_cluster();

	if(flag == -1)
		return flag;
	else
		write_dir(flag, dir_name);
	/*DEBUG new function here*/

}


/*----------------------------------------- END OF PART 7: MKDIR DIRNAME[5]  ------------------------------------------ */


void print_dir_entries(){

	/*the first data sector of a cluster number is found by finding the first Data Sector and then multiplying
	 * the cluster number (minus 2) with the sectors per cluster*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	/*there's 512 bytes per cluster, so multiply the sector/cluster number by those bytes
	 * to properly place the file ptr*/
	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*cluster is 512 bytes with each entry being 62 bytes of info*/
	for(entryNum = 0; entryNum < 8; entryNum++){

		/*read the information into an array of entries struct*/
		fread(&FI, sizeof(struct File_Info), 1, fptr);

		/*if it starts reading in 0s, means no more valid entries*/
		if(FI.attribute == 0x00)
			break;

		/*prints info in global FI struct*/
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

	/*jump to first data sector of the currDir/cluster number*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*512 bytes for data sector of a cluster and each entry is 64 bytes*/
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
			if((FI.high_cluster << 16 | FI.low_cluster) == 0){
				currDir = BSI.BPB_RootClus;
			}
			else{
				currDir = (FI.high_cluster << 16 | FI.low_cluster);
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

int check_if_file(unsigned char* file_name){
	/*is practically the same as print_dir_entries
	 * but with a check for filename comparisons*/

	/*returns 1 for success and sets currDir,
	 * -1 if entry found but not a directory,
	 *  and 0 on failure*/

	char* tempFileName;

	/*jump to first data sector of the currDir/cluster number*/
	unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

	/*offset of bytes*/
	int entryNum;

	fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

	/*512 bytes for data sector of a cluster and each entry is 64 bytes*/
	for(entryNum = 0; entryNum < 8; entryNum++){

		/*read the information into an array of entries struct*/
		fread(&FI, sizeof(struct File_Info), 1, fptr);

		/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
		if(FI.attribute == 0x00)
			break;

		//take out whitespace in filename and set to temp
		tempFileName = deblank(FI.filename);

		/*if filename found and is a file*/
		if((strcmp(tempFileName, file_name) == 0) && FI.attribute == 0x20){
			/*clusters that point back to root directory are 0 for some reason, 
			 * so make them 2 for proper navigation.*/
			if((FI.high_cluster << 16 | FI.low_cluster) == 0){
				currDir = BSI.BPB_RootClus;
			}
			else{
				currDir = (FI.high_cluster << 16 | FI.low_cluster);
			}
			return 1; 
		}			
		/*if filename found but wasn't a file*/
		else if((strcmp(tempFileName, file_name) == 0) && FI.attribute != 0x20){
			return -1;
		}
	}
	/*return 0 at the end if it was never found*/
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

	/*
	  printf("DIR attribute is: 0x%02X\n", FI.attribute);
	  printf("DIR HI cluster is: %d\n", FI.high_cluster);
	  printf("DIR LO cluster is: %d\n", FI.low_cluster);
	  printf("DIR file size is: %d\n", FI.file_size);
	*/
}

/*this finds the next cluster for the current directory
 * and stores it in the global secBuff*/
void find_next_cluster(uint32_t clusterNum){
	int i;

	uint32_t start_FAT = BSI.BPB_RsvdSecCnt * BSI.BPB_BytesPerSec;

	/*this moves fptr1 to start of FAT region*/
	fseek(fptr1, start_FAT, SEEK_SET);

	for(i = 0; i <= clusterNum; i++){
		fread(&secBuff, 1, 4, fptr1);
	}


}

int find_empty_cluster(){
	/*this function returns i on success, which is the empty cluster number*/
	int i;

	//used for fwrite, means end of Cluster
	uint8_t emptyClusBuff[4] = {0xFF, 0xFF, 0xFF, 0x0F};

	uint32_t start_FAT = BSI.BPB_RsvdSecCnt * BSI.BPB_BytesPerSec;

	/*move fptr1 to start of FAT region*/
	fseek(fptr1, start_FAT, SEEK_SET);

	/*There's 1009 total clusters in each FAT region*/
	for(i = 0; i <= 1009; i++){
		fread(&secBuff, 1, 4, fptr1);

		/*if an empty cluster is found*/
		if(secBuff[0] == 0x00 && secBuff[1] == 0x00 && secBuff[2] == 0x00 && secBuff[3] == 0x00){
			/*move fptr1 to right before the empty cluster for writing*/
			fseek(fptr1, start_FAT + (i * 4), SEEK_SET);
			/*write the end of cluster marker over the empty cluster*/
			fwrite(emptyClusBuff,1, 4, fptr1);

			return i;
		}
	}
	printf("ERROR no empty cluster found, FAT table full.\n");
	return -1;
}

void write_data(int clusterNum, unsigned char* file_name){

	int oldCurrDir = currDir;
	int moreClustersFlag = 0;
	int newCluster;

	while(moreClustersFlag == 0){
		/*flag that sets to 1 when a free entry is found in current cluster to write new data*/
		int freeEntry = 0;

		/*check for an empty slot in current cluster/directory*/

		/*jump to first data sector of the currDir/cluster number*/
		unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

		/*offset of bytes*/
		int entryNum;

		fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

		/*512 bytes for data sector of a cluster and each entry is 64 bytes*/
		for(entryNum = 0; entryNum < 8; entryNum++){

			/*read the information into an array of entries struct*/
			fread(&FI, sizeof(struct File_Info), 1, fptr);

			/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
			if(FI.attribute == 0x00){
				freeEntry = 1;
				break;
			}
		}

		/*if 1, can write new data into this cluster
		 * each entry is 64 bytes*/
		if(freeEntry == 1){
			fseek(fptr, ((FirstSectorofCluster * BSI.BPB_BytesPerSec) + (entryNum * 64)), SEEK_SET);

			/*put data into temp struct*/
			fill_temp_file_info(file_name, clusterNum);

			/*write struct info into empty data section of current cluster*/
			fwrite(&TFI, 1, sizeof(struct Temp_File_Info), fptr);
			moreClustersFlag = 1;
		}
		/*else will have to make new cluster for new data in current cluster*/
		else{
			find_next_cluster(currDir);

			/*means there is no other cluster*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				/*get the cluster number of the next empty cluster 
				 * after replacing it with EoC mark*/
				newCluster = find_empty_cluster();

				/*overwrite last cluster with EoC mark to point to
				 * the newly created cluster
				 * also makes secBuff the new cluster*/
				overwrite_last_cluster(newCluster);
			}

			/*set currDir to new cluster number*/
			currDir = secBuff[1] << 8 | secBuff[0];
		}
	}
	currDir = oldCurrDir;
}

void write_data1(int clusterNum, unsigned char* file_name){

	int oldCurrDir = currDir;
	int moreClustersFlag = 0;
	int newCluster;

	while(moreClustersFlag == 0){
		/*flag that sets to 1 when a free entry is found in current cluster to write new data*/
		int freeEntry = 0;

		/*check for an empty slot in current cluster/directory*/

		/*jump to first data sector of the currDir/cluster number*/
		unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

		/*offset of bytes*/
		int entryNum;

		fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

		/*512 bytes for data sector of a cluster and each entry is 64 bytes*/
		for(entryNum = 0; entryNum < 8; entryNum++){

			/*read the information into an array of entries struct*/
			fread(&FI, sizeof(struct File_Info), 1, fptr);

			/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
			if(FI.attribute == 0x00){
				freeEntry = 1;
				break;
			}
		}

		/*if 1, can write new data into this cluster
		 * each entry is 64 bytes*/
		if(freeEntry == 1){
			fseek(fptr, ((FirstSectorofCluster * BSI.BPB_BytesPerSec) + (entryNum * 64)), SEEK_SET);

			/*put data into temp struct, use different 
			 * temp file that fills as a dir since
			 * original fill_dir doesn't work for some 
			 * unknown reason*/
			fill_temp_file_info1(file_name, clusterNum);

			/*write struct info into empty data section of current cluster*/
			fwrite(&TFI, 1, sizeof(struct Temp_File_Info), fptr);
			moreClustersFlag = 1;
		}
		/*else will have to make new cluster for new data in current cluster*/
		else{
			find_next_cluster(currDir);

			/*means there is no other cluster*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				/*get the cluster number of the next empty cluster 
				 * after replacing it with EoC mark*/
				newCluster = find_empty_cluster();

				/*overwrite last cluster with EoC mark to point to
				 * the newly created cluster
				 * also makes secBuff the new cluster*/
				overwrite_last_cluster(newCluster);
			}

			/*set currDir to new cluster number*/
			currDir = secBuff[1] << 8 | secBuff[0];
		}
	}
	currDir = oldCurrDir;
}







/*this writes the new directory into the directory*/
void write_dir(int clusterNum, unsigned char* dir_name){

	int oldCurrDir = currDir;
	int moreClustersFlag = 0;
	int newCluster;

	while(moreClustersFlag == 0){
		/*flag that sets to 1 when a free entry is found in current cluster to write new data*/
		int freeEntry = 0;

		/*check for an empty slot in current cluster/directory*/

		/*jump to first data sector of the currDir/cluster number*/
		unsigned long FirstSectorofCluster = find_first_data_sector() + ((currDir - 2) * BSI.BPB_SecPerClus);

		/*offset of bytes*/
		int entryNum;

		fseek(fptr, FirstSectorofCluster * BSI.BPB_BytesPerSec, SEEK_SET);

		/*512 bytes for data sector of a cluster and each entry is 64 bytes*/
		for(entryNum = 0; entryNum < 8; entryNum++){

			/*read the information into an array of entries struct*/
			fread(&FI, sizeof(struct File_Info), 1, fptr);

			/*if it starts reading in 0s, means no more valid entries, so can break out of loop*/
			if(FI.attribute == 0x00){
				freeEntry = 1;
				break;
			}
		}

		/*if 1, can write new data into this cluster
		 * each entry is 64 bytes*/
		if(freeEntry == 1){
			fseek(fptr, ((FirstSectorofCluster * BSI.BPB_BytesPerSec) + (entryNum * 64)), SEEK_SET);

			/*put dir data into temp struct*/
			fill_temp_dir_info(dir_name, clusterNum);

			/*write struct info into empty data section of current cluster*/
			fwrite(&TDI, 1, sizeof(struct Temp_Dir_Info), fptr);

			/*DEBUG after this should write .. entry into new dir
			 * basically have to write new file called ".."
			 * and set low cluster to oldCurrDir*/

			oldCurrDir = currDir;

			cd_dir(dir_name);
	
			/*a different write data that writes as a dir, won't work
			 * with normal write_dir for some reason*/
			write_data1(oldCurrDir, "..");

			/*
			fill_temp_file_info("..", oldCurrDir);
			fwrite(&TFI, 1, sizeof(struct Temp_File_Info), fptr);	
			*/

			moreClustersFlag = 1;
		}
		/*else will have to make new cluster for new data in current cluster*/
		else{
			find_next_cluster(currDir);

			/*means there is no other cluster*/
			if(secBuff[0] == 0xFF || secBuff[0] == 0xF8 || secBuff[0] == 0x00){
				/*get the cluster number of the next empty cluster 
				 * after replacing it with EoC mark*/
				newCluster = find_empty_cluster();

				/*overwrite last cluster with EoC mark to point to
				 * the newly created cluster
				 * also makes secBuff the new cluster*/
				overwrite_last_cluster(newCluster);
			}

			/*set currDir to new cluster number*/
			currDir = secBuff[1] << 8 | secBuff[0];
		}
	}
	currDir = oldCurrDir;
}


/*this finds the last cluster of the current directory 
 * and overwrites it to point to the new cluster*/
void overwrite_last_cluster(int newCluster){

	 uint8_t newClusBuff[4] = {newCluster, newCluster >> 8, newCluster >> 16, newCluster >> 24};

	/*beginning of FAT region*/
	uint32_t start_FAT = BSI.BPB_RsvdSecCnt * BSI.BPB_BytesPerSec;

	/*this moves fptr1 to beginning of EoC cluster*/
	fseek(fptr1, start_FAT + (4 * currDir), SEEK_SET);

	/*write new cluster marker over EoC cluster*/
	fwrite(newClusBuff, 1, 4, fptr1);

	/*move back to beginning of cluster*/
	fseek(fptr1, start_FAT + (4 * currDir), SEEK_SET);

	/*read in new cluster number to secBuff*/
	fread(&secBuff, 1, 4, fptr1);
}


void fill_temp_file_info(unsigned char* file_name, int clusterNum){
	int i;

	/*fill temp_file_info with data*/	
	for(i = 0; i < 32; i++){
		TFI.skip0[i] = 0x00;
	}

	strcpy(TFI.filename, file_name);
	TFI.attribute = 0x20;

	for(i = 0; i < 8; i++){
		TFI.skip1[i] = 0x00;
	}

	TFI.high_cluster = 0;
	TFI.low_cluster = clusterNum; 
	TFI.file_size = 0;
}

void fill_temp_file_info1(unsigned char* file_name, int clusterNum){
	int i;

	/*fill temp_file_info with data*/	
	for(i = 0; i < 32; i++){
		TFI.skip0[i] = 0x00;
	}

	strcpy(TFI.filename, file_name);
	TFI.attribute = 0x10;

	for(i = 0; i < 8; i++){
		TFI.skip1[i] = 0x00;
	}

	TFI.high_cluster = 0;
	TFI.low_cluster = clusterNum; 
	TFI.file_size = 0;
}

void fill_temp_dir_info(unsigned char* dir_name, int clusterNum){
	int i ;

	/*fill temp_dir_info with data*/
	for(i = 0; i < 32; i++){
		TDI.skip0[i] = 0x00;	
	}

	strcpy(TDI.dirname, dir_name);
	TDI.attribute = 0x10;

	for(i = 0; i < 8; i++){
		TDI.skip1[i] = 0x00;
	}

	TDI.high_cluster = 0;
	TDI.low_cluster = clusterNum;
	TDI.dir_size = 0;
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

	uint32_t ThisFATSecNum = BSI.BPB_RsvdSecCnt + (FATOffset / BSI.BPB_BytesPerSec);

	return ThisFATSecNum;
}


int find_FAT_entry_offset(){
	/*N is the cluster number*/

	int FATOffset = currDir * 4;

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
