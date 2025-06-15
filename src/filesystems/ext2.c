#include <filesystems/ext2.h>





fs_node_t * ext2_node_create(fs_node_t* node){

    device_t* blk_device = node->device;
    if(blk_device->dev_type != DEVICE_BLOCK) return NULL;

    //well let's read the superblock then
    uint8_t* superblock = kmalloc(1024);
    read_fs(node, 1024, 1024, superblock);
    ext2_superblock_t* superblockptr = (ext2_superblock_t*)superblock;

    if(superblockptr->s_magic != EXT2_SUPER_MAGIC){
        fb_console_printf("ext2fs: Invalid magic number\n");
        kfree(superblock);
        return NULL;
    }

    int main_revision = superblockptr->s_rev_level;
    int minor_revision = superblockptr->s_minor_rev_level;
    fb_console_printf("ext2fs revision %u.%u\n", main_revision, minor_revision);

    if(superblockptr->s_feature_incompat & EXT2_FEATURE_INCOMPAT_COMPRESSION){
        fb_console_printf("Compression is not supported\n");
        kfree(superblock);
        return NULL;
    }

    uint32_t block_size = 1024 << superblockptr->s_log_block_size;
    uint32_t frag_size = 1024 << superblockptr->s_log_frag_size;

    uint32_t n_group_blocks = (superblockptr->s_blocks_count / superblockptr->s_blocks_per_group) + (superblockptr->s_blocks_count % superblockptr->s_blocks_per_group != 0);
    
    //i know but let's say we wan't to read inode 2,which resides in group 0, inode index 2 so
    ext2_inode_t inode;
    if(ext2_get_inode(node, 2, &inode)){
        
        fb_console_put("Failed to find root directory :(\n");
        kfree(superblock);
        return NULL;
    }
    

    // for(int i = 1; i <= superblockptr->s_inodes_count; ++i){
        
    //     ext2_inode_t inode;
    //     if(!ext2_get_inode(node, i, &inode)){
            
    //         fb_console_printf("Inode %u\n", i );
    //         switch( inode.i_mode >> 12){
                
    //             case 4: fb_console_printf("Directory, size: %u\n", inode.i_size);break;
    //             case 8: fb_console_printf("Regular file, size: %u\n", inode.i_size);break;
    //             default: fb_console_put("Wut?\n");break;
    //         }
    //     }
    // }

    

    //should probe the drive for ext2 specific information
    fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));

    strcpy(fnode->name, "ext2");
    fnode->inode = EXT2_ROOT_DIRECTORY_INODE;
    fnode->impl = 6;
    fnode->flags = FS_DIRECTORY;

    ext2_data_struct_t* priv = kcalloc(1, sizeof(ext2_data_struct_t));
    priv->block_size = block_size;
    priv->device = node;
    priv->n_groups_blocks = n_group_blocks;
    //only 268 bytes are used so
    priv->superblock = kmalloc(264); 
    memcpy(priv->superblock, superblock, 264);
    kfree(superblock);
    fnode->device = priv;

    fnode->readdir = (readdir_type_t)ext2_readdir;
    fnode->finddir = (finddir_type_t)ext2_finddir;



    return fnode;
}


int32_t ext2_vfs[8] = {
    [0] = -1,
    [1] = FS_FILE,
    [2] = FS_DIRECTORY,
    [3] = FS_CHARDEVICE,
    [4] = FS_BLOCKDEVICE,
    [5] = FS_PIPE,
    [6] = FS_SOCKET,
    [7] = FS_SYMLINK
    
};

int ext2_get_inode(fs_node_t* node, uint32_t inode, ext2_inode_t* outinode) {
    
    int result = 0;
    // Read the superblock
    uint8_t* superblock = kmalloc(1024);
    read_fs(node, 1024, 1024, superblock);
    ext2_superblock_t* superblockptr = (ext2_superblock_t*)superblock;
    int block_size = 1024 << superblockptr->s_log_block_size;


    // Locate the block group and local index
    uint32_t block_group = (inode - 1) / superblockptr->s_inodes_per_group;
    uint32_t local_inode_index = (inode - 1) % superblockptr->s_inodes_per_group;

    // Read the Block Group Descriptor Table entry for this group
    ext2_block_group_descriptor_table_t bgdt;
    read_fs(node, block_size + (block_group * sizeof(ext2_block_group_descriptor_table_t)), 
            sizeof(ext2_block_group_descriptor_table_t), (uint8_t*)&bgdt);

    // Retrieve the inode bitmap and check if the inode is allocated
    uint8_t sub_bitmap;
    read_fs(node, (bgdt.bg_inode_bitmap * block_size) + (local_inode_index / 8), 
            1, &sub_bitmap);

    int local_bit_index = local_inode_index % 8;
    
    if (GET_BIT(sub_bitmap, local_bit_index)) { // If the inode is allocated
        // Retrieve the correct inode
        uint32_t inode_offset = (bgdt.bg_inode_table * block_size) + (local_inode_index * superblockptr->s_inode_size ); //it has variable
        read_fs(node, inode_offset, sizeof(ext2_inode_t), (uint8_t*)outinode);
        result = 0;
    } else {
        result = 1; // Inode is not allocated
    }

    kfree(superblock);
    return result;
}



struct dirent* ext2_readdir( fs_node_t* node, uint32_t index){

    ext2_data_struct_t* ext2 = node->device;
    
    ext2_inode_t root_inode;
    if(ext2_get_inode(ext2->device, node->inode, &root_inode)){ //an error
        node->offset = 0;
        return NULL;
    }

    for(size_t offset = 0, i = 0; offset < root_inode.i_size; i++){
        
        ext2_directory_linked_list_t idirent;
        read_fs(ext2->device, (ext2->block_size * root_inode.i_block[0]) + offset, sizeof(ext2_directory_linked_list_t), (uint8_t*)&idirent);
        offset += idirent.rec_len; 

        if( idirent.inode == 0){
            // i--;
        }
        else{
            if(i >= index){
                struct dirent* out = kcalloc(1, sizeof(struct dirent));
                out->ino = idirent.inode;
                out->off = index;
                out->reclen = idirent.rec_len;
                memcpy(out->name, idirent.name, idirent.name_len);
                out->type = ext2_vfs[idirent.file_type];
                node->offset++;
                return out;
            }
        }

    }


    node->offset = 0;
    return NULL;
}

struct fs_node* ext2_finddir( fs_node_t* node, char* name){

    ext2_data_struct_t* ext2 = node->device;
    ext2_inode_t root_inode;
    if(ext2_get_inode(ext2->device, node->inode, &root_inode)){ //an error
        return NULL;
    }

    if( (root_inode.i_mode & 0xF000) != EXT2_S_IFDIR){
        return NULL;
    }
    
    for(size_t offset = 0, i = 0; offset < root_inode.i_size; i++){
        
        ext2_directory_linked_list_t idirent;
        read_fs(ext2->device, (ext2->block_size * root_inode.i_block[0]) + offset, sizeof(ext2_directory_linked_list_t), (uint8_t*)&idirent);
        offset += idirent.rec_len; 

        if(idirent.inode == 0){
            i--;
        }
        else if(!strncmp(name, idirent.name, idirent.name_len) ){
            
            ext2_inode_t finode;
            ext2_get_inode(ext2->device, idirent.inode, &finode);
            fs_node_t* fnode = kcalloc(1, sizeof(fs_node_t));
            fnode->inode = idirent.inode;
            fnode->impl = 6;

            fnode->device = node->device;
            strcpy(fnode->name, name);
            fnode->flags = ext2_vfs[idirent.file_type];

            if(fnode->flags == FS_DIRECTORY){
                
                fnode->readdir = (readdir_type_t)ext2_readdir;
                fnode->finddir = (finddir_type_t)ext2_finddir;
            }
            else if(fnode->flags == FS_FILE){
                
                fnode->read = (read_type_t)ext2_read;
            }
            

            return fnode;
            

        }
    }


    return NULL;

}

uint32_t ext2_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){

    ext2_data_struct_t* ext2 = node->device;
    ext2_inode_t inode;
    if(ext2_get_inode(ext2->device, node->inode, &inode)){ //an error
        return 0;
    }

    if( (inode.i_mode & 0xF000) == EXT2_S_IFDIR || (inode.i_mode & 0xF000) == EXT2_S_IFLNK){
        fb_console_printf("That's not a file/blk_device/chr_device\n");
        return 0;
    }
    //first 12 blocks are direct, 13 one indirect, 14 doubly indirect
    if(offset >= inode.i_size){
        //EOF
        return 0;
    }
    

    uint32_t block_index = offset / ext2->block_size;
    uint32_t offset_within_block = offset % ext2->block_size;

    if(block_index > 11){ //max 48kB
        return 0;
    }

    //for now make it inefficient as fuck
    size_t left = size;
    
    while(left--){

        read_fs(ext2->device, (ext2->block_size * inode.i_block[block_index]) + offset_within_block++, 1, buffer++);
        if(offset_within_block >= ext2->block_size){
            
            offset_within_block = 0;
            block_index++;

            if(block_index > 11){
                //abort the thing
                return (size - left);
            }
        }
    }
    return size;
}

mkdir_type_t ext2_mkdir(fs_node_t* node, const char* name, uint16_t permission){

    ext2_data_struct_t* ext2 = node->device;
    ext2_inode_t inode;
    if(ext2_get_inode(ext2->device, node->inode, &inode)){ //an error
        return 0;
    }

    //what we need to do is find and empty inode, mark it as allocated, fill the inode, and add it in root_dir 
    //block's directory entry

}