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
uint32_t total_sector = 0;

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
	int remain;
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
			fseek(fp,addr,SEEK_SET);
			while(filesz >= 512){
				for(j = 0;j < 512; j++){
					buf[j] = 0;
				}
				//fseek(fp,addr,SEEK_SET);		
				fread(buf,512,1,fp);
				filesz -= 512;
				//addr += 512;
				fwrite(buf, 512,1,fp2);
				sector_mem += 1;
			}
			for(j = 0;j < 512; j++){
				buf[j] = 0;
			}

			//fseek(fp,addr,SEEK_SET);
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
			/*buf[511] = 0x55;
			buf[510] = 0x55;
			buf[509] = 0xaa;
			buf[508] = 0xaa;*/
			fwrite(buf, 512,1,fp2);
			sector_mem += 1;
			for(j = 0;j < 512; j++){
				buf[j] = 0;
			}
			if(ph[i].p_vaddr != 0xa0830000){
				if(ph[i].p_vaddr == 0xa0800000){
					remain = 0;
					total_sector += 1;
				}
				else if(ph[i].p_vaddr == 0xa0800200){
					remain = 127 - sector_mem;
					total_sector += 127;
				}
				else{
					remain = 128 - sector_mem;
					total_sector += 128;	
				}
				for(j = 0;j < remain ;j++){
					fwrite(buf, 512,1,fp2);
				}
			}
			else total_sector += sector_mem;
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
	int i;
	for(i = 2;i < argc;i++){
		loader(argv[i],fp2);
	}
	changesize(fp2,total_sector);
	fclose(fp2);
	printf("Writing %s succeed\n", argv[0]);
	return 0; 
} 
