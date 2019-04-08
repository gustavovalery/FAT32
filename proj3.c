/* Project 3
 * Casey Kwatkosky
 * Camila Rios
 * Emilio Sillart */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* Prototypes! */
int my_exit(int status);

typedef struct Boot_Sector_Info{
	unsigned char BS_jmpBoot[3];
	unsigned char BS_OEMName[8];

	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATS;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;

	/*THIS VALUE MUST BE ZERO FOR FAT32 SYSTEMS*/
	uint16_t BPB_FATSz16;

	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	int32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;

	/*START OF SECOND TABLE FOR FAT32*/
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint16_t BPB_FSVer;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	/*UNSURE ABOUT THIS ONE, IS 12 BYTES OR 96 BITS*/
	unsigned char BPB_Reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	unsigned char BS_VolLab[11];
	unsigned char BS_FilSysType[8];

} __attribute__((packed)) boot_sector_info;


int main(int argc, char *argv[]){
	
	FILE *fptr;
	char buffer[2048] = {0};
	int n = 0;
	struct Boot_Sector_Info BSI;

	fptr = fopen(argv[1],"rb");

	if(fptr){
		printf("fptr is valid\n\n");
	}

	fread(&BSI, sizeof(struct Boot_Sector_Info), 1, fptr);

	/*FOR TESTING PURPOSES - DELETE LATER*/
	printf("Jump Boot: %s\n", BSI.BS_jmpBoot);
	printf("OEMName: %s\n", BSI.BS_OEMName);
	printf("Bytes per Section: %d\n", BSI.BPB_BytesPerSec);

	fclose(fptr);


	/*	
	char arr[10];
	if(argc < 2){
		printf("ERROR: wrong number of arguments!\n");
		my_exit(1);
	}

	printf("%s was the argument\n",argv[1]);

	if ((fptr = fopen(argv[1],"rb")) == NULL){
		printf("ERROR: could not open file\n");
		my_exit(1);
	}
	
	fgets(arr, 10, fptr);
	printf("%s\n",arr);	
	fclose(fptr);
	*/

	my_exit(0);
return 0;
}


int my_exit(int status){
/*exit the program and clean up space*/
	printf("\n*+*+*+calling my_exit with status*+*+*+ %d\n",status);
	exit(status);
}
