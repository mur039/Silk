#ifndef __EXT2_H__
#define __EXT2_H__

#include <dev.h>
#include <filesystems/vfs.h>
#include <filesystems/fs.h>
#include <fb.h>
#include <module.h>


#define EXT2_SUPER_MAGIC 0xEF53

/*
https://www.nongnu.org/ext2-doc/ext2.pdf
*/
typedef struct{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;

    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;

    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;

    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    /*-- EXT2_DYNAMIC_REV Specific -- 1.0 has it*/
    uint32_t s_first_ino;

    uint16_t s_inode_size;
    uint16_t s_block_group_nr;

    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;

    char s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];

    uint32_t s_algo_bitmap;

    /*-- Performance Hints --*/
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t _alignment;

    /*-- Journaling Support --*/
    char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;

    /*-- Directory Indexing Support --*/
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t _padding[3];

    /*-- Other options --*/
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;

    uint8_t _unused[760];
} __attribute__((packed)) ext2_superblock_t;


typedef struct{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;

    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;

    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;

    uint8_t bg_reserved[12];

} __attribute__((packed)) ext2_block_group_descriptor_table_t;



//i_mode
#define EXT2_S_IFSOCK 0xC000 //socket
#define EXT2_S_IFLNK  0xA000 //symbolic link
#define EXT2_S_IFREG  0x8000 //regular file
#define EXT2_S_IFBLK  0x6000 //block device
#define EXT2_S_IFDIR  0x4000 //directory
#define EXT2_S_IFCHR  0x2000 //character device
#define EXT2_S_IFIFO  0x1000 //fifo

typedef struct{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;

    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];

} __attribute__((packed)) ext2_inode_t;

typedef struct{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[255]; //not fixed so i will &name it
} __attribute__((packed)) ext2_directory_linked_list_t;


#define EXT2_ROOT_DIRECTORY_INODE 2
typedef struct{
    
    ext2_superblock_t* superblock;
    fs_node_t* device;
    uint32_t block_size; //to spare some calculations
    uint32_t n_groups_blocks;
} ext2_data_struct_t;

fs_node_t * ext2_node_create(fs_node_t* node);
int ext2_get_inode(fs_node_t* node, uint32_t inode, ext2_inode_t* outinode);


#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001 //Disk/File compression is used
#define EXT2_FEATURE_INCOMPAT_FILETYPE 0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER 0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG 0x0010



struct dirent* ext2_readdir( fs_node_t* node, uint32_t index);
struct fs_node* ext2_finddir( fs_node_t* node, char* name);
uint32_t ext2_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

mkdir_type_t ext2_mkdir(fs_node_t* node, const char* name, uint16_t permission);


#endif