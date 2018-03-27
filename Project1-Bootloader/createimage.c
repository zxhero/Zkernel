#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>  
#include <stdarg.h>
#include <elf.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

uint32_t sector_mem;

void loader(char *file,FILE *fp2) {
	FILE *fp = fopen(file, "rb");
	//assert(fp);
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;
	long addr = 0;
	uint8_t buf[512];
	uint8_t buf2[512];
	int i;
	int j;
	sector_mem = 0;
	uint32_t filesz;
	uint32_t memsize;
	struct stat statbuf;	
	fread(buf2,512,1,fp);
	elf = (void *)buf2;
	stat(file,&statbuf);
	printf("%s imformation:\n",file);
	printf("Total length of %s is %d\n",file,(int)statbuf.st_size);

	for(i = 0, ph = (void *)buf2 + elf->e_phoff; i < elf->e_phnum; i ++) {
		// scan the program header table, load each segment into memory
		if(ph[i].p_type == PT_LOAD) {
			filesz = ph[i].p_filesz;
			memsize = ph[i].p_memsz - filesz;
			addr = ph[i].p_offset;
			while(filesz >= 512){
				for(j = 0;j < 512; j++){
					buf[j] = 0;
				}
				fseek(fp,addr,SEEK_SET);		
				fread(buf,512,1,fp);
				filesz -= 512;
				addr += 512;
				fwrite(buf, 512,1,fp2);
				sector_mem += 1;
			}
			for(j = 0;j < 512; j++){
				buf[j] = 0;
			}
			fseek(fp,addr,SEEK_SET);
			fread(buf,filesz,1,fp);
			while(memsize >= (512-filesz)){
				fwrite(buf, 512,1,fp2);
				memsize -= (512-filesz);
				for(j = 0;j < 512; j++){
					buf[j] = 0;
				}
				sector_mem += 1;
				filesz = 0;
			}
			buf[511] = 0x55;
			buf[510] = 0x55;
			buf[509] = 0xaa;
			buf[508] = 0xaa;
			fwrite(buf, 512,1,fp2);
			sector_mem += 1;
			printf("Sectors: %u\n",sector_mem);
			printf("File will be loaded to %x\n",ph[i].p_vaddr);
			printf("The file size of %s is %u\n",file,ph[i].p_filesz);
			printf("The memory size of %s is %u\n",file,ph[i].p_memsz); 
		}	
	}
	fclose(fp);
}

void changesize(FILE *fp2, uint32_t sector_num){
	uint32_t size = sector_num << 9;
	fseek(fp2,160,SEEK_SET);
	fwrite(&size,4,1,fp2);
}

int main(int argc, char *argv[]) {  
	/* load MIPS binary executable file to distributed memory */
	char *file2 = "image"; 
	FILE *fp2 = fopen(file2,"wb+");
	if(argc == 4){
		loader(argv[2],fp2);
		loader(argv[3],fp2);
		changesize(fp2,sector_mem);
	}
	fclose(fp2);
	printf("Writing %s succeed\n", argv[0]);
	return 0; 
} 
