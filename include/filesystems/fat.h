#ifndef __FAT_H__
#define __FAT_H__

#include <dev.h>
#include <filesystems/vfs.h>
#include <fb.h>


#define FAT_TYPE_12 0x12
#define FAT_TYPE_16 0x16
#define FAT_TYPE_32 0x32


//fat12/16/32 common bootsect
typedef struct{
        uint8_t BS_jmpBoot[3]; //unconditioanl jump 
        char BS_OEMNAme[8]; //unconditioanl jump 
        uint16_t BPB_BytsPerSec;
        uint8_t BPB_SecPerClus;
        uint16_t BPB_RsvdSecCnt;
        uint8_t BPB_NumFATs;
        uint16_t BPB_RootEntCnt;
        uint16_t BPB_TotSec16;
        uint8_t BPB_Media;
        uint16_t BPB_FATSz16;
        uint16_t BPB_SecPerTrk;
        uint16_t BPB_NumHeads;
        uint32_t BPB_HiddSec;
        uint32_t BPB_TotSec32;
        
    } __attribute__((packed)) fat_bootsector_common_t;


//fat12/16 specific
typedef struct{
    fat_bootsector_common_t common;
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
} __attribute__((packed)) fat_bootsector_12_16_t;


//fat32 specific
typedef struct{
    fat_bootsector_common_t comm;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    char BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];

} __attribute__((packed)) fat_bootsector_32_t;




typedef union{
    fat_bootsector_common_t common;
    char _align[512];
} fat_boot_sector_t;


/*
f(FATType == FAT12) {
If(FATContent >= 0x0FF8)
IsEOF = TRUE;
} else if(FATType == FAT16) {
If(FATContent >= 0xFFF8)
IsEOF = TRUE;
} else if (FATType == FAT32) {
If(FATContent >= 0x0FFFFFF8)
IsEOF = TRUE;
}
*/


typedef struct{

    uint8_t fs_type;
    void* sector0; //will be selected using fs_type


    int sector_offset;
    int cluster_index;
    

	fs_node_t* dev;
    fs_node_t* root_node;
} fat_struct_t;


typedef struct{
    uint8_t dir_name[11];
    uint8_t dir_attr; 
    uint8_t dir_NTRes; //reserved
    uint8_t dir_creation_time_milli;
    uint16_t dir_crt_time;
    uint16_t dir_crt_date;
    uint16_t dir_last_access_time;
    uint16_t dir_first_cluster_hi;
    uint16_t dir_write_time;
    uint16_t dir_write_date;
    uint16_t dir_first_cluster_lo;
    uint32_t dir_file_size;

} __attribute__((packed)) fat_directory_entry_t;


typedef struct{
    uint8_t long_dir_order;
    char long_dir_name1[10];
    uint8_t long_dir_attr;  //must be longnamedir
    uint8_t long_dir_type; //if zero indicates subcomponent
    uint8_t long_dir_cheksum; //checksum of dos_dir_name
    char long_dir_name2[12];
    uint16_t long_dir_fst_clust_lo; //meaningless
    char long_dir_name3[4]; 

} __attribute__((packed)) fat_long_name_directory_entry_t;

/*
0 – Reserved Region
1 – FAT Region
2 – Root Directory Region (doesn’t exist on FAT32 volumes)
3 – File and Directory Data Region
*/

/*
    0x000: Unused
    0x001: Reserved Cluster
    0x002 – 0xFEF: The cluster is in use, and the value represents the next cluster
    0xFF0 – 0xFF6: Reserved Cluster
    0xFF7: Bad Cluster
    0xFF8 – 0xFFF: Last Cluster in a file
*/

#define FAT_DIR_ATTR_READ_ONLY    0x1
#define FAT_DIR_ATTR_HIDDEN       0x2
#define FAT_DIR_ATTR_SYSTEM       0x4
#define FAT_DIR_ATTR_VOLUME_ID    0x8
#define FAT_DIR_ATTR_LONGNAME     0xF
#define FAT_DIR_ATTR_DIRECTORY    0x10
#define FAT_DIR_ATTR_ARCHIVE      0x20

fs_node_t * fat_node_create( fs_node_t* node);

fat_struct_t* fat_probe_block_dev(fs_node_t* device);

int fat_create_ascii_from_dir(fat_directory_entry_t dir, char*  target_ascii);
int32_t fat_read_fat_cluster(fs_node_t* node, int cluster, int fs_type);


struct dirent * fat12_readdir(fs_node_t *node, uint32_t index);
finddir_type_t fat12_finddir(struct fs_node* node, char *name);
read_type_t fat12_read_fs(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);


struct dirent * fat32_readdir(fs_node_t *node, uint32_t index);
finddir_type_t fat32_finddir(struct fs_node* node, char *name);
read_type_t fat32_read(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
write_type_t fat32_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

mkdir_type_t fat32_mkdir(fs_node_t* node, const char* name, uint16_t permission);





#endif