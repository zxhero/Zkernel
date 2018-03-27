#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

unsigned sector_size = 0x200;
uint8_t *buf;
int num_sec, total_sec = 0;

int count_sectors(Elf32_Phdr *kernel_phdr);

Elf32_Phdr *read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr) {
    // open file and check whether it is successfully opened
    *execfile = fopen(filename, "rb");
    assert(*execfile);
    // print size of the file
    fseek(*execfile, 0, SEEK_END);
    unsigned filesize = (unsigned) ftell(*execfile);
    printf("length_of_%s = %d\n", filename, filesize);
    // check whether the file is successfully  read
    fseek(*execfile, 0x0, SEEK_SET);
    buf = (uint8_t *)malloc(filesize);
    assert(fread(buf, 1, filesize, *execfile));
    // copy elf header to ehdr
    int sz = (sizeof(Elf32_Ehdr) > sector_size) ? sizeof(Elf32_Ehdr) : sector_size;
    *ehdr = (Elf32_Ehdr *)malloc(sz);
    memcpy(*ehdr, buf, sizeof(Elf32_Ehdr));
    // copy program headers
    Elf32_Phdr *ph = (Elf32_Phdr *)malloc((*ehdr)->e_phentsize);
    memcpy(ph, buf + (*ehdr)->e_phoff, (*ehdr)->e_phentsize);
    fclose(*execfile);
    return ph;
}

void write_bootblock(FILE **imagefile, FILE *boot_file, Elf32_Ehdr *boot_header,
        Elf32_Phdr *boot_phdr) {
    // get program headers of bootblock
    *boot_phdr = *read_exec_file(&boot_file, "bootblock", &boot_header);
    // copy the content of segment
    // it seems that elf file here has only one program header
    void *addr = buf + boot_phdr->p_offset;
    assert(fwrite(addr, 1, boot_phdr->p_filesz, *imagefile));
    printf("p_offset = %d, p_filesz = %d\n", boot_phdr->p_offset, boot_phdr->p_filesz);
    // set rest of the sector to zero
    memset(buf, 0, sector_size);
    assert(fwrite(buf, 1, sector_size - boot_phdr->p_filesz, *imagefile));
    free(buf);
    total_sec += 1;
}

void write_kernel(FILE **image_file, FILE *kernel_file, Elf32_Ehdr *kernel_ehdr,
        Elf32_Phdr *kernel_phdr) {
    // get program header of kernel
    *kernel_phdr = *read_exec_file(&kernel_file, "kernel", &kernel_ehdr);
    // get sectors of kernel
    num_sec = count_sectors(kernel_phdr);
    printf("kernel_sectors: %d\n", num_sec);
    // copy the content of segment
    void *addr = buf + kernel_phdr->p_offset;
    assert(fwrite(addr, 1, kernel_phdr->p_filesz, *image_file));
    printf("p_offset = %d, p_filesz = %d\n", kernel_phdr->p_offset, kernel_phdr->p_filesz);
    // set rest of the sector(s) to zero
    memset(buf, 0, sector_size);
    assert(fwrite(buf, 1, num_sec * sector_size - kernel_phdr->p_filesz, *image_file));
    free(buf);
    total_sec += num_sec;
}

void write_other(FILE **image_file, FILE *other_file, Elf32_Ehdr *other_ehdr,
        Elf32_Phdr *other_phdr, char *filename, int offset) {
    // get program header of kernel
    *other_phdr = *read_exec_file(&other_file, filename, &other_ehdr);
    // zero part of image_file to offset
    int zero_size = offset / sector_size - total_sec;
    uint8_t temp[sector_size];
    memset(temp, 0, sector_size);
    while (zero_size > 0) {
        assert(fwrite(temp, 1, sector_size, *image_file));
        zero_size--;
    }
    // copy the content of segment
    void *addr = buf + other_phdr->p_offset;
    assert(fwrite(addr, 1, other_phdr->p_filesz, *image_file));
    printf("p_offset = %d, p_filesz = %d\n", other_phdr->p_offset, other_phdr->p_filesz);
    // set rest of the sector(s) to zero
    int sec = count_sectors(other_phdr);
    assert(fwrite(temp, 1, sec * sector_size - other_phdr->p_filesz, *image_file));
    free(buf);
    total_sec = offset / sector_size + sec;
}

int count_sectors(Elf32_Phdr *phdr) {
    int count = phdr->p_filesz;
    // round up to int
    int not_full_sector = (count % sector_size == 0) ? 0 : 1;
    return count / sector_size + not_full_sector;
}

void record_total_sectors(FILE **image_file) {
    uint32_t os_size = (total_sec - 1) * sector_size;
    uint16_t temp[] = {os_size & 0xffff, (os_size >> 16) & 0xffff};
    fseek(*image_file, 0x44, SEEK_SET);
    assert(fwrite(temp + 1, 1, 2, *image_file));
    fseek(*image_file, 0x4c, SEEK_SET);
    assert(fwrite(temp, 1, 2, *image_file));
    fseek(*image_file, 0, SEEK_END);
}

void extended_opt(Elf32_Phdr *boot_phdr, int k_phnum, Elf32_Phdr *kernel_phdr) {
    printf("\nbootblock image info\n");
    printf("sectors: 1\n");
    printf("offset of segment in the file: 0x%x\n", boot_phdr->p_offset);
    printf("the image's virtural address of segment in memory: 0x%x\n", 
            boot_phdr->p_vaddr);
    printf("the file image size of segment: 0x%x\n", boot_phdr->p_filesz);
    printf("the memory image size of segment: 0x%x\n", boot_phdr->p_memsz);
    printf("the size of write to the OS image: 0x%x\n", boot_phdr->p_filesz);
    printf("padding up to 0x%x\n", sector_size);

    printf("\nkernel image info\n");
    printf("sectors: %d\n", num_sec);
    printf("offset of segment in the file: 0x%x\n", kernel_phdr->p_offset);
    printf("the image's virtural address of segment in memory: 0x%x\n", 
            kernel_phdr->p_vaddr);
    printf("the file image size of segment: 0x%x\n", kernel_phdr->p_filesz);
    printf("the memory image size of segment: 0x%x\n", kernel_phdr->p_memsz);
    printf("the size of write to the OS image: 0x%x\n", kernel_phdr->p_filesz);
    printf("padding up to 0x%x\n", sector_size);
}


int main(int argc, char *argv[]) {
    FILE *imagefile = fopen("image", "wb"), *boot, *kernel, *other;
    Elf32_Ehdr boot_ehdr, kernel_ehdr, other_ehdr;
    Elf32_Phdr boot_phdr, kernel_phdr, other_phdr;
    int i;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "bootblock"))
            write_bootblock(&imagefile, boot, &boot_ehdr, &boot_phdr);
        else if (!strcmp(argv[i], "kernel"))
        write_kernel(&imagefile, kernel, &kernel_ehdr, &kernel_phdr);
        else if (!strcmp(argv[i], "process1"))
            write_other(&imagefile, other, &other_ehdr, &other_phdr, "process1", 0x7400);
        else if (!strcmp(argv[i], "process2"))
            write_other(&imagefile, other, &other_ehdr, &other_phdr, "process2", 0x8200);
        //else if (!strcmp(argv[i], "process3"))
        //    write_other(&imagefile, other, &other_ehdr, &other_phdr, "process3", 0x30000);
    }
    record_total_sectors(&imagefile);
    fclose(imagefile);
    for (i = 0; i < argc; i++)
        if (!strcmp(argv[i], "--extended"))
            extended_opt(&boot_phdr, kernel_ehdr.e_phnum, &kernel_phdr);
}
