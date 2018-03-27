#include "common.h"

/*define global variables here*/

/*
 Use linear table or other data structures as you need.
 
 example:
 struct file_info[MAX_OPEN_FILE] fd_table;
 struct inode[MAX_INODE] inode_table;
 unsigned long long block_bit_map[];
 Your dentry index structure, and so on.
 
 
 //keep your root dentry and/or root data block
 //do path parse from your filesystem ROOT<@mount point>
 
 struct dentry* root;
 */
#define	MAX_INODE	100 //一个dir中包含file的最大个数
#define MAX_OPEN_FILE	10
#define MAGIC		122481721

struct file_info  fd_table[MAX_OPEN_FILE];
unsigned int file_opennum;
struct inode root_inode;
char bit_map[DATA_MAP_SIZE+INODE_MAP_SIZE];
struct superblock sblock;
struct dlocation {
	unsigned int phoffset;
	unsigned int index;
	unsigned int pre_index;
};

unsigned int fpath2inode(char **path, struct inode *itemp);
unsigned int path_parse(char *path);
unsigned int backup_haxi(unsigned int haxi);
void init_inode(struct inode *inode_temp,struct inode *father,mode_t mode,char *name,int name_len);
void free_inode(unsigned int iindex);
void init_dentry(struct inode *inode_temp,struct dentry *dentry_tmp,unsigned int pre_index);
void write_bmap(unsigned int offset);
void write_imap(unsigned int index, unsigned int value);
void write_super();
void write_inode(struct inode *inode_temp);
void write_dentry(struct dentry *dentry_tmp, unsigned int index, unsigned int phaddr);
void read_dentry(struct dentry *dentry_tmp, unsigned int index, unsigned int phaddr);
void read_inode(struct inode *inode_temp, unsigned int inode_index);
unsigned int aloc_block();
int	find_hash(struct dlocation *dloc, unsigned int src, unsigned int haxi, char* name, int name_len);
int	rmdir(char *name, struct inode *root) ;

unsigned int fpath2inode(char **path, struct inode *itemp){
	*path = *path + 1;
	char *next_name = strchr(*path, '/');
	char buf[4096], buf2[4096];
	int i, j, flag;
	unsigned int name_len, haxi, iindex=0, fsize;
	unsigned int *ptr;
	struct inode  *temp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	temp = &root_inode;
	read_inode(itemp,0);
	while (next_name != NULL) {
		name_len = next_name - *path;
		haxi = path_parse(*path);
		iindex = find_hash(&dloc, temp->first_point, haxi, *path, name_len);
		read_inode(itemp, iindex);
		temp = itemp;
		*path = next_name + 1;
		next_name = strchr(*path, '/');
	}
	return iindex;
}

unsigned int path_parse(char *path){
	char x = *path;
	unsigned int haxi = 0;
	if(x == '_') haxi+=9300;
	else if(x <= '9' && x >= '0'){
		haxi+=((x-'0')*150);
	}
	else if(x <= 'Z' && x >= 'A'){
		haxi+=((x-'A')*150+1500);
	}
	else if(x <= 'z' && x >= 'a'){
		haxi+=((x-'a')*150+5400);
	}
	x = *(path+1);
	if(x == '\0' || x == '/') return haxi;
	else if(x == '_') haxi+=124;
	else if(x <= '9' && x >= '0'){
		haxi+=((x-'0')*2);
	}
	else if(x <= 'Z' && x >= 'A'){
		haxi+=((x-'A')*2+20);
	}
	else if(x <= 'z' && x >= 'a'){
		haxi+=((x-'a')*2+72);
	}
	x = *(path+1);
	if(x == '\0'|| x == '/') return haxi;
	else if(x <= 'z' && x >= 'a' || x <= 'Z' && x >= 'V' || x == '_')
		haxi+=1;
	return haxi;
}

unsigned int backup_haxi(unsigned int haxi){
	unsigned int group_index = haxi / 150;
	unsigned int block_index = (haxi % 150)/2;
	unsigned int new_haxi = 9450 + group_index * 90 + (block_index/2);
	return new_haxi;
}

void init_inode(struct inode *inode_temp,struct inode *father,mode_t mode,char *name,int name_len){
	int		new_inode_index,i, write_sector;
	memset(inode_temp,0,sizeof(struct inode));
	new_inode_index = sblock.Itable_ptr - (struct inode_t *)sblock.Itable_offset;
	sblock.Itable_ptr++;
	write_imap(new_inode_index,1);
	for(i = 0; i< name_len;i++){
		inode_temp->fname[i] = name[i];
	}
	inode_temp->link_count = 0;
	inode_temp->mode = mode;
	inode_temp->current_inode = new_inode_index;
	inode_temp->file_size = 4096;
	inode_temp->creat_time = new_inode_index;
	inode_temp->f_inode = father->current_inode;
	inode_temp->first_point = aloc_block();
	inode_temp->slink_node = 0;
	sblock.f_ffree -= 1;
}

void free_inode(unsigned int iindex) {
	struct inode inode_temp;
	write_imap(iindex, 0);
	read_inode(&inode_temp, iindex);
	sblock.f_files -= inode_temp.file_size / SECTOR_SIZE;
	//sblock.f_ffree += 1;

}

void init_dentry(struct inode *inode_temp,struct dentry *dentry_tmp,unsigned int pre_index){
	int i=0;
	memset(dentry_tmp,0,sizeof(struct dentry));
	dentry_tmp->finode = inode_temp->current_inode;
	dentry_tmp->ftype = inode_temp->file_type;
	dentry_tmp->next_index = -1;
	while (inode_temp->fname[i] != '\0') {
		dentry_tmp->fname[i] = inode_temp->fname[i];
		i++;
	}
	dentry_tmp->fname[i] = '\0';
	dentry_tmp->pre_index = pre_index;
}

void write_bmap(unsigned int offset){
	char *map_ptr;
	unsigned int write_sector;
	char	buf[4096];
	memset(buf,0,4096);
	map_ptr = bit_map + (offset-sblock.datablock_offset)/SECTOR_SIZE/8;
	(*map_ptr) |= (0x1<<((offset -sblock.datablock_offset)/SECTOR_SIZE)%8);
	write_sector = (map_ptr - bit_map) /SECTOR_SIZE+2;
	memcpy(buf,bit_map +((map_ptr-bit_map)/SECTOR_SIZE)*SECTOR_SIZE,SECTOR_SIZE);
	device_write_sector(buf, write_sector);
}

void write_imap(unsigned int index, unsigned int value){
	char *map_ptr;
	unsigned int write_sector;
	char buf[4096];
	memset(buf,0,4096);
	map_ptr = bit_map + sblock.imap_offset + index / 8;
	if(value == 1)
		(*map_ptr) |= (0x1 << (index % 8));
	else
		(*map_ptr) &= (~(0x1 << (index % 8)));
	write_sector = (map_ptr - bit_map) / SECTOR_SIZE + 2;
	memcpy(buf, bit_map + ((map_ptr - bit_map) / SECTOR_SIZE)*SECTOR_SIZE, SECTOR_SIZE);
	device_write_sector(buf, write_sector);
}

void write_super(){
	char buf[4096];
	memset(buf,0,4096);
	struct superblock_t *sptr;
	sptr = (struct superblock_t *)buf;
	sptr->magic = MAGIC;
	sptr->f_blocks = DATA_BLOCK_NUM;
	sptr->f_files = 8 * INODE_BLOCK_NUM;
	sptr->f_ffree = sblock.f_files;
	sptr->f_bfree = sblock.f_bfree;
	sptr->Sblock2_offset = SECTOR_SIZE;
	sptr->Sblock_len = 64;
	sptr->Bmap_offset = 2 * SECTOR_SIZE;
	sptr->Bmap_len = DATA_MAP_SIZE;
	sptr->imap_offset = sptr->Bmap_offset + DATA_MAP_SIZE;
	sptr->imap_len = INODE_MAP_SIZE;
	sptr->Itable_offset = 36 * SECTOR_SIZE;
	sptr->Itable_len = sblock.Itable_ptr - (struct inode_t*)(sptr->Itable_offset);
	sptr->datablock_offset = sblock.datablock_offset;
	sptr->datablock_len = (sblock.data_ptr - sblock.datablock_offset)/SECTOR_SIZE;
	device_write_sector(buf, 0);
	device_write_sector(buf, 1);
}

void write_inode(struct inode *inode_temp){
	char buf[4096];
	memset(buf,0,4096);
	unsigned int write_sector;
	struct inode_t *iptr;
	int i = 0;
	write_sector = (inode_temp->current_inode/8)+sblock.Itable_offset/SECTOR_SIZE;
	device_read_sector(buf, write_sector);
	iptr = (struct inode_t*)buf + inode_temp->current_inode % 8;
	iptr->file_size = inode_temp->file_size;
	while (inode_temp->fname[i] != '\0') {
		iptr->fname[i] = inode_temp->fname[i];
		i++;
	}
	iptr->fname[i] = '\0';
	iptr->file_type = inode_temp->file_type;
	iptr->mode = inode_temp->mode;
	iptr->link_count = inode_temp->link_count;
	iptr->creat_time = inode_temp->creat_time;
	iptr->first_point = inode_temp->first_point;
	iptr->f_inode = inode_temp->f_inode;
	iptr->current_inode = inode_temp->current_inode;
	iptr->slink_node = inode_temp->slink_node;
	device_write_sector(buf, write_sector);
}

void write_dentry(struct dentry *dentry_tmp, unsigned int index, unsigned int phaddr){
	char buf[4096];
	memset(buf,0,4096);
	unsigned int write_sector;
	struct dentry *dptr;
	write_sector = phaddr / SECTOR_SIZE;
	device_read_sector(buf, write_sector);
	dptr = (struct dentry*)buf + index;
	if (dptr->fname[0] == 0) {
		memcpy(dptr, dentry_tmp, DENTRY_SIZE);
	}
	device_write_sector(buf, write_sector);
}

void read_dentry(struct dentry *dentry_tmp, unsigned int index, unsigned int phaddr) {
	char buf[4096];
	memset(buf,0,4096);
	memset(dentry_tmp,0,sizeof(struct dentry));
	unsigned int read_sector;
	read_sector = phaddr / SECTOR_SIZE;
	device_read_sector(buf, read_sector);
	memcpy(dentry_tmp, (struct dentry *)buf + index, DENTRY_SIZE);
}

void read_inode(struct inode *inode_temp, unsigned int inode_index){
	unsigned int read_block;
	char buff[4096];
	memset(buff,0,4096);
	memset(inode_temp,0,sizeof(struct inode));
	struct inode_t *iptr;
	int i = 0;
	read_block = sblock.Itable_offset / SECTOR_SIZE + inode_index/8;
	device_read_sector(buff, read_block);
	iptr = (struct inode_t *)buff + inode_index % 8;
	inode_temp->file_type = iptr->file_type;
	inode_temp->file_size = iptr->file_size;
	inode_temp->link_count = iptr->link_count;
	inode_temp->creat_time = iptr->creat_time;
	inode_temp->first_point = iptr->first_point;
	inode_temp->current_inode = iptr->current_inode;
	inode_temp->f_inode = iptr->f_inode;
	inode_temp->mode = iptr->mode;
	inode_temp->slink_node = iptr->slink_node;
	while (iptr->fname[i] != '\0') {
		inode_temp->fname[i] = iptr->fname[i];
		i++;
	}
	inode_temp->fname[i] = '\0';
}

unsigned int aloc_block(){
	char buf[4096];
	//memset(buf,0,4096);
	unsigned int alloc_sector, phaddr;
	phaddr = sblock.data_ptr;
	alloc_sector = phaddr / SECTOR_SIZE;
	memset(buf, 0, 4096);
	device_write_sector(buf, alloc_sector);
	write_bmap(phaddr - sblock.datablock_offset);
	sblock.data_ptr += SECTOR_SIZE;
	sblock.f_bfree -= 1;
	return phaddr;
}

int	find_hash(struct dlocation *dloc, unsigned int src, unsigned int haxi, char* name, int name_len) {							//返回-1表示未找到，建立好块，同时返回dentry块的地址和块内偏移，否则返回inode的index,以及dentry的位置
	char buf1[4096], buf2[4096], buf3[4096];
	unsigned int read_sector1, read_sector2, read_sector3,nindex=0, next_index,pre_index;
	unsigned int *ptr;
	struct dentry *dptr, *ndptr;
	int	new_haxi;
	read_sector1 = src / SECTOR_SIZE;
	device_read_sector(buf1, read_sector1);
	ptr = (unsigned int*)buf1;
	ptr += (haxi / 15);
	if (*ptr == NULL) {
		//分配空块, 更新目录块内容
		*ptr = sblock.data_ptr;
		device_write_sector(buf1, read_sector1);
		dloc->phoffset = aloc_block();
		dloc->index = haxi % 15;
		dloc->pre_index = -1;
		return -1;	
	}
	else {
		read_sector2 = (*ptr) / SECTOR_SIZE;
		device_read_sector(buf2, read_sector2);
		dptr = (struct inode*)buf2;
		dptr += (haxi % 15);
		dloc->pre_index = haxi;
		if (strncmp(name, dptr->fname, name_len) != 0 && dptr->fname[0] != 0) {		//在溢出区或备用区
			while (dptr->next_index != -1) {	
				next_index = dptr->next_index;
				dloc->pre_index = next_index;
				if (next_index / 15 == (ptr - (unsigned int*)buf1)) {
					dptr = (struct inode*)buf2 + next_index % 15;
				}
				else {
					ptr = (unsigned int*)buf1 + next_index / 15;
					read_sector2 = (*ptr) / SECTOR_SIZE;
					device_read_sector(buf2, read_sector2);
					dptr = (struct inode*)buf2 ;
					dptr += (next_index % 15);
				}
				if (strncmp(name, dptr->fname, name_len) == 0) {
					dloc->phoffset = (*ptr);
					dloc->index = next_index % 15;
					return dptr->finode;
				}
			}
			//未找到，找到一个合适的位置进行分配
			ptr = (unsigned int*)buf1 + haxi/150 * 10 + 9;		//从溢出区找
			if (*ptr == NULL) {
				*ptr = sblock.data_ptr;
				device_write_sector(buf1, read_sector1);
				dptr->next_index = haxi / 150 * 150 + 135;
				device_write_sector(buf2, read_sector2);
				dloc->phoffset = aloc_block();
				dloc->index = 0;
				return -1;
			}
			else {
				read_sector3 = (*ptr) / SECTOR_SIZE;
				device_read_sector(buf3, read_sector3);
				ndptr = (struct inode*)buf3;
				while (nindex < 15 && ndptr->fname[0] != 0) {
					ndptr++;
					nindex++;
				}
				if (nindex == 15) {								//从后备区找
					new_haxi = backup_haxi(haxi);
					ptr = (unsigned int*)buf1 + new_haxi /15;
					if (*ptr == NULL) {
						*ptr = sblock.data_ptr;
						device_write_sector(buf1, read_sector1);
						dptr->next_index = new_haxi;
						device_write_sector(buf2, read_sector2);
						dloc->phoffset = aloc_block();
						dloc->index = new_haxi % 15;
						return -1;
					}
					else {
						read_sector3 = (*ptr) / SECTOR_SIZE;
						device_read_sector(buf3, read_sector3);
						ndptr = (struct inode*)buf3;
						ndptr += (new_haxi % 15);
						if (ndptr->fname[0] == 0) {
							dloc->phoffset = *ptr;
							dloc->index = new_haxi % 15;
							dptr->next_index = new_haxi;
							device_write_sector(buf2, read_sector2);
							return -1;
						}
						else {			//从后备区的溢出区找
							ptr = (unsigned int*)buf1 + (new_haxi- 9450)/90 * 6 + 635;
							nindex = 0;
							if (*ptr == NULL) {
								*ptr = sblock.data_ptr;
								device_write_sector(buf1, read_sector1);
								dptr->next_index = (new_haxi - 9450) / 90 * 90 + 9450 + 75;
								device_write_sector(buf2, read_sector2);
								dloc->phoffset = aloc_block();
								dloc->index = 0;
								return -1;
							}
							while (nindex < 15 && ndptr->fname[0] != 0) {
								nindex++;
								ndptr++;
							}
							if (nindex == 15)	printf("too many\n");
							else {
								dptr->next_index = (new_haxi - 9450) / 90 * 90 + 9450 + 75 + nindex;
								device_write_sector(buf2, read_sector2);
								dloc->phoffset = *ptr;
								dloc->index = nindex;
								return -1;
							}
						}
					}
				}					//
				else {
					dptr->next_index = haxi / 150 * 150 + 135 + nindex;
					device_write_sector(buf2, read_sector2);
					dloc->phoffset = *ptr;
					dloc->index = nindex;
					return -1;
				}
			}																		
		}
		else if (dptr->fname[0] == 0) {			//未找到
			dloc->phoffset = (*ptr);
			dloc->index = haxi % 15;
			return -1;
		}
		else {
			dloc->phoffset = (*ptr);
			dloc->index = haxi % 15;
			return dptr->finode;
		}
	}
}

int p6fs_mkdir(const char *path, mode_t mode)
{
     /*do path parse here
      create dentry  and update your index*/

	char *name = path;
	char *next_name;
	int  name_len,flag;				//-1未找到
	struct inode inode_temp,*temp;
	struct dentry dentry_temp;
	struct dlocation dloc;
	unsigned int haxi;
	if((*path)=='/'){
		temp = &root_inode;
		while(name != NULL){
			name++;
			next_name = strchr(name, '/');
			if (next_name == NULL)	name_len = strlen(name);
			else name_len = next_name - name;
			haxi = path_parse(name);
			flag = find_hash(&dloc, temp->first_point, haxi, name,name_len);
			if(flag == -1){
				temp->file_size += SECTOR_SIZE;
				init_inode(&inode_temp,temp,mode,name,name_len);
				inode_temp.file_type = LIST;
				write_inode(&inode_temp);
				init_dentry(&inode_temp,&dentry_temp,dloc.pre_index);
				write_dentry(&dentry_temp, dloc.index,dloc.phoffset);
				write_super();
				write_inode(temp);
			}
			else	read_inode(&inode_temp, flag);
			name = next_name;
			temp = &inode_temp;
		}
	}
    else return ENOTDIR;
	return 0;
}

int	rmdir(char *name, struct inode *root) {			//返回文件夹大小
	char *next_name;
	unsigned int name_len, haxi, iindex,fsize;
	struct inode inode_temp, *temp;
	struct dlocation dloc;
	next_name = strchr(name, '/');
	haxi = path_parse(name);
	if (next_name != NULL) {
		name_len = next_name - name;
		iindex = find_hash(&dloc, root->first_point, haxi, name, name_len);
		read_inode(&inode_temp, iindex);
		next_name++;
		fsize = rmdir(next_name, &inode_temp);
		root->file_size -= fsize;
		write_inode(root);
		return fsize;
	}
	else {
		name_len = strlen(name);
		iindex = find_hash(&dloc, root->first_point, haxi, name, name_len);
		read_inode(&inode_temp, iindex);
		char	buf[4096];
		memset(buf,0,4096);
		struct dentry *dptr;
		unsigned int read_sector = dloc.phoffset / SECTOR_SIZE;
		device_read_sector(buf, read_sector);
		dptr = (struct dentry *)buf + dloc.index;
		memset((void*)dptr, 0, sizeof(struct dentry));
		device_write_sector(buf, read_sector);
		write_imap(iindex,0);
		root->file_size -= inode_temp.file_size;
		write_inode(root);
		return inode_temp.file_size;
	}
}

int p6fs_rmdir(const char *path)		
{
	char *name = path + 1;
	rmdir(name,&root_inode);
	return 0;
}

int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
	char *name = path;
	char buf1[4096],buf2[4096];
	int i,j;
	unsigned int name_len, haxi, iindex, fsize;
	unsigned int *ptr;
	struct inode inode_temp, *temp;
	struct dlocation dloc;
	struct dentry *dptr;
	memset(buf1,0,4096);
	memset(buf2,0,4096);
	if(strcmp(path, "/") == 0) {
		read_inode(&root_inode, 0);
		device_read_sector(buf1, root_inode.first_point / SECTOR_SIZE);
	}
	else if(strlen(path)>0){
		iindex = fpath2inode(&name,&inode_temp);
		name_len = strlen(name);
		haxi = path_parse(name);
		iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
		read_inode(&inode_temp, iindex);
		device_read_sector(buf1, inode_temp.first_point / SECTOR_SIZE);
	}
	else return -ENOENT;
	ptr = (unsigned int*)buf1;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (i = 0; i < 1024; i++,ptr++) {
		if (*ptr != NULL) {
			device_read_sector(buf2, (*ptr) / SECTOR_SIZE);
			dptr = (struct dentry*)buf2;
			for (j = 0; j < 15; j++) {
				if (dptr->fname[0] != 0) {
					filler(buf, dptr->fname, NULL, 0);
				}
				dptr++;
			}
		}
	}
	return 0;
}

//optional
//int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo)


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev)
{
 /*do path parse here
  create file*/
	char *name = path ;
	int i, j,flag;
	unsigned int name_len, haxi, iindex;
	unsigned int *ptr;
	struct inode inode_temp, *temp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	iindex = fpath2inode(&name, &inode_temp);
	name_len = strlen(name);
	haxi = path_parse(name);
	flag = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	if (flag = -1) {
		temp->file_size += SECTOR_SIZE;
		init_inode(&inode_temp, temp, mode, name, name_len);
		inode_temp.file_type = DOCUMENT;
		write_inode(&inode_temp);
		init_dentry(&inode_temp, &dentry_temp,dloc.pre_index);
		write_dentry(&dentry_temp, dloc.index, dloc.phoffset);
		write_super();
		write_inode(temp);
	}
	return 0;
}

//int p6fs_readlink(const char *path, char *link, size_t size)

int p6fs_symlink(const char *path, const char *link)
{
	char *name = path;
	char *Lname = link;
	int i, j, flag;
	unsigned int name_len, Lname_len,haxi, iindex,Lindex,Lhaxi;
	unsigned int *ptr;
	struct inode inode_temp, link_itemp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	iindex = fpath2inode(&name, &inode_temp);
	Lindex = fpath2inode(&Lname, &link_itemp);
	Lhaxi = path_parse(Lname);
	haxi = path_parse(name);
	name_len = strlen(name);
	Lname_len = strlen(Lname);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	Lindex = find_hash(&dloc, link_itemp.first_point, haxi, Lname, Lname_len);
	read_inode(&inode_temp, iindex);
	inode_temp.file_type = LINK;
	inode_temp.slink_node = Lindex;
	write_inode(&inode_temp);
	return 0;
}

int p6fs_link(const char *path, const char *newpath)
{
	char *name = path;
	char *Lname = newpath;
	char buf[4096];
	unsigned int name_len, Lname_len, haxi, iindex, Lindex, Lhaxi;
	struct inode inode_temp, link_itemp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	memset(buf,0,4096);
	iindex = fpath2inode(&name, &inode_temp);
	Lindex = fpath2inode(&Lname, &link_itemp);
	Lhaxi = path_parse(Lname);
	haxi = path_parse(name);
	name_len = strlen(name);
	Lname_len = strlen(Lname);
	Lindex = find_hash(&dloc, link_itemp.first_point, haxi, Lname, Lname_len);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	read_inode(&link_itemp, Lindex);
	device_read_sector(buf, dloc.phoffset / SECTOR_SIZE);
	dptr = (struct dentry*)buf + dloc.index;
	//memcpy(&dentry_temp, dptr, DENTRY_SIZE);
	dptr->ftype = link_itemp.file_type;
	dptr->finode = link_itemp.f_inode;
	link_itemp.link_count += 1;
	inode_temp.file_size += (link_itemp.file_size - SECTOR_SIZE);
	device_write_sector(buf, dloc.phoffset / SECTOR_SIZE);
	write_inode(&link_itemp);
	write_inode(&inode_temp);
	return 0;
	//memcpy(dptr)
}

int p6fs_unlink(const char *path)
{
	char *name = path;
	unsigned int name_len,haxi, iindex;
	struct inode inode_temp, link_itemp;
	struct dlocation dloc;
	iindex = fpath2inode(&name, &inode_temp);
	haxi = path_parse(name);
	name_len = strlen(name);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	read_inode(&link_itemp, iindex);
	link_itemp.link_count--;
	if (link_itemp.link_count == 0) {
		free_inode(iindex);
	}
	else{
		write_inode(&link_itemp);
	}
	inode_temp.file_size -= link_itemp.file_size;
	write_inode(&inode_temp);
	return 0;
}

int p6fs_open(const char *path, struct fuse_file_info *fileInfo)
{
 /*
  Implemention Example:
  S1: look up and get dentry of the path
  S2: create file handle! Do NOT lookup in read() or write() later
  */
    

    //assign and init your file handle
    struct file_info *fi;
	char *name = path;
	int i;
	unsigned int name_len, haxi, iindex;
	struct inode inode_temp, file_itemp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	if (file_opennum == MAX_OPEN_FILE) {
		printf("too many files");
		return EOVERFLOW;
	}
	iindex = fpath2inode(&name, &inode_temp);
	haxi = path_parse(name);
	name_len = strlen(name);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	read_inode(&file_itemp, iindex);
	for (i = 0; i < MAX_OPEN_FILE; i++) {
		if (fd_table[i].inode_num == 0) {
			fi = fd_table + i;
		}
	}
	file_opennum++;
	fi->flag = fileInfo->flags;
	fi->inode_num = file_itemp.current_inode;
	//file_itemp.link_count++;
    //check open flags, such as O_RDONLY
    //O_CREATE is tansformed to mknod() + open() by fuse ,so no need to create file here
    /*
     if(fileInfo->flags & O_RDONLY){
     fi->xxxx = xxxx;
     }
     */
    
    fileInfo->fh = (uint64_t)fi;
	return 0;
}

int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
	unsigned int iindex,read_sector1, read_sector2,*addr_ptr;
	size_t psize,total_size=0;
	off_t poffset;
	char buf1[4096],buf2[4096],*ptr;
	struct file_info * fi;
	struct inode inode_temp, file_itemp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	memset(buf2,0,4096);
	memset(buf1,0,4096);
	fi = (struct file_info *)fileInfo->fh;
	iindex = fi->inode_num;
	if (fi->flag & O_WRONLY) {
		return EACCES;
	}
	read_inode(&file_itemp, iindex);
	device_read_sector(buf1, file_itemp.first_point / SECTOR_SIZE);
	read_sector1 = offset / SECTOR_SIZE;
	addr_ptr = (unsigned int *)buf1 + read_sector1;
	poffset = offset%SECTOR_SIZE;
	psize = (size > (SECTOR_SIZE - poffset)) ? (SECTOR_SIZE - poffset) : size;
	//device_read_sector(buf2, (*addr_ptr) / SECTOR_SIZE);
	while (size > 0) {
		device_read_sector(buf2, (*addr_ptr) / SECTOR_SIZE);
		memcpy(buf+ total_size, buf2 + poffset, psize);
		size -= psize;
		total_size += psize;
		addr_ptr++;
		psize = (size > SECTOR_SIZE) ? SECTOR_SIZE : size;
		poffset = 0;
	}
	return 0;
}

int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
	unsigned int iindex, write_sector1, write_sector2, *addr_ptr;
	size_t psize,total_size=0;
	off_t poffset;
	char buf1[4096], buf2[4096], *ptr;
	struct inode inode_temp, file_itemp;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	struct file_info * fi;
	memset(buf2,0,4096);
	memset(buf1,0,4096);
	fi = (struct file_info *)fileInfo->fh;
	iindex = fi->inode_num;
	if (fi->flag & O_WRONLY) {
		return EACCES;
	}
	read_inode(&file_itemp, iindex);
	device_read_sector(buf1, file_itemp.first_point / SECTOR_SIZE);
	write_sector1 = offset / SECTOR_SIZE;
	addr_ptr = (unsigned int *)buf1 + write_sector1;
	poffset = offset%SECTOR_SIZE;
	psize = (size > (SECTOR_SIZE - poffset)) ? (SECTOR_SIZE - poffset) : size;
	while (size > 0) {
		if (*addr_ptr == 0) {
			*addr_ptr = aloc_block();
			file_itemp.file_size += SECTOR_SIZE;
		}
		write_sector1 = (*addr_ptr) / SECTOR_SIZE;
		device_read_sector(buf2, write_sector1);
		memcpy(buf2 + poffset, buf+ total_size, psize);
		device_write_sector(buf2, write_sector1);
		size -= psize;
		total_size += psize;
		addr_ptr++;
		poffset = 0;
		psize = (size > SECTOR_SIZE) ? SECTOR_SIZE : size;
	}
	write_inode(&file_itemp);
	write_super();
	return 0;
}

int p6fs_truncate(const char *path, off_t newSize)
{
	unsigned int iindex, truncate_sector, delete_sector, *addr_ptr;
	size_t psize;
	off_t poffset;
	char buf1[4096];
	char *name = path;
	unsigned int name_len, haxi;
	struct inode  file_itemp, inode_temp;
	struct dlocation dloc;
	memset(buf1,0,4096);
	iindex = fpath2inode(&name, &inode_temp);
	haxi = path_parse(name);
	name_len = strlen(name);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);

	read_inode(&file_itemp, iindex);
	device_read_sector(buf1, file_itemp.first_point / SECTOR_SIZE);
	truncate_sector = newSize / SECTOR_SIZE + 1;
	psize = truncate_sector * sizeof(unsigned int);
	poffset = psize;
	memset(buf1 + poffset, 0, SECTOR_SIZE - psize);
	device_write_sector(buf1, file_itemp.first_point / SECTOR_SIZE);
	delete_sector = file_itemp.file_size / SECTOR_SIZE - truncate_sector;
	file_itemp.file_size = (truncate_sector+1) * SECTOR_SIZE;
	write_inode(&file_itemp);
	return 0;
}

//optional
//p6fs_flush(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
int p6fs_release(const char *path, struct fuse_file_info *fileInfo)
{
    /*release fd*/
	struct file_info *file = fileInfo->fh;
	fileInfo->fh = NULL;
	file_opennum--;
	file->inode_num = 0;
	return 0;
}
int p6fs_getattr(const char *path, struct stat *statbuf)
{
    /*stat() file or directory */
	char *name = path;
	unsigned int name_len, haxi; 
	int iindex;
	unsigned int *ptr;
	struct inode inode_temp, *iptr;
	struct dlocation dloc;
	struct dentry *dptr, dentry_temp;
	//memset(statbuf,0,sizeof(struct stat));
	int res = 0;

    	memset(statbuf, 0, sizeof(struct stat));
    	if(strcmp(path, "/") == 0) {
        	statbuf->st_mode = S_IFDIR | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        	statbuf->st_nlink = 2;
		statbuf->st_size = root_inode.file_size;
   	}
    	else if(strlen(path)>0) {
		iindex = fpath2inode(&name, &inode_temp);
		haxi = path_parse(name);
		name_len = strlen(name);
		iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
		if(iindex == -1) return -ENOENT;
		read_inode(&inode_temp, iindex);
		iptr = &inode_temp;
        	statbuf->st_mode = S_IFREG | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        	statbuf->st_nlink = iptr->link_count;
        	statbuf->st_size = iptr->file_size ;
    	}
   	 else
       	 res = -ENOENT;
	return res;
}
/*
int p6fs_utime(const char *path, struct utimbuf *ubuf);//optional
int p6fs_chmod(const char *path, mode_t mode); //optional
int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional
*/

int p6fs_rename(const char *path, const char *newpath)
{
	char *name = path;
	char *Nname = newpath;
	int i, j, flag;
	unsigned int name_len, Nname_len, haxi, iindex, Nindex, Nhaxi;
	unsigned int *ptr;
	struct inode inode_temp, New_itemp, iinode;
	struct dlocation dloc,Ndloc;
	struct dentry *dptr, dentry_temp,pre_dentry,next_dentry;
	iindex = fpath2inode(&name, &inode_temp);
	Nindex = fpath2inode(&Nname, &New_itemp);
	Nhaxi = path_parse(Nname);
	haxi = path_parse(name);
	name_len = strlen(name);
	Nname_len = strlen(Nname);
	iindex = find_hash(&dloc, inode_temp.first_point, haxi, name, name_len);
	Nindex = find_hash(&Ndloc, New_itemp.first_point, haxi, Nname, Nname_len);
	read_inode(&iinode, iindex);
	inode_temp.file_size -= iinode.file_size;
	iinode.f_inode = New_itemp.current_inode;
	New_itemp.file_size += iinode.file_size;
	write_inode(&iinode);
	write_inode(&New_itemp);
	write_inode(&inode_temp);
	read_dentry(&dentry_temp, dloc.index, dloc.phoffset);
	read_dentry(&pre_dentry, dloc.pre_index, dloc.phoffset);
	read_dentry(&next_dentry, dentry_temp.next_index, dloc.phoffset);
	pre_dentry.next_index = dentry_temp.next_index;
	next_dentry.pre_index = dentry_temp.pre_index;
	dentry_temp.next_index = -1;
	dentry_temp.pre_index = Ndloc.pre_index;
	write_dentry(&pre_dentry, dloc.pre_index, dloc.phoffset);
	write_dentry(&next_dentry, pre_dentry.next_index, dloc.phoffset);
	write_dentry(&dentry_temp, Ndloc.index, Ndloc.phoffset);
	return 0;
}

int p6fs_statfs(const char *path, struct statvfs *statInfo)
{
    /*print fs status and statistics */
	statInfo->f_bsize = SECTOR_SIZE;
	statInfo->f_blocks = sblock.f_blocks;
	statInfo->f_bfree = sblock.f_bfree;
	statInfo->f_files = sblock.f_files;
	statInfo->f_ffree = sblock.f_ffree;
	statInfo->f_fsid = 0;
	statInfo->f_flag = MAGIC;
	return 0;
}

void* p6fs_init(struct fuse_conn_info *conn)
{
    /*init fs
     think what mkfs() and mount() should do.
     create or rebuild memory structures.
     
     e.g
     S1 Read the magic number from disk
     S2 Compare with YOUR Magic
     S3 if(exist)
        then
            mount();
        else
            mkfs();
     */
    
    
    /*HOWTO use @return
     struct fuse_context *fuse_con = fuse_get_context();
     fuse_con->private_data = (void *)xxx;
     return fuse_con->private_data;
     
     the fuse_context is a global variable, you can use it in
     all file operation, and you could also get uid,gid and pid
     from it.
     
     */
	//printf("Your FS is initializing...\n");
	logging_open("debug_LOG.txt");
	__LOG(1,"Your FS is initializing...\n",0,"hh");
	int	i,j,flag=0;
	unsigned char	buff[4096];
	for(i = 0; i< 4096;i++)	buff[i] = 0;
	struct superblock_t *sblock_t;
	unsigned int read_block;
	unsigned char *ptr = bit_map;
	struct inode_t *iptr;
	struct dentry *dptr;
	device_read_sector(buff,0);
	sblock_t = (struct superblock_t*)buff;
	if(sblock_t->magic == MAGIC){						//mount
		sblock.f_blocks = sblock_t->f_blocks;
		sblock.f_files = sblock_t->f_files;
		sblock.f_ffree = sblock_t->f_ffree;
		sblock.f_bfree = sblock_t->f_bfree;
		sblock.imap_offset = sblock_t->imap_offset - sblock_t->Bmap_offset;
		sblock.Itable_offset = sblock_t->Itable_offset;
		sblock.Itable_ptr = (struct inode_t *)sblock.Itable_offset + sblock_t->Itable_len;
		sblock.datablock_offset = sblock_t->datablock_offset;
		sblock.data_ptr = sblock.datablock_offset + sblock_t->datablock_len * SECTOR_SIZE;
		sblock.sb_offset = 0;
		read_block = sblock_t->Bmap_offset/SECTOR_SIZE;
		for(i = 0;i < 34;i++,read_block++){
			device_read_sector(buff,read_block);
			for(j=0;j<SECTOR_SIZE;j++,ptr++){
				*ptr = buff[i];
			}
		}
		read_inode(&root_inode, 0);
		file_opennum = 0;
		for (i = 0; i < MAX_OPEN_FILE; i++) {
			fd_table[i].inode_num = 0;
		}
	}
	else{											//mkfs
		sblock_t->magic = MAGIC;
		sblock_t->f_blocks = DATA_BLOCK_NUM;
		sblock_t->f_files = 8*INODE_BLOCK_NUM;
		sblock_t->f_ffree = sblock_t->f_files-1;
		sblock_t->f_bfree = DATA_BLOCK_NUM-1;
		sblock_t->Sblock2_offset = SECTOR_SIZE;
		sblock_t->Sblock_len = 64;
		sblock_t->Bmap_offset = 2*SECTOR_SIZE;
		sblock_t->Bmap_len = DATA_MAP_SIZE;
		sblock_t->imap_offset = sblock_t->Bmap_offset + DATA_MAP_SIZE;
		sblock_t->imap_len = INODE_MAP_SIZE;
		sblock_t->Itable_offset = 36*SECTOR_SIZE;
		sblock_t->Itable_len = 1;
		sblock_t->datablock_offset = sblock_t->Itable_offset + sblock_t->Itable_len;
		sblock_t->datablock_len = 1;
		device_write_sector(buff, 0);
		device_write_sector(buff, 1);
		for(i = 0; i< 4096;i++)	buff[i] = 0;
		iptr = (struct inode_t *)buff;
		iptr->fname[0] = '/';
		iptr->fname[1] = '\0';
		iptr->file_type = LIST;
		iptr->file_size = 4096;
		iptr->link_count = 0;
		iptr->current_inode = 0;
		iptr->f_inode = 0;
		iptr->creat_time = 0;
		iptr->first_point = 15172*SECTOR_SIZE;
		iptr->mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		device_write_sector(buff, 36);
		for(i = 0; i< 4096;i++)	buff[i] = 0;
		device_write_sector(buff, 15172);
		buff[0] = 0x1;
		device_write_sector(buff, 2);
		buff[0] = 0;
		for(i = 3; i< 33;i++)
			device_write_sector(buff, i);
		buff[1248] = 0x1;
		device_write_sector(buff, 33);
		buff[1248] = 0;
		device_write_sector(buff, 34);
		device_write_sector(buff, 35);
	}
    return NULL;
}
void p6fs_destroy(void* private_data)
{
    /*
     flush data to disk
     free memory
     */
    device_close();
    logging_close();
}
