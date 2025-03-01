#include <filesystems/fat.h>
#include <filesystems/ata.h>

#define SECTOR_SIZE 512
#define FAT12_CLUSTER_FREE 0x000
#define FAT12_CLUSTER_END  0xFFF


fat_struct_t* fat_probe_block_dev(fs_node_t* device){

	
	uint8_t* sector0 = kmalloc(512);
	
	read_fs(device, 0, 512, sector0);
	fat_bootsector_common_t* comm = (void*)sector0;

	//altough this driver only supports sectors of 512bytes if on accident user puts a block device that this field is 0, we get divisionbyzero
	if(comm->BPB_BytsPerSec < 512 || comm->BPB_SecPerClus < 1){
		kfree(sector0);
		return NULL;
	}


	//determine filesystem type 12,16,32?
	uint32_t RootDirSectors = ((comm->BPB_RootEntCnt * 32) + (comm->BPB_BytsPerSec - 1)) / comm->BPB_BytsPerSec;

	uint32_t fat_sz;
	if(comm->BPB_FATSz16 != 0){
		fat_sz = comm->BPB_FATSz16;
	}
	else{
		fat_bootsector_32_t* f32 = (void*)sector0;
		fat_sz = f32->BPB_FATSz32;
	}
	
	uint32_t tot_sec;
	if(comm->BPB_TotSec16 != 0){

		tot_sec = comm->BPB_TotSec16;
	}
	else{
		tot_sec = comm->BPB_TotSec32;
	}

	uint32_t DataSec = tot_sec - (comm->BPB_RsvdSecCnt + (comm->BPB_NumFATs * fat_sz) + RootDirSectors);
	uint32_t count_of_clusters = DataSec/ comm->BPB_SecPerClus;


	int file_type;
	if(count_of_clusters < 4085) {
		/* Volume is FAT12 */
		file_type = FAT_TYPE_12;
	} else if(count_of_clusters < 65525) {
		/* Volume is FAT16 */
		file_type = FAT_TYPE_16;
	} else {
		/* Volume is FAT32 */
		file_type = FAT_TYPE_32;
	}




	fat_struct_t* fat = kmalloc(sizeof(fat_struct_t));
	fat->dev = device;
	fat->fs_type = file_type;
	fat->sector0 = sector0;

	return fat;


}

fs_node_t * fat_node_create( fs_node_t* node){

	device_t *dev = node->device;

    if(dev->dev_type != DEVICE_BLOCK){
        
        return NULL;
    }


    fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));
	
	fat_struct_t* fat = fat_probe_block_dev(node);
	if(!fat){ //not a valid fat partitions
		kfree(fnode);
		return NULL;
	}

	fnode->inode = 0;
	sprintf(fnode->name, "fat%x", fat->fs_type);
	fb_console_printf("fat_node_create: nodename: %s\n", fnode->name);
	// strcpy(fnode->name, "fat12");
    fnode->impl = 5; // ??
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_DIRECTORY;

	fnode->read    = NULL;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;
	fnode->ioctl   = NULL;

    fnode->device = fat;
	fat->root_node = fnode;
	fat->cluster_index = 1; //determines reserved cluster which is rootdirectory

	switch(fat->fs_type){
		case FAT_TYPE_12:
			break;

		case FAT_TYPE_16:
			break;

		case FAT_TYPE_32:
			;
			fnode->readdir = fat32_readdir;
			fnode->finddir = fat32_finddir;
			fnode->mkdir   = fat32_mkdir;
			fat_bootsector_32_t* fat32_sp = fat->sector0;
			fat->cluster_index = fat32_sp->BPB_RootClus;
			break;

	}


	//since this is going to be root node of the disk first sector and root node is a directory, first sector is root_directory_sector;
	return fnode;
}



int fat_create_ascii_from_dir(fat_directory_entry_t dir, char*  target_ascii){
	if(dir.dir_attr != FAT_DIR_ATTR_LONGNAME) //obv
		return 0; 

	if(!target_ascii) //can't dereference a nullpntr can we?
		return 0;

	fat_long_name_directory_entry_t* ldir = &dir;
	//with assumption that target_ascii has atleast 14 bytes of space 13 for string and null for termination
	uint16_t cs2_string_buffer[13];
	
	memcpy(&cs2_string_buffer[0], ldir->long_dir_name1, 2*5);
	memcpy(&cs2_string_buffer[5], ldir->long_dir_name2, 2*6);
	memcpy(&cs2_string_buffer[11],ldir->long_dir_name3, 2*2);

	//convert cs2, or utf-16, string to ascii by, well throwing higher byte away :)
	//and also is long name is smaller than 13 byte, it is  null terminated and padded with 0xffffs
	int i;
	for(i = 0; i < 13 && cs2_string_buffer[i] & 0xff; ++i){

		target_ascii[i] = cs2_string_buffer[i] & 0xff;
		target_ascii[i + 1] = '\0';

	}

	return i;
}


int fat_create_usc2_from_ascii_n(char* ascii_str, uint16_t* ucs2_str, size_t length){
	
	int ispadding = 0;
	for(size_t i = 0; i < length; ++i){
		
		if(!ispadding){
	
			ucs2_str[i] = 0x0000;
			
			if(ascii_str[i]){	ucs2_str[i] |= ascii_str[i];	}
			else{	ispadding = 1;	}
		}
		else{
			
			ucs2_str[i] = 0xffff;
		}
	}		
	return 0;
}




struct dirent * fat32_readdir(fs_node_t *node, uint32_t index){
	
	fat_struct_t* fat = node->device;

	fs_node_t* dev = fat->dev;

	fat_directory_entry_t dir;
	list_t longname_list = list_create();

	fat_bootsector_32_t* bpb = fat->sector0;
	int data_start_sector = bpb->comm.BPB_RsvdSecCnt + ( bpb->comm.BPB_NumFATs * bpb->BPB_FATSz32);
	int dir_start_offset;
	dir_start_offset =  (data_start_sector + ((fat->cluster_index - 2)*bpb->comm.BPB_SecPerClus));
	dir_start_offset *= bpb->comm.BPB_BytsPerSec;



	uint32_t i = 0;
	while(1){ //search for entire directory to find folder/file

		read_fs(dev, dir_start_offset, sizeof(fat_directory_entry_t), &dir);
		// dev->read(dev, dir_start_offset, sizeof(fat_directory_entry_t), &dir);
		// ata_read(ata, dir_start_offset, sizeof(fat_directory_entry_t), (uint8_t*)&dir);
		dir_start_offset += sizeof(fat_directory_entry_t);


		// kxxd(&dir, sizeof(fat_directory_entry_t));

		if(dir.dir_name[0] == 0xe5){ continue; }  //empty entry

		if(dir.dir_name[0] == 0x00){	goto no_dir_entry_left;} //no entry beyond this point

		if(
			dir.dir_attr == FAT_DIR_ATTR_VOLUME_ID || dir.dir_attr == FAT_DIR_ATTR_HIDDEN    ||
			dir.dir_attr == FAT_DIR_ATTR_SYSTEM    || dir.dir_attr == FAT_DIR_ATTR_READ_ONLY ||
			dir.dir_attr == FAT_DIR_ATTR_LONGNAME
			){	continue;}

		if(i == index) break;
		i++;
	}
	
	dir_start_offset -= 2*sizeof(fat_directory_entry_t); //reverse back to check wheter it has long name
	
	int is_long_name;

	int longname_buffer_index = 0;
	char longname_buffer[256];
	memset(longname_buffer, 0, 256);
	char * phead = longname_buffer;

	fat_directory_entry_t mdir;
	// ata_read(ata, dir_start_offset, sizeof(fat_long_name_directory_entry_t), (uint8_t*)&mdir);
	// dev->read(dev, dir_start_offset, sizeof(fat_directory_entry_t), &mdir);
	read_fs(dev, dir_start_offset, sizeof(fat_directory_entry_t), &mdir);

	// dev->read(node, dir_start_offset, sizeof(fat_long_name_directory_entry_t), &mdir);


	if(mdir.dir_attr == FAT_DIR_ATTR_LONGNAME){
		
		is_long_name = 1;
		while(1){
			
			longname_buffer_index += fat_create_ascii_from_dir( mdir, &longname_buffer[longname_buffer_index]);
			if( mdir.dir_name[0] & 0x40 ) break;
			dir_start_offset -= sizeof(fat_directory_entry_t); //reverse back to check wheter it has long name
			// ata_read(ata, dir_start_offset, sizeof(fat_long_name_directory_entry_t), &mdir);
			// ata_read(node, dir_start_offset, sizeof(fat_long_name_directory_entry_t), (uint8_t*)&mdir);
			// dev->read(dev, dir_start_offset, sizeof(fat_directory_entry_t), &mdir);
			read_fs(dev, dir_start_offset, sizeof(fat_directory_entry_t), &mdir);

		}
	}


	// fb_console_printf("fat12_readdir. %s->%s\n", long_filename, dir.dir_name);
	struct dirent* out = kcalloc(1, sizeof(struct dirent));
	
	if(is_long_name){
		strcpy(out->name, longname_buffer);
		out->name[strlen(longname_buffer)] = '\0';
	}
	else{
		memcpy(out->name, dir.dir_name, 11);
		out->name[12] = '\0';
	}

	out->type = dir.dir_attr == FAT_DIR_ATTR_ARCHIVE ? FS_FILE : FS_DIRECTORY;
	node->offset++;
	return out;	


no_dir_entry_left:
	node->offset = 0;
    return NULL;

}


finddir_type_t fat32_finddir(struct fs_node* node, char *name){

	fat_struct_t* fat = node->device;
	fs_node_t** dev = fat->dev;
	

	fat_bootsector_32_t* bpb = fat->sector0;
	int data_start_sector = bpb->comm.BPB_RsvdSecCnt + ( bpb->comm.BPB_NumFATs * bpb->BPB_FATSz32);
	int dir_start_offset;
	dir_start_offset =  (data_start_sector + ((fat->cluster_index - 2)*bpb->comm.BPB_SecPerClus));
	dir_start_offset *= bpb->comm.BPB_BytsPerSec;

	
	
	list_t longname_list = list_create();
	while(1){ //search for entire directory to find folder/file
		fat_directory_entry_t dir;
		// ata_read(ata, dir_start_offset, sizeof(fat_directory_entry_t), (uint8_t*)&dir);
		read_fs(dev, dir_start_offset, sizeof(fat_directory_entry_t), (uint8_t*)&dir);
		dir_start_offset += sizeof(fat_directory_entry_t);

		if(dir.dir_name[0] == 0xe5){ continue;}  //empty entry
		if(dir.dir_name[0] == 0x00){	break;} //no entry beyond this point


		//we don't care about entries other than directory,archive and longnames
		if(
			dir.dir_attr == FAT_DIR_ATTR_VOLUME_ID ||
			dir.dir_attr == FAT_DIR_ATTR_HIDDEN    ||
			dir.dir_attr == FAT_DIR_ATTR_SYSTEM    || 
			dir.dir_attr == FAT_DIR_ATTR_READ_ONLY 
			){	continue; 	}


		
		if(dir.dir_attr == FAT_DIR_ATTR_LONGNAME ){
			/*
			we save longname entries, because very next entry after longnames is directory/file
			we need longnames since we interface with filesystem through longnames.after the last longname
			, found by either non_longname entry or dir_order = 0x1, we compare it with the name string. if there's a match
			we return it to the caller 
			*/
			char* ascii_name = kmalloc(14);
			fat_create_ascii_from_dir(dir, ascii_name);
			list_insert_end(&longname_list, ascii_name);

		}

		if(dir.dir_attr == FAT_DIR_ATTR_DIRECTORY || dir.dir_attr == FAT_DIR_ATTR_ARCHIVE ){
			
			char* long_filename = NULL;
			for(listnode_t* node = list_pop_end(&longname_list);  node ; node = list_pop_end(&longname_list) ){
				
				if(!long_filename) long_filename=kcalloc(256, 1);
				char* name = node->val;
				strcat(long_filename, name);
				kfree(name);
				kfree(node);
			}


			if(long_filename){ //compare it by long names if file has it
				if(!strcmp(long_filename, name)){
					
					fs_node_t* fnode = kcalloc(1, sizeof(fs_node_t));

					fnode->inode = 0;
					strcpy(fnode->name, long_filename);
    				fnode->impl = 5; // ??
					fnode->uid = 0;
					fnode->gid = 0;

					//clone fat of parent node
		
					fat_struct_t* ffat = kcalloc(1, sizeof(fat_struct_t));
					memcpy(ffat, fat, sizeof(fat_struct_t));
    				fnode->device = ffat;
					ffat->root_node = fnode;
					// ffat->sector_offset = 0;
					// ffat->sector_offset =dir.dir_first_cluster_lo;
					fnode->length = dir.dir_file_size;
					ffat->cluster_index = ( dir.dir_first_cluster_hi << 16 )| dir.dir_first_cluster_lo;
					
					


					if(dir.dir_attr == FAT_DIR_ATTR_DIRECTORY ){
						
						fnode->flags   = FS_DIRECTORY;
						fnode->readdir = fat32_readdir;
						fnode->finddir = fat32_finddir;
						fnode->mkdir   = fat32_mkdir;

					}
					else{
						fnode->flags   = FS_FILE;
						fnode->read = fat32_read;
						fnode->write = fat32_write;
					}

					return fnode;
				}
			}

		}
	}
	
		
    return NULL;
}

read_type_t fat32_read(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer){
	

	if(offset + size >= node->length){
		return 0;
	}
	

	fat_struct_t* fat = node->device;
	fs_node_t* dev = fat->dev;


	fat_bootsector_32_t* bpb = fat->sector0;
	int data_start_sector = bpb->comm.BPB_RsvdSecCnt + ( bpb->comm.BPB_NumFATs * bpb->BPB_FATSz32);
	
	int fat_start_sector = bpb->comm.BPB_RsvdSecCnt;
	int fat_start_offset = fat_start_sector * bpb->comm.BPB_BytsPerSec; 



	uint32_t cluster_size = bpb->comm.BPB_SecPerClus * bpb->comm.BPB_BytsPerSec;
	int cluster_index = offset / cluster_size;
	int cluster_offset = offset % cluster_size;
	
	

	uint32_t current_cluster = fat->cluster_index;
	for(int i = 0; i < cluster_index; ++i){
		
		uint32_t fat_entry;
		uint32_t offset_within_fat = current_cluster*4;
		// ata_read(ata, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		read_fs(dev, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		
		if( (fat_entry & 0x0FFFFFFF) >= 0x0FFFFFF8) return 0; //EOC
		current_cluster = fat_entry & 0x0FFFFFFF;
	}


	size_t cluster_data_left = cluster_size - cluster_offset;
	
	
	if(size <= cluster_data_left){
		
		int dir_start_offset;
		dir_start_offset =  (data_start_sector + (( current_cluster - 2)*bpb->comm.BPB_SecPerClus));
		dir_start_offset *= bpb->comm.BPB_BytsPerSec;
		
		// ata_read(ata, dir_start_offset + cluster_offset, size, buffer);
		read_fs(dev, dir_start_offset + cluster_offset, size, buffer);
		node->offset += size;
		return size;
	}
	


	//two parts

	int dir_start_offset;
	dir_start_offset =  (data_start_sector + (( current_cluster - 2)*bpb->comm.BPB_SecPerClus));
	dir_start_offset *= bpb->comm.BPB_BytsPerSec;
		
	// ata_read(ata, dir_start_offset + cluster_offset, cluster_data_left, buffer);
	read_fs(dev, dir_start_offset + cluster_offset, cluster_data_left, buffer);
	buffer += cluster_data_left;

	//get next cluster
	{
		uint32_t fat_entry;
		uint32_t offset_within_fat = current_cluster*4;
		read_fs(dev, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		// ata_read(ata, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		
		current_cluster = fat_entry & 0x0FFFFFFF;

	}	
	
	
	dir_start_offset =  (data_start_sector + (( current_cluster - 2)*bpb->comm.BPB_SecPerClus));
	dir_start_offset *= bpb->comm.BPB_BytsPerSec;		
	// ata_read(ata, dir_start_offset , size - cluster_data_left , buffer);
	read_fs(dev, dir_start_offset , size - cluster_data_left , buffer);


	node->offset += size;
	return size;
}


write_type_t fat32_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){


	fat_struct_t* fat = node->device;
	fs_node_t* dev = fat->dev;
	


	fat_bootsector_32_t* bpb = fat->sector0;
	int data_start_sector = bpb->comm.BPB_RsvdSecCnt + ( bpb->comm.BPB_NumFATs * bpb->BPB_FATSz32);
	
	int fat_start_sector = bpb->comm.BPB_RsvdSecCnt;
	int fat_start_offset = fat_start_sector * bpb->comm.BPB_BytsPerSec; 



	uint32_t cluster_size = bpb->comm.BPB_SecPerClus * bpb->comm.BPB_BytsPerSec;
	int cluster_index = offset / cluster_size;
	int cluster_offset = offset % cluster_size;


	int cluster_in_question = fat->cluster_index;
	for(int i = 0 ; i < cluster_index; ++i){
		
		int fat_entry;
		// ata_read(ata, fat_start_offset + (cluster_in_question*4), 4, (uint8_t*)&fat_entry);
		read_fs(dev, fat_start_offset + (cluster_in_question*4), 4, (uint8_t*)&fat_entry);

		// we encountered eoc before getting to the indexed cluster
		// allocate a new empty cluster and append it
		if(fat_entry & 0xfffffff >= 0xFFFFFF8){ 
			fb_console_printf("TODO allocate new cluster to the list: %x\n", fat_entry);
			node->offset = 0;
			return 0;

		}
		else{ //continue on the list	
			cluster_in_question = fat_entry;
		}

		/*
		int32_t free_entry = -1;
		for(int i = 0; fat_start_offset + 4*i  < fat_end_offset; ++i){
		
		int32_t fat_entry ;
		uint32_t offset_within_fat = i*4;
		ata_read(ata, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		
		if( (fat_entry & 0x0FFFFFFF) == 0x0){
			free_entry = i;
			break;

		}
		*/	
	}

	//well write to that addr mf
	uint32_t file_cluster_start =  (data_start_sector + ((cluster_in_question - 2)*bpb->comm.BPB_SecPerClus));
	file_cluster_start *= bpb->comm.BPB_BytsPerSec;
	
	
	// ata_write(ata, file_cluster_start + cluster_offset, size, buffer);
	write_fs(dev, file_cluster_start + cluster_offset, size, buffer);
	node->offset += size;
	return size;	
}




mkdir_type_t fat32_mkdir(fs_node_t* node, const char* name, uint16_t permission){

	
	fat_struct_t* fat = node->device;
	device_t* dev = fat->dev;
	ata_device_t* ata = dev->priv;

	//select correct drive?
	int drive = 0xa | (ata->drive_index & 1);
	drive <<= 4;
	outb(ata->io_base + ATA_REG_DRIVE_SELECT, drive);
	
	fat_bootsector_32_t* fat32_sp = fat->sector0;
	// fat->cluster_index = fat32_sp->BPB_RootClus;

	int data_start_sector = fat32_sp->comm.BPB_RsvdSecCnt + ( fat32_sp->comm.BPB_NumFATs * fat32_sp->BPB_FATSz32);	
	int fat_start_sector = fat32_sp->comm.BPB_RsvdSecCnt;
	int fat_start_offset = fat_start_sector * fat32_sp->comm.BPB_BytsPerSec;
	int fat_end_offset = fat_start_offset + (fat32_sp->BPB_FATSz32*fat32_sp->comm.BPB_BytsPerSec);

	uint32_t cluster_size = fat32_sp->comm.BPB_SecPerClus * fat32_sp->comm.BPB_BytsPerSec;


	//calculate the amount of directory entries needed, both dos directory and long names
	int number_of_entries = (strlen(name) / 13) + (strlen(name) % 13 != 0); //entries needed for long dir entries
	number_of_entries += 1; //and one directory entry

	//find empty cluster for directories list;
	int32_t free_entry = -1;
	for(int i = 0; fat_start_offset + 4*i  < fat_end_offset; ++i){
		
		int32_t fat_entry ;
		uint32_t offset_within_fat = i*4;
		ata_read(ata, fat_start_offset + offset_within_fat, 4, (uint8_t*)&fat_entry);
		
		if( (fat_entry & 0x0FFFFFFF) == 0x0){
			free_entry = i;
			break;

		}
	}
	if(free_entry == -1){
		fb_console_printf("no avaliable clusters\n");
		return;
	}


	
	
	//allocate on node's directory table?
	int dir_start_offset;
	dir_start_offset =  (data_start_sector + ((fat->cluster_index - 2)*fat32_sp->comm.BPB_SecPerClus));
	dir_start_offset *= fat32_sp->comm.BPB_BytsPerSec;

	fat_directory_entry_t edir;
	int32_t empty_dir_index = -1;
	uint32_t i = 0;
	int previous_found = 0;
	
	while(i < (cluster_size / 32)){ //search for entire directory cluster for continous spot
	
		ata_read(ata, dir_start_offset + i*sizeof(fat_directory_entry_t), sizeof(fat_directory_entry_t), (uint8_t*)&edir);

		
		if(edir.dir_name[0] == 0xe5 || edir.dir_name[0] == 0x00){
			
			empty_dir_index = i;
			break;

			previous_found++;
			if(previous_found >= number_of_entries){
				
				empty_dir_index = i - previous_found;
				break;
			}
		}
		else{ //non empty directory
			previous_found = 0;
		}

		i++;
	}
	if(empty_dir_index == -1){
		fb_console_printf("no empty entries in parent dir\n");
		return;
	}


	

	//this one is wiht the assumption that there's 2 consecutive list enties and name length is smaller than 13 characters
	fat_long_name_directory_entry_t* ldir = &edir;
	ldir->long_dir_order = 0x41;
	ldir->long_dir_attr = FAT_DIR_ATTR_LONGNAME;
	ldir->long_dir_type = 0;
	ldir->long_dir_fst_clust_lo = 0;

	uint16_t ucs_str[13];
	fat_create_usc2_from_ascii_n(name, ucs_str, 13);

	memcpy( ldir->long_dir_name1, &ucs_str[0], 10);
	memcpy( ldir->long_dir_name2, &ucs_str[5], 12);
	memcpy( ldir->long_dir_name3, &ucs_str[11], 4);

	ata_write(ata, dir_start_offset + (empty_dir_index)*sizeof(fat_directory_entry_t), sizeof(fat_directory_entry_t), (uint8_t*)&edir);
	
	memset(&edir, 0, sizeof(fat_directory_entry_t));
	edir.dir_attr = FAT_DIR_ATTR_DIRECTORY;
	edir.dir_first_cluster_lo = free_entry & 0xffff;
	edir.dir_first_cluster_hi = free_entry >> 16;

	char sfn[11];
	memset(sfn, 0, 11);
	if(strlen(name) > 8){
		for(int i = 0; i < 6; ++i){
			sfn[i] = name[i] >= 'a' && name[i] <= 'z' ? name[i] - 32 : name[i];
		}
		sfn[6] = '~';
		sfn[7] = '1'; //?
	}
	else{
		for(int i = 0; i < 8; ++i){
			sfn[i] = name[i] >= 'a' && name[i] <= 'z' ? name[i] - 32 : name[i];
		}
	}


	memcpy(edir.dir_name, sfn, 11);

	ata_write(ata, dir_start_offset + (empty_dir_index + 1)*sizeof(fat_directory_entry_t), sizeof(fat_directory_entry_t), (uint8_t*)&edir);
	
	memset(&edir, 0, sizeof(fat_directory_entry_t));
	ata_write(ata, dir_start_offset + (empty_dir_index + 2)*sizeof(fat_directory_entry_t), sizeof(fat_directory_entry_t), (uint8_t*)&edir);



	//mark the cluster as allocated and end of chain
	uint32_t eoc_dat = 0x0FFFFFF8;
	ata_write(ata, fat_start_offset + free_entry*4, 4, &eoc_dat);

	dir_start_offset =  (data_start_sector + ((free_entry - 2)*fat32_sp->comm.BPB_SecPerClus));
	dir_start_offset *= fat32_sp->comm.BPB_BytsPerSec;

	uint8_t* temp =  kmalloc(512);
	memset(temp, 0, 512);
	ata_write(ata, dir_start_offset , 512, temp);
	kfree(temp);


	return;

}

