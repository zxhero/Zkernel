#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<stdint.h>
#include<sys/stat.h>

int main(int argc,int *argv[])
{
	FILE *fp[10];
	int i,size,j;
	FILE *imageaddress=fopen("image","w");
	FILE *writepointer=imageaddress;
	struct stat file_info;
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph;
	int address[100]={0,0,0,512,131072,139264,147456,155648};
	int zero=0;
	int write_size;
	int total_size;
	for(i=2;i<argc;i++){
		write_size=0;
		fseek(writepointer,address[i],0);
		fp[i]=fopen(argv[i],"r");
		stat(argv[i],&file_info);
		size=file_info.st_size;
	        uint8_t buf[size];
		fread(buf,size,1,fp[i]);
		elf=(void *)buf;
		for(j=0,ph=(void *)buf+elf->e_phoff;j<elf->e_phnum;j++){
			if(ph[j].p_type==PT_LOAD){
			    fwrite(buf+ph[j].p_offset,ph[j].p_filesz,1,writepointer);
			    write_size=write_size+ph[j].p_filesz;
		            printf("offset of segment in the file: 0x%x\n",ph[j].p_offset);
	                    printf("the image's virtual address of segment in memory:0x%x\n",ph[j].p_vaddr);
	                    printf("the file image size of segment: 0x%x\n",ph[j].p_filesz);
	                    printf("the memory image size of segment: 0x%x\n",ph[j].p_filesz);
	                    printf("the size of write to the OS image: 0x%x\n\n",ph[j].p_filesz);
			}
			
		}
//		if(i>2&&i<argc-1){
//			if(write_size<(address[i+1]-address[i]))
//				fwrite(&zero,address[i+1]-address[i],1,writepointer);
//		}
		if(i==(argc-1)){
//			for(;write_size%512!=0;write_size=write_size+4)
//				fwrite(&zero,4,1,writepointer);
			total_size=address[i]+write_size;
		}		
	}
	fseek(writepointer,508,0);
	fwrite(&total_size,4,1,writepointer);
	for(i=2;i<argc;i++)
	        fclose(fp[i]);
        fclose(imageaddress);
	return 0;	
}
