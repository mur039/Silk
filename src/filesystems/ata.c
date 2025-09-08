#include <filesystems/ata.h>
#include <fb.h>


int ata_find_devices(){
    
    // look for primary and secondary busses
    int drives_found = 0;
    uint8_t status_byte;
    uint16_t iobase[2] = { ATA_PRIMARY_IO_BASE, ATA_SECONDARY_IO_BASE};
    uint16_t ctrlbase[2] = { ATA_PRIMARY_CTRL_BASE, ATA_SECONDARY_CTRL_BASE};

    uint8_t* sector_buf = kmalloc(512); // a sector darling

    for(int bus = 0; bus < 2 ; ++bus){

        ata_soft_reset_bus(bus); //reset devices on the bus
        ata_wait_ready(iobase[bus]);

        for(int drive = 0; drive < 2; ++drive){

            int drive_sel = 0xa | (drive & 1);
            drive_sel <<= 4;

            outb( iobase[bus] + ATA_REG_DRIVE_SELECT, drive_sel);
            ata_400ns_delay(iobase[bus]);

            
            outb(iobase[bus] + ATA_REG_SECCOUNT0, 0);
            outb(iobase[bus] + ATA_REG_LBA0, 0);
            outb(iobase[bus] + ATA_REG_LBA1, 0);
            outb(iobase[bus] + ATA_REG_LBA2, 0);
            outb(iobase[bus] + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_DEVICE);

        
            status_byte = inb( iobase[bus] + ATA_REG_STATUS);
            
            //if so floating bus, nothing is connected
            if (status_byte == 0x0){
                fb_console_printf("Not connected\n");
                continue;
            }

            ata_wait_ready(iobase[bus]);
            ata_400ns_delay(iobase[bus]);

            int sector_count = 0;
            const uint8_t* serial_number = "0123456789";
            const char* model_name = "Generic Harddisk";
 
            
            int err;
            if ((inb(iobase[bus] + ATA_REG_STATUS) & ATA_STATUS_ERR)) {

                
                //probably ATAPI or SATA try the other command
                int cl, ch;
                cl = inb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA1);
                ch = inb(ATA_PRIMARY_IO_BASE + ATA_REG_LBA2);

                if(cl == 0x14 && ch == 0xeb){ //atapi device
                    fb_console_printf("ATAPI device\n");
                    err = atapi_send_identify(iobase[bus], ctrlbase[bus], drive, sector_buf);
                    if(err != -1){
                        uint16_t identifier = *(uint16_t*)sector_buf;
                        int not_ata = (identifier >> 15) & 1;
                        int device_type = (identifier >> 8) & 0x1F;

                        if(not_ata){
                            fb_console_printf("Not a ATA device that is:");
                            switch(device_type){
                                case ATAPI_DEVICE_HDD:
                                    fb_console_printf("a hdd\n");
                                break;

                                case ATAPI_DEVICE_TAPE:
                                    fb_console_printf("a tape\n");
                                break;

                                case ATAPI_DEVICE_CDROM:
                                    fb_console_printf("a cdrom\n");
                                    //probably register cdrom or something
                                break;

                                default:
                                    fb_console_printf("Unknown device type %x\n", device_type);
                                break;
                            }
                        }
    
                    }
                }
                
    

                
            }
            else{

                //simple ata device
                fb_console_printf("ATA device\n");
                err = ata_send_identify(iobase[bus], ctrlbase[bus], drive, sector_buf);
                if(err != -1){

                    int sector_count = *(uint32_t*)&sector_buf[60];
                    const uint8_t* serial_number = &sector_buf[10];
                    const char* model_name = &sector_buf[27];

                    ata_register_device(bus, drive, sector_count, serial_number, model_name);

                }
            }

        }
    }

    kfree(sector_buf);
    return drives_found;

}


void ata_register_device(int bus, int drive, int sector_count, const uint8_t serial_number[20], const char modelname[40]){

    //sanitiz them john
    bus &= 1;
    drive &= 1;

    int drive_index = (bus << 1) | drive;
    device_t* hd = kcalloc(1, sizeof(device_t));
    hd->name = strdup("hdxxx");
    sprintf(hd->name, "hd%c", 'a' + drive_index ); // 00 a 01 b 10 c 11 d
    hd->ops.get_size = (get_size_type_t)ata_getsize_fs;
    hd->ops.write = (write_type_t)ata_write_fs;
    hd->ops.read = ata_read_fs;
    hd->dev_type = DEVICE_BLOCK;
    hd->unique_id = 3;
    
    ata_device_t* priv = kcalloc(sizeof(ata_device_t), 1);
    hd->priv = priv;

    priv->io_base = bus ? ATA_SECONDARY_IO_BASE : ATA_PRIMARY_IO_BASE;
    priv->ctrl_base = bus ? ATA_SECONDARY_CTRL_BASE :ATA_PRIMARY_CTRL_BASE;
    priv->drive_index = drive_index;
    priv->start_sector = 0;
    priv->end_sector = sector_count;
    
    
    if(serial_number)
        memcpy(priv->serial_number, serial_number ,20); //serial number

    if(modelname)
        memcpy(priv->model_name, modelname ,40); //model name                

    dev_register(hd);

    uint8_t* sector_buf = kmalloc(512);
    ata_read_sector(bus, drive, 0, sector_buf);
                
    if(ata_is_mbr(sector_buf)){ //if mbr exists

        //parse primary partitions

        for(int i = 0; i < 4 ; ++i){
        
            //first entry : 0x1be
            mbr_partition_entry_t entry;
            memcpy(&entry, &sector_buf[0x1be + 16*i], 16);
            
            if(!entry.number_of_sectors)  //empty entry
                continue; 

            device_t* hda_part = kcalloc(1, sizeof(device_t));
            ata_device_t* hda_part_priv = kmalloc(sizeof(ata_device_t));

            hda_part->name = strdup("hdap000");
            sprintf(hda_part->name, "hd%cp%u", 'a' + drive_index,i + 1);
            hda_part->ops.write = (write_type_t)ata_write_fs;
            hda_part->ops.read = &ata_read_fs;
            hda_part->dev_type = DEVICE_BLOCK;
            hda_part->unique_id = 3;
            hda_part->ops.get_size = (get_size_type_t)ata_getsize_fs;

            hda_part->priv = hda_part_priv;
            *hda_part_priv = *priv;
            hda_part_priv->start_sector = entry.LBA_start_partition;
            hda_part_priv->end_sector = entry.LBA_start_partition + entry.number_of_sectors;
            dev_register(hda_part);
        }
    }

    kfree(sector_buf);

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
int ata_read_sector(uint16_t bus, uint16_t drive, uint32_t lba, uint8_t *buffer) {

    uint16_t io_base   = ((bus &  1) == 0) ?   ATA_PRIMARY_IO_BASE   : ATA_SECONDARY_IO_BASE;
    uint16_t ctrl_base = ((bus &  1) == 0) ?   ATA_PRIMARY_CTRL_BASE : ATA_SECONDARY_CTRL_BASE;

    ata_wait_ready(io_base);

    uint8_t drive_sel = 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F);
    
    outb(io_base + ATA_REG_DRIVE_SELECT, drive_sel ); // Select drive and LBA
    ata_wait_ready(io_base);

    outb(io_base + ATA_REG_SECCOUNT0, 1); // Number of sectors
    outb(io_base + ATA_REG_LBA0, (uint8_t)lba);
    outb(io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

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


// Write a sector to the disk
int atapi_send_identify(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t *buffer) {
    
    ata_wait_ready(io_base);

    drive = 0xa | (drive & 1);
    drive <<= 4; 
    outb(io_base + ATA_REG_DRIVE_SELECT, drive ); // Select drive and LBA
    outb(io_base + ATA_REG_SECCOUNT0, 1);
    outb(io_base + ATA_REG_LBA0, 0);
    outb(io_base + ATA_REG_LBA1, 0);
    outb(io_base + ATA_REG_LBA2, 0);
    
    outb(io_base + ATA_REG_COMMAND, ATAPI_IDENTIFY_PACKET_DEVICE);

    if ((inb(io_base + ATA_REG_STATUS) & ATA_STATUS_ERR)) {
        
        fb_console_printf("command aborted device doesn't support IDENTIFY command\n");
    }
    ata_wait_ready(io_base);

    int err = 0;
    int max_tries = 16;
    while(max_tries--){

        if (!(inb(io_base + ATA_REG_STATUS) & ATA_STATUS_DRQ)) {
            fb_console_printf("Error: Disk not ready for data.\n");
            err = -1;
            continue;
        }
        err = 0;
    }

    if(err == -1){
        fb_console_printf("Exhausted max tries\n");
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
    //for test with hardware i do this
    return 0;
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


uint32_t ata_write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    device_t* dev = node->device;
    ata_device_t* ata = dev->priv;
    
    return  (uint32_t)ata_write(ata, offset, size, buffer);
}

uint32_t ata_read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    
    device_t* dev = node->device;
    ata_device_t* ata = dev->priv;
    return (uint32_t)ata_read(ata, offset, size, buffer);
}

int ata_getsize_fs(fs_node_t* node){
    device_t* dev = node->device;
    ata_device_t* ata = dev->priv;
    return (ata->end_sector - ata->start_sector)*ATA_SECTOR_SIZE;
}

//hot steaming shit
int ata_read(ata_device_t* ata, uint32_t offset, uint32_t size, uint8_t* buffer){
        
    uint32_t total_size = (ata->end_sector - ata->start_sector) * ATA_SECTOR_SIZE;
    if(offset >= total_size){
        return 0;
    }

    //trim it down
    uint32_t remaining = total_size - offset;
    if(size > remaining){
        size = remaining;
    }

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

		if(last_piece){

            ata_read(ata, offset, last_piece, buffer);
        }
			
		return size;
    }

    size_t sector_start = (offset - (offset % 512)) / 512;
    size_t sector_offset = offset % 512;

    sector_start += ata->start_sector;

    if(sector_offset + size > ATA_SECTOR_SIZE){
        
        size_t rem_1 = 512 - sector_offset;
        size_t rem_2 = (sector_offset + size) % 512;
        
        ata_read(ata, offset, rem_1, buffer);
        buffer += rem_1;
        offset += rem_1;
        size -= rem_1;

        sector_start = (offset - (offset % 512)) / 512;
        sector_offset = offset % 512;

    }

    int bus = (ata->drive_index >> 1) & 1;
    int drive = (ata->drive_index ) & 1;
    //check wheter there's cached sector and that sector is sector we looking for
    if( !vfs_sector_cache.valid || vfs_sector_cache.cached_lba != sector_start ){ //checks whethet there's cached sector, then check if it is our sector
        if(ata_read_sector(bus, drive, sector_start, vfs_sector_cache.data) != 0){

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
    
    /*
     * On the test on real hardware i will disable ata_write to protect the disks attached to the computer
     *
     * */

    return size;

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
    
    int bus = (ata->drive_index >> 1) & 1;
    int drive = (ata->drive_index) & 1;

    // fb_console_printf("ata_write-> sector_start:%x sector_offset:%x size:%u\n", sector_start, sector_offset, size);
    uint8_t * sector_buffer = kmalloc(512);
    
    ata_read_sector(bus, drive, sector_start, sector_buffer);
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



void initialize_ata_cache(struct ata_cache* cache_struct, int max_cache_count){
    
    cache_struct->current_cache_count = 0;
    cache_struct->max_cache_count = max_cache_count;
    cache_struct->head = NULL;
    cache_struct->raw_buffer = kmalloc(max_cache_count * ATA_SECTOR_SIZE);

}



