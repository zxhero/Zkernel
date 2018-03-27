#ifndef P6_COMMON
#define P6_COMMON

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Maybe you need the pthread locks or speedup by multi-threads or background GC in task2
// check if your $(CC) add -lfuse -pthread to $(CFLAGS) automatically, if not add them manually.
#include <pthread.h>

#include "disk.h"
#include "logging.h"
#define		INODE_SIZE 			512		//inode in disk
#define		INODE_MAP_SIZE		15136
#define		INODE_BLOCK_NUM		15136
#define		DATA_MAP_SIZE		128224
#define		DATA_BLOCK_NUM		1025792
#define		DENTRY_SIZE			272
#define		LIST				0
#define		DOCUMENT			1
#define		LINK				2


/*   on-disk  data structure   */
struct superblock_t{
	unsigned int magic;
	unsigned int fs_mode;
	unsigned int Sblock2_offset;
	unsigned int Sblock_len;
	unsigned int Bmap_offset;
	unsigned int Bmap_len;
	unsigned int imap_offset;
	unsigned int imap_len;
	unsigned int Itable_offset;
	unsigned int Itable_len;
	unsigned int datablock_offset;
	unsigned int datablock_len;
	unsigned int f_blocks;
	unsigned int f_files;
	unsigned int f_ffree;
	unsigned int f_bfree;
    // complete it
};

struct inode_t{
    // complete it
	char fname[256];
	unsigned int file_type;
	mode_t mode;
	unsigned int file_size;
	unsigned int link_count;
	unsigned int creat_time;
	//unsigned int imm_point[10];   	//10个块
	unsigned int first_point;		//1024个块（4M）
	unsigned int f_inode;
	unsigned int current_inode;
	unsigned int slink_node;
};


struct dentry{
    // complete it
	char fname[256];
	unsigned int finode;
	unsigned int ftype;
	unsigned int next_index;		//-1表示不存在
	unsigned int pre_index;
};

/*  in-memory data structure   */

struct superblock{
	unsigned int f_blocks;
	unsigned int f_files;
	unsigned int f_ffree;
	unsigned int f_bfree;
	unsigned int imap_offset;
    unsigned int sb_offset;
	unsigned int Itable_offset;
	struct inode_t *Itable_ptr;
	unsigned int datablock_offset;
	unsigned int data_ptr;
    // Add what you need, Like locks
};

struct inode{
	char fname[256];
	unsigned int file_type;
	unsigned int file_size;
	unsigned int link_count;
	unsigned int creat_time;
	unsigned int mode;
	//unsigned int imm_point[10];
	unsigned int current_inode;
	unsigned int f_inode;//第一次创建时的父节点
	unsigned int first_point;
	unsigned int slink_node;
    // Add what you need, Like locks
};

/*Your file handle structure, should be kept in <fuse_file_info>->fh
 (uint64_t see fuse_common.h), and <fuse_file_info> used in all file operations  */
struct file_info{
	unsigned int	flag;		
	unsigned int	inode_num;
    // complete it
};


//Interf.  See "fuse.h" <struct fuse_operations>
//You need to implement all the interfaces except optional ones

//dir operations
int p6fs_mkdir(const char *path, mode_t mode);
int p6fs_rmdir(const char *path);
int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo);//optional


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev);
int p6fs_symlink(const char *path, const char *link);
int p6fs_link(const char *path, const char *newpath);
int p6fs_unlink(const char *path);
int p6fs_readlink(const char *path, char *link, size_t size);//optional

int p6fs_open(const char *path, struct fuse_file_info *fileInfo);
int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_truncate(const char *path, off_t newSize);
int p6fs_flush(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi);//optional
int p6fs_release(const char *path, struct fuse_file_info *fileInfo);


int p6fs_getattr(const char *path, struct stat *statbuf);
int p6fs_utime(const char *path, struct utimbuf *ubuf);//optional
int p6fs_chmod(const char *path, mode_t mode); //optional
int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional

int p6fs_rename(const char *path, const char *newpath);
int p6fs_statfs(const char *path, struct statvfs *statInfo);
void* p6fs_init(struct fuse_conn_info *conn);
void p6fs_destroy(void* private_data);//optional

#endif
