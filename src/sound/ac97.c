#include <sound/ac97.h>
#include <sys.h>
#include <pmm.h>
#include <vmm.h>
#include <dev.h>
#include <fb.h>


struct fs_ops ac97_fs_ops = {
    
};
 struct bdl_desc {
        uint32_t phyaddr;
        uint16_t nos;
        uint16_t misc;
    } __attribute__((packed));

void ac97_beep(pci_device_t* pdev);
int ac97_pci_initialize(pci_device_t* pdev){
    

    return 0;
    uint32_t* bars = (uint32_t*)&pdev->header.type_0.base_address_0;
        
    uint16_t nam = bars[0] & ~1ul;
    uint16_t nabm = bars[1] & ~1ul;

    ac97_beep(pdev);
    // halt();
    return 0;

    outw(nam + AC97_RESET_CAP_REG, 0x2);
    io_wait();
    outw(nam + AC97_RESET_CAP_REG, 0x0);

    outw(nam + AC97_PCM_OUT_VOL_REG, 0x0F0F); //sets maximum volume

    uint16_t* beepbuf = kcalloc(256, 2);
     for(int i=0;i<256;i++){
        beepbuf[i] = (i%50 < 25) ? 0x7FFF : -0x8000; // 16-bit PCM
    }

   

    outb(nabm + AC97_NABM_PCM_OUT + AC97_NABM_N_BUFF_DESC_ENTRY_REG, 1); //one entry
    //bdl descriptor list must be 4byte aligned

    char* backingmem = kcalloc(2, sizeof(struct bdl_desc));
    struct bdl_desc* desc = (struct bdl_desc*)(((uintptr_t)backingmem + 3) & ~3ul);
    
    desc->phyaddr = (uint32_t)get_physaddr(beepbuf);
    desc->nos = 256;
    desc->misc =  0 | (1 << 14);

    outl(nabm + AC97_NABM_PCM_OUT + AC97_NABM_PHYS_BUFF_DESC_REG, (uint32_t)get_physaddr(desc));

    pci_enable_bus_mastering(pdev);
    outb(nabm + AC97_NABM_PCM_OUT + AC97_NABM_TRANSFER_CTRL_REG, 1); //start the playback

        
    halt();


    return 0;
}


void ac97_beep(pci_device_t* pdev) {
    // Get port I/O addresses
    uint16_t* bars = (uint16_t*)&pdev->header.type_0.base_address_0;
    uint16_t nam_port  = bars[0] & ~1; // NAM I/O port
    uint16_t nabm_port = bars[1] & ~1; // NABM I/O port

    // Reset NAM
    outw(nam_port + AC97_RESET_CAP_REG, 0x2);
    io_wait();
    outw(nam_port + AC97_RESET_CAP_REG, 0x0);

    // Max PCM volume
    outw(nam_port + AC97_PCM_OUT_VOL_REG, 0x0F0F);

    // Prepare beep buffer (16-bit PCM, square wave)
    size_t beep_samples = 2048;
    int16_t* beepbuf = kcalloc(beep_samples, sizeof(int16_t));
    for (size_t i = 0; i < beep_samples; i++) {
        beepbuf[i] = (i % 50 < 25) ? 0x7FFF : -0x8000;
    }

    // Prepare aligned BDL
    char* bdl_mem = kcalloc(1, sizeof(struct bdl_desc) + 3);
    struct bdl_desc* bdl = (struct bdl_desc*)(((uintptr_t)bdl_mem + 3) & ~3UL);
    bdl->phyaddr = (uint32_t)get_physaddr(beepbuf);
    bdl->nos = beep_samples;
    bdl->misc = 1 << 14; // IOC

    // Reset PCM output channel
    outb(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_TRANSFER_CTRL_REG, 2);
    while (inb(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_TRANSFER_CTRL_REG) & 2) io_wait();

    // Set BDL
    outl(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_PHYS_BUFF_DESC_REG, (uint32_t)get_physaddr(bdl));
    outb(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_N_BUFF_DESC_ENTRY_REG, 0); // one entry (index 0)

    // Start transfer
    outb(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_TRANSFER_CTRL_REG, 1);

    // Poll until transfer is complete
    while (!(inw(nabm_port + AC97_NABM_PCM_OUT + AC97_NABM_STAT_TRANFERRING_REG) & 1)) {
        io_wait();
    }

    // Optional: loop to let beep play fully
    for (volatile int i = 0; i < 1000000; i++) { io_wait(); }

    kfree(beepbuf);
    kfree(bdl_mem);
}