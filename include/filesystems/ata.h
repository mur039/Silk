#ifndef __ATA_H__
#define __ATA_H__

#include <sys.h>
#include <stdint.h>
#include <filesystems/vfs.h>
#include <dev.h>

/*


IDENTIFY command

To use the IDENTIFY command, select a target drive by sending 0xA0 for the master drive, 
or 0xB0 for the slave, to the "drive select" IO port. On the Primary bus, 
this would be port 0x1F6. Then set the Sectorcount, LBAlo, LBAmid, and LBAhi IO ports 
to 0 (port 0x1F2 to 0x1F5). Then send the IDENTIFY command (0xEC) to the 
Command IO port (0x1F7). Then read the Status port (0x1F7) again. If the value read is 0, 
the drive does not exist. For any other value: poll the Status port (0x1F7) until bit 7 (BSY, value = 0x80) clears. Because of some ATAPI drives that do not follow spec, at this point you need to check the LBAmid and LBAhi ports (0x1F4 and 0x1F5) to see if they are non-zero. If so, the drive is not ATA, and you should stop polling. Otherwise, continue polling one of the Status ports until bit 3 (DRQ, value = 8) sets, or until bit 0 (ERR, value = 1) sets.
At that point, if ERR is clear, the data is ready to read from the Data port (0x1F0). Read 256 16-bit values, and store them.



"Command Aborted"

ATAPI or SATA devices are supposed to respond to an ATA IDENTIFY command by immediately reporting an error in the Status Register, rather than setting BSY, then DRQ, then sending 256 16 bit values of PIO data. These devices will also write specific values to the IO ports, that can be read. Seeing ATAPI specific values on those ports after an IDENTIFY is definitive proof that the device is ATAPI -- on the Primary bus, IO port 0x1F4 will read as 0x14, and IO port 0x1F5 will read as 0xEB. If a normal ATA drive should ever happen to abort an IDENTIFY command, the values in those two ports will be 0. A SATA device will report 0x3c, and 0xc3 instead. See below for a code example.
However, at least a few real ATAPI drives do not set the ERR flag after aborting an ATA IDENTIFY command. So do not depend completely on the ERR flag after an IDENTIFY.


Interesting information returned by IDENTIFY

    uint16_t 0: is useful if the device is not a hard disk.
    uint16_t 83: Bit 10 is set if the drive supports LBA48 mode.
    uint16_t 88: The bits in the low byte tell you the supported UDMA modes, the upper byte tells you which UDMA mode is active. If the active mode is not the highest supported mode, you may want to figure out why. Notes: The returned uint16_t should look like this in binary: 0000001 00000001. Each bit corresponds to a single mode. E.g. if the decimal number is 257, that means that only UDMA mode 1 is supported and running (the binary number above) if the binary number is 515, the binary looks like this, 00000010 00000011, that means that UDMA modes 1 and 2 are supported, and 2 is running. This is true for every mode. If it does not look like that, e.g 00000001 00000011, as stated above, you may want to find out why. The formula for finding out the decimal number is 257 * 2 ^ position + 2 ^position - 1.
    uint16_t 93 from a master drive on the bus: Bit 11 is supposed to be set if the drive detects an 80 conductor cable. Notes: if the bit is set then 80 conductor cable is present and UDMA modes > 2 can be used; if bit is clear then there may or may not be an 80 conductor cable and UDMA modes > 2 shouldn't be used but might work fine. Because this bit is "master device only", if there is a slave device and no master there is no way information about cable type (and would have to assume UDMA modes > 2 can't be used).
    uint16_t 60 & 61 taken as a uint32_t contain the total number of 28 bit LBA addressable sectors on the drive. (If non-zero, the drive supports LBA28.)
    uint16_t 100 through 103 taken as a uint64_t contain the total number of 48 bit addressable sectors on the drive. (Probably also proof that LBA48 is supported.)


Registers

An ATA bus typically has ten I/O ports which control its behavior. For the primary bus, these I/O ports are usually 0x1F0 (the "I/O" port base) through 0x1F7 and 0x3F6 (the "Control" port base) through 0x3F7. For the secondary bus, they are usually 0x170 through 0x177 and 0x376 through 0x377. Some systems may have non-standard port locations for the ATA busses, in which case it may be helpful to consult the section on PCI to determine how to retrieve port addresses for various devices in the system. 



ATAPI
For ATAPI:

    Bit 15 = 1 → Indicates device is not ATA (i.e., it’s ATAPI)

    Bits 8–7 = Device type:

        00 = Direct-access (e.g., HDD)

        01 = Tape

        05 = CD-ROM (most ATAPI devices)

    Bits 6–5 = Reserved

    Bits 4–0 = Packet interface standard (usually 0 or 1)
*/

//register offsets

//##offset from io base

// ATA Registers
#define ATA_REG_DATA         0x00
#define ATA_REG_ERROR        0x01
#define ATA_REG_FEATURES     0x01
#define ATA_REG_SECCOUNT0    0x02
#define ATA_REG_LBA0         0x03
#define ATA_REG_LBA1         0x04
#define ATA_REG_LBA2         0x05
#define ATA_REG_DRIVE_SELECT 0x06
#define ATA_REG_COMMAND      0x07
#define ATA_REG_STATUS       0x07



//##offset from control base
#define ATA_ALTERNATE_STATUS_REGISTER   0
#define ATA_DEVICE_CTRL_REGISTER        0
#define ATA_DRIVE_ADDRESS               1

#define ATA_SECTOR_SIZE 512

//io bases 
#define ATA_PRIMARY_IO_BASE 0x1F0
#define ATA_PRIMARY_CTRL_BASE 0x3F6


#define ATA_SECONDARY_IO_BASE 0x170
#define ATA_SECONDARY_CTRL_BASE 0x376


// ATA Commands
#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY_DEVICE 0xEC
#define ATA_CMD_IDENTIFY_PACKET_DEVICE 0xA1
#define ATA_CMD_FLUSH_CACHE 0xe7

// ATAPI Commands
#define ATAPI_IDENTIFY_PACKET_DEVICE 0xA1

//ATAPI DEVICE TYPES
#define ATAPI_DEVICE_HDD 0
#define ATAPI_DEVICE_TAPE 1
#define ATAPI_DEVICE_CDROM 5


// ATA Status Flags
#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_BSY  0x80


#include <isr.h>

#define ATA_MASTER_IRQ 14
void ata_master_irq_handler(struct regs* r);
void ata_400ns_delay(uint16_t io);

int ata_find_devices();

void ata_initialize(uint16_t io_base, uint16_t ctrl_base);
int ata_send_identify(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t *buffer);
int atapi_send_identify(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t *buffer);

int ata_send_command(uint16_t io_base, uint16_t ctrl_base, int drive, uint8_t command, uint8_t* buffer);

void ata_register_device(int bus, int drive, int sector_count, const uint8_t serial_number[20], const char modelname[40]);
int ata_register_partitions(int bus, int drive, int sector_count);
void atapi_register_device(int bus, int drive, const uint8_t serial_number[20], const char modelname[40]);


int ata_read_sector(uint16_t bus, uint16_t drive, uint32_t lba, uint8_t *buffer);
int ata_write_sector(uint32_t bus, uint32_t drive, uint32_t lba, uint8_t *buffer);



struct ata_cache_node{
    struct ata_cache_node* next;
    uint8_t disk_index; //0b10 bus, 0b1 drive
    uint32_t lba_index;
    uint8_t* data;

    int is_dirty;

  
};

struct ata_cache{
    struct ata_cache_node* head;
    int current_cache_count;
    int max_cache_count;
    uint8_t* raw_buffer;
};


void initialize_ata_cache(struct ata_cache* cache_struct, int max_cache_count);

typedef struct {
    uint8_t data[ATA_SECTOR_SIZE]; // Cached sector data
    uint32_t cached_lba;           // LBA of the cached sector
    uint32_t valid;                     // Is the cache valid?
    
} ata_sector_cache_t;

typedef struct{
    uint16_t io_base;
    uint16_t ctrl_base;
    uint16_t drive_index; // 0 1

    uint32_t start_sector;
    uint32_t end_sector;
    
    uint8_t serial_number[20];
    uint8_t model_name[40];


} ata_device_t;


typedef struct{
    
    uint8_t drive_attr;
    uint8_t chs_partition_start[3];
    
    uint8_t partition_type;
    uint8_t chs_partition_last[3];

    uint32_t LBA_start_partition;
    uint32_t number_of_sectors;

} __attribute__((packed)) mbr_partition_entry_t;


/*ata device types*/
enum ata_device_type{

    ATA_TYPE_ATA = 1,
    ATA_TYPE_ATAPI ,
    ATA_TYPE_SATA ,
    ATA_TYPE_SATAPI,
};

int ata_wait_ready(uint16_t io_base);
enum ata_device_type ata_determine_dev_type(int bus, int drive);
void ata_soft_reset_bus(int bus);

uint32_t ata_write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t  ata_read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
int ata_read(ata_device_t* ata, uint32_t offset, uint32_t size, uint8_t* buffer);
int ata_write(ata_device_t* ata, uint32_t offset, uint32_t size, uint8_t* buffer);
int ata_is_mbr(uint8_t* sector0);



#endif