#include <stdint.h>
#include <16tty.h>
struct VbeInfoBlock{
    char VBESignature[4]; //== "VESA"
    uint16_t VbeVersion;              // == 0x0300 for VBE 3.0
    uint16_t OemStringPtr[2];         // isa vbeFarPtr
    uint8_t  Capabilities[4];
    uint16_t VideoModePtr[2];         // isa vbeFarPtr
    uint16_t TotalMemory;             // as # of 64KB blocks
    uint8_t  Reserved[492];
} __attribute__((packed));

struct VBEModeInfoBlock {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed));

extern int getVbeInfo(struct VbeInfoBlock  *vib); //implemented in asm
extern int getVbeModeInfo(struct VBEModeInfoBlock  *vmib, uint16_t mode); //implemented in asm
extern int setVbeMode(uint16_t mode);
extern int queryModes(uint16_t seg, uint16_t offset);

struct VbeInfoBlock vib = { .VBESignature = {'V', 'B', 'E', '2'} };
struct VBEModeInfoBlock vmib;
//struct VbeInfoBlock vib;
void k16main(){
    //short ax;
    init_screen();
    
    if(getVbeInfo(&vib) == 0 || vib.VbeVersion != 0x0300){ //things that i would panic
        puts("VBE is not valid");
        return;
    }

    uint16_t * modes = (uint16_t *)((vib.VideoModePtr[1] << 4) + vib.VideoModePtr[0]);
   
    
    /*
    for(int i = 0;  modes[i] != 0xFFFF; ++i ){
        getVbeModeInfo(&vmib, modes[i]);
        if(((vmib.attributes >> 4) & 1) && vmib.height < 1000 && vmib.width < 1000 ){ //graphics mode
            printf("%x : %u %u %x %u", modes[i], vmib.height, vmib.width, vmib.memory_model, vmib.bpp);
            putchar(i & 1 ? '\n': '\0');
        }
        
    }
    */
    getVbeModeInfo(&vmib, 0x143);
    printf("%x %x", vmib.width, vmib.height);
    //setVbeMode(0x143 | 0x4000);
    
    for(;;);
        
    return;  //straight down to shutdown
}