#include <filesystems/ata.h>
#include <fb.h>





int ata_find_devices(){
    
    // look for primary and secondary busses
    uint8_t status_byte;

    ata_soft_reset_bus(0);//reset devices on primary bus
    ata_wait_ready(ATA_PRIMARY_IO_BASE);
    for(int i = 0; i < 2 ; ++i){
        
        int drive = 0xa | (i & 1);
        drive <<= 4; 
        outb(ATA_PRIMARY_IO_BASE + ATA_REG_DRIVE_SELECT, drive );
        ata_400ns_delay(ATA_PRIMARY_IO_BASE);
        status_byte = inb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
        if(status_byte == 0xFF){
            fb_console_printf("at bus 1, drive %u doesn't exist\n", i);
            continue;
        }

        //drive is connected so
        int cl, ch;
        cl = inb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA1);
        ch = inb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA2);
        // fb_console_printf("After reset signature bytes: cl:%x ch:%x\n", cl, ch);
        
        if(cl == 0xff && ch == 0xff) continue; //floating thus empty bus
        if(cl == 0 && ch == 0){ //simple ata device so

            uint8_t* sector_buf = kmalloc(512); // a sector darling
            int result = ata_send_identify(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, i, sector_buf);
            if(result != -1){
                
                device_t* hda = kcalloc(1, sizeof(device_t));
                hda->name = strdup("hdx");
                hda->name[2] = '0' + i;
                hda->write = ata_write_fs;
                hda->read = ata_read_fs;
                hda->dev_type = DEVICE_BLOCK;
                hda->unique_id = 3;
                
                ata_device_t* priv = kmalloc(sizeof(ata_device_t));
                hda->priv = priv;

                uint16_t* head = sector_buf;
                priv->io_base = ATA_PRIMARY_IO_BASE;
                priv->ctrl_base = ATA_PRIMARY_CTRL_BASE;
                priv->drive_index = i;
                priv->start_sector = 0;
                memcpy(&priv->end_sector, &head[60] ,4); //sector count
                memcpy(&priv->serial_number, &head[10] ,20); //serial number
                memcpy(&priv->model_name, &head[27] ,40); //model name                
                dev_register(hda);

                fb_console_printf("device on bus 1 drive %u has %xh sectors\n", i, priv->end_sector);
                //while there look for partitions and register them as well
                ata_read_sector(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, 0, sector_buf);
                if(ata_is_mbr(sector_buf)){ //if mbr exists

                    //parse primary partitions

                    for(int i = 0; i < 4 ; ++i){
                    
                    //first entry : 0x1be
                    mbr_partition_entry_t entry;
                    memcpy(&entry, &sector_buf[0x1be + 16*i], 16);
                    if(!entry.number_of_sectors) continue; //empty entry
                    
                    device_t* hda_part = kcalloc(1, sizeof(device_t));
                    ata_device_t* hda_part_priv = kmalloc(sizeof(ata_device_t));

                    hda_part->name = strdup("hd0p000");
                    sprintf(hda_part->name, "hd0p%u", i + 1);
                    hda_part->write = ata_write_fs;
                    hda_part->read = ata_read_fs;
                    hda_part->dev_type = DEVICE_BLOCK;
                    hda_part->unique_id = 3;

                    hda_part->priv = hda_part_priv;
                    *hda_part_priv = *priv;
                    hda_part_priv->start_sector = entry.LBA_start_partition;
                    hda_part_priv->end_sector = entry.LBA_start_partition + entry.number_of_sectors;
                    dev_register(hda_part);
                    }
                }
            }
            
            else{ //well another command to i



            }

            kfree(sector_buf);
        }
        if(cl == 0x14 && ch == 0xeb){ //atapi device

        }



    }


    ata_soft_reset_bus(1);//reset devices on secondary bus
    ata_wait_ready(ATA_SECONDARY_IO_BASE);
    for(int i = 0; i < 2 ; ++i){
        
        int drive = 0xa | (i & 1);
        drive <<= 4; 
        outb(ATA_SECONDARY_IO_BASE + ATA_REG_DRIVE_SELECT, drive );
        ata_400ns_delay(ATA_SECONDARY_IO_BASE);
        status_byte = inb(ATA_SECONDARY_IO_BASE + ATA_REG_STATUS);
        if(status_byte == 0xFF){
            fb_console_printf("at bus 2, drive %u doesn't exist\n", i);
            continue;
        }

        //drive is connected so
        int cl, ch;
        cl = inb(ATA_SECONDARY_IO_BASE + ATA_REG_LBA1);
        ch = inb(ATA_SECONDARY_IO_BASE + ATA_REG_LBA2);
        
        //floating bus so
        if(cl == 0xff && ch == 0xff) continue;

        
    }


    return;
#if 0
    uint8_t * block = kmalloc(512);
    memset(block, 0, 512);
    if(pri_status_byte != 0xff){ //Primary bus exist

        fb_console_printf("found the primary bus\n");
        ata_initialize(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE);
        ata_400ns_delay(ATA_PRIMARY_IO_BASE);

        uint32_t max_sector_count;
        if(ata_send_identify(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, 0, block) != -1){
            
            //well register the device than darling
            uint16_t *p_head = block;
            max_sector_count =  p_head[61] << 16 | p_head[60] ; 

            fb_console_printf("->master drive:maximum block size: %x\n", max_sector_count );

            device_t* hda = kcalloc(1, sizeof(device_t));
            ata_device_t* hda_priv = kmalloc(sizeof(ata_device_t));

            hda->name = strdup("hd0");
            hda->write = ata_write_fs;
            hda->read = ata_read_fs;
            hda->dev_type = DEVICE_BLOCK;
            hda->unique_id = 3;
    
            hda->priv = hda_priv;
            hda_priv->io_base = ATA_PRIMARY_IO_BASE;
            hda_priv->ctrl_base = ATA_PRIMARY_CTRL_BASE;
            hda_priv->drive_index = 0;
            hda_priv->start_sector = 0;
            hda_priv->end_sector = max_sector_count;
            dev_register(hda);

            //probe for mbr and create partition devices
            ata_read_sector(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, 0, block);
            if(!ata_is_mbr(block)){ /*fb_console_printf("mbr table doesn't exist\n")*/}
            else{
                // fb_console_printf("mbr table does exist\n");
                for(int i = 0; i < 4 ; ++i){
                    
                    //first entry : 0x1be
                    mbr_partition_entry_t entry;
                    memcpy(&entry, &block[0x1be + 16*i], 16);
                    if(!entry.number_of_sectors) continue; //empty entry
                    
                    // kxxd(&entry, 16);

                    device_t* hda_part = kcalloc(1, sizeof(device_t));
                    ata_device_t* hda_part_priv = kmalloc(sizeof(ata_device_t));

                    hda_part->name = strdup("hd0p000");
                    sprintf(hda_part->name, "hd0p%u", i + 1);
                    hda_part->write = ata_write_fs;
                    hda_part->read = ata_read_fs;
                    hda_part->dev_type = DEVICE_BLOCK;
                    hda_part->unique_id = 3;

                    hda_part->priv = hda_part_priv;
                    hda_part_priv->io_base = ATA_PRIMARY_IO_BASE;
                    hda_part_priv->ctrl_base = ATA_PRIMARY_CTRL_BASE;
                    hda_priv->drive_index = 0;
                    hda_part_priv->start_sector = entry.LBA_start_partition;
                    hda_part_priv->end_sector = entry.LBA_start_partition + entry.number_of_sectors;
                    dev_register(hda_part);

                }
            }

        }
        else{ //packet device perhaps
            fb_console_printf("found a packet device\n");
        }

        memset(block, 0, 512);
        ata_400ns_delay(ATA_PRIMARY_IO_BASE);
        if(ata_send_identify(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, 1, block) != -1){
            // kxxd(block, 512);
            uint32_t* word_pntr = block;
            uint32_t max_sector_count =  word_pntr[60]; 
            fb_console_printf("->slave drive:maximum block size: %x\n", max_sector_count );

        }
        else{ //packet device perhaps
            fb_console_printf("found a packet device\n");

        }

    }

    if(sec_status_byte != 0xff){ //there's something on the bus now send identify command

        fb_console_printf("found the secondary bus\n");
        ata_initialize(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE);
        ata_400ns_delay(ATA_SECONDARY_IO_BASE);
        if(ata_send_identify(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, 0, block) != -1){
            // kxxd(block, 512);
            uint32_t* word_pntr = block;
            uint32_t max_sector_count =  word_pntr[60]; 
            fb_console_printf("master drive:maximum block size: %x\n", max_sector_count );

        }
         else{ //packet device perhaps
            fb_console_printf("found a packet device\n");
        }

        ata_400ns_delay(ATA_SECONDARY_IO_BASE);
        if(ata_send_identify(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, 1, block) != -1){
            // kxxd(block, 512);
            uint32_t* word_pntr = block;
            uint32_t max_sector_count =  word_pntr[60]; 
            fb_console_printf("slave drive:maximum block size: %x\n", max_sector_count );

        }
         else{ //packet device perhaps
            fb_console_printf("found a packet device\n");
        }




    }

    kfree(block);
    return 0;
#endif
}


void ata_io_wait(int ctrl_base){
    inb(ctrl_base);
    inb(ctrl_base);
    inb(ctrl_base);
    inb(ctrl_base);
}

void ata_400ns_delay(uint16_t io)
{
	for(int i = 0;i < 4; i++)
		inb(io + ATA_ALTERNATE_STATUS_REGISTER);
}


// Initialize ATA PIO mode
void ata_initialize(uint16_t io_base, uint16_t ctrl_base) {
    
    outb(ctrl_base, 0x00); // Disable interrupts
    ata_io_wait(ctrl_base);
    // fb_console_printf("ATA initialized.\n");
}




void ata_master_irq_handler(struct regs* r){

    fb_console_printf("ata_master_irq: Fires\n");
    return;
}


int ata_wait_ready(uint16_t io_base) {
    while (inb(io_base + ATA_REG_STATUS) & ATA_STATUS_BSY);
    return 0;
}

// Read a sector from the disk
int ata_read_sector(uint16_t io_base, uint16_t ctrl_base, uint32_t lba, uint8_t *buffer) {
    ata_wait_ready(io_base);

    outb(io_base + ATA_REG_DRIVE_SELECT,0xE0 | ((lba >> 24) & 0x0F)); // Select drive and LBA
    outb(io_base + ATA_REG_SECCOUNT0, 1); // Number of sectors
    outb(io_base + ATA_REG_LBA0, (uint8_t)lba);

    outb(io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

    ata_wait_ready(io_base);
    if (!(inb(io_base + ATA_REG_STATUS) & ATA_STATUS_DRQ)) {
        // fb_console_printf("Error: Disk not ready for data.\n");
        return -1;
    }

    for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
        ((uint16_t *)buffer)[i] = inw(io_base + ATA_REG_DATA);
    }

    ata_io_wait(ctrl_base);
    return 0;
}

// Write a sector to the disk
int ata_send_identify(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t *buffer) {
    
    ata_wait_ready(io_base);

    drive = 0xa | (drive & 1);
    drive <<= 4; 
    outb(io_base + ATA_REG_DRIVE_SELECT, drive ); // Select drive and LBA
    outb(io_base + ATA_REG_SECCOUNT0, 1);
    outb(io_base + ATA_REG_LBA0, 0);
    outb(io_base + ATA_REG_LBA1, 0);
    outb(io_base + ATA_REG_LBA2, 0);
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_DEVICE);

    if ((inb(io_base + ATA_REG_STATUS) & ATA_STATUS_ERR)) {
        
        fb_console_printf("command aborted device doesn't support IDENTIFY command\n");
        //probably ATAPI or SATA try the other command
        return -1;
    }
    ata_wait_ready(io_base);


    if (!(inb(io_base + ATA_REG_STATUS) & ATA_STATUS_DRQ)) {
        fb_console_printf("Error: Disk not ready for data.\n");
        return -1;
    }


    for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
        ((uint16_t *)buffer)[i] = inw(io_base + ATA_REG_DATA);

    }

    ata_io_wait(ctrl_base);
    return 0;
}


int ata_send_command(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t command, uint8_t* buffer){
    
    ata_wait_ready(io_base);
    drive = 0xa | (drive & 1);
    drive <<= 4; 
    outb(io_base + ATA_REG_DRIVE_SELECT, drive ); // Select drive and LBA
    outb(io_base + ATA_REG_SECCOUNT0, 0);

    return 0;

}

enum ata_device_type ata_determine_dev_type(int bus, int drive);
void ata_soft_reset_bus(int bus){
    bus &= 1;
    int ctrl_port;
    
    if(!bus) ctrl_port = ATA_PRIMARY_CTRL_BASE;
    else ctrl_port = ATA_SECONDARY_CTRL_BASE;

    outb(ctrl_port + ATA_DEVICE_CTRL_REGISTER , 1 << 2); //reset the fella
    
    //some how wait for 5us
    for(int i = 0;i < 50; i++) inb( 0 );


}



ata_sector_cache_t vfs_sector_cache =  {.cached_lba = 0, .valid = 0};


// Write a sector to the disk
int ata_write_sector(uint32_t io_base, uint32_t ctrl_base, uint32_t lba, uint8_t *buffer) {
    // fb_console_printf("ata_write_sector: io_base:%x ctrl_base:%x\n", io_base, ctrl_base);

    ata_wait_ready(io_base);

    outb(io_base + ATA_REG_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0xf) ); // Select drive and LBA
    outb(io_base + ATA_REG_SECCOUNT0, 1); // Number of sectors
    outb(io_base + ATA_REG_LBA0, (uint8_t)lba & 0xff);
    outb(io_base + ATA_REG_LBA1, (uint8_t) ((lba >> 8) & 0xff) );
    outb(io_base + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xff));
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);


    while(!(inb(io_base + ATA_REG_STATUS) & ATA_STATUS_DRQ));


    uint16_t *word_ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        // uart_print(COM1, "In inner loop %u\r\n", i);
        outw(io_base + ATA_REG_DATA, word_ptr[i]);
    }

    ata_wait_ready(io_base);

    uint8_t status = inb(io_base + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) {
        fb_console_printf("ata_write_sector: Write error (Status: 0x%x)\n", status);
        return -1;
    }

    outb(io_base + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
    ata_wait_ready(io_base);

    fb_console_printf("ata_write_sector: Write successful to LBA %u\n", lba);

    //invalidate the sector cache
    vfs_sector_cache.valid = 0;
    return 0;
}


write_type_t ata_write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    device_t* dev = node->device;
    ata_device_t* ata = dev->priv;
    
    return  ata_write(ata, offset, size, buffer);
}

read_type_t  ata_read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    
    device_t* dev = node->device;
    ata_device_t* ata = dev->priv;
    return ata_read(ata, offset, size, buffer);
}



int ata_read(ata_device_t* ata, uint32_t offset, uint32_t size, uint8_t* buffer){
        
    if(size  > 512){
        //chop it off into sectors 
		size_t amount_of_sectors = size / 512;
		size_t last_piece = size % 512;

		//do the sector reads
		for(size_t i = 0; i < amount_of_sectors; ++i){
            ata_read(ata, offset, 512, buffer);
			offset += 512;
			buffer += 512;

		}

		if(last_piece)
            ata_read(ata, offset, last_piece, buffer);
			
		return size;
    }

    int sector_start = (offset - (offset % 512)) / 512;
    int sector_offset = offset % 512;

    sector_start += ata->start_sector;

    if(sector_offset + size > ATA_SECTOR_SIZE){
        //read that spans multiple sectors just don't do it for now
        fb_console_printf("read spans multiple sectors we don't do that here do we?\n");
        return 0;
    }

    //check wheter there's cached sector and that sector is sector we looking for
    if( !vfs_sector_cache.valid || vfs_sector_cache.cached_lba != sector_start ){ //checks whethet there's cached sector, then check if it is our sector
        if(ata_read_sector(ata->io_base, ata->ctrl_base, sector_start, vfs_sector_cache.data) != 0){

            fb_console_printf("failed to read sector %u, offset:%u size:%u\n", sector_start, offset, size);
            return 0;
        }
        vfs_sector_cache.valid = 1;
        vfs_sector_cache.cached_lba = sector_start;
    }

    memcpy(buffer, vfs_sector_cache.data + sector_offset, size );
    return size;
}






ata_sector_cache_t vfs_sector_write_cache =  {.cached_lba = 0, .valid = 0};


int ata_write(ata_device_t* ata, uint32_t offset, uint32_t size, uint8_t* buffer){
    
    //special op
    //if buffer is null, we should flush dirty cache to the disk

    // if(!buffer){
        
    //     return 0;
    //     ata_write_sector(ata->io_base, ata->ctrl_base, vfs_sector_write_cache.cached_lba, vfs_sector_write_cache.data); //write back
    //     vfs_sector_write_cache.valid = 0;
    //     vfs_sector_write_cache.cached_lba = 0;
    //     return 0;

    // }

    if(size  > 512){
        fb_console_printf("can't write more than a sector");
        return 0;
    }

    int sector_start  = offset  / 512;
    int sector_offset = offset % 512;

    sector_start += ata->start_sector;

    if(sector_offset + size > ATA_SECTOR_SIZE){
        fb_console_printf("write spans multiple sectors we don't do that here do we?\n");
        return 0;
    }


    // fb_console_printf("ata_write-> sector_start:%x sector_offset:%x size:%u\n", sector_start, sector_offset, size);
    uint8_t * sector_buffer = kmalloc(512);

    ata_read_sector(ata->io_base, ata->ctrl_base, sector_start, sector_buffer);
    memcpy(&sector_buffer[sector_offset], buffer, size);

    ata_write_sector(ata->io_base, ata->ctrl_base, sector_start, sector_buffer);
    
    kfree(sector_buffer);
    return size;

}


int ata_is_mbr(uint8_t* sector0){
    if( !(sector0[510] == 0x55 && sector0[511] == 0xAA) ) return 0;

    for (int i = 446; i < 510; i++) {
        if (sector0[i] != 0) {
            return 1; // MBR exists
        }

    }
    return 0;
}







