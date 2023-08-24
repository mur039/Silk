#include <acpi.h>

uint32_t * RSDT = NULL;
uint32_t * acpiCheckRSDPtr(uint32_t * addr){

    struct RSDP_t * rsdp = (struct RSDP_t *)addr;
    uint32_t chcksum = 0;
    uint8_t * head = (uint8_t *)rsdp;

    if(memcmp(rsdp->Signature, "RSD PTR ", 8)){
        for(unsigned long i = 0; i < sizeof(struct RSDP_t); ++i){
        chcksum += head[i];
        }
        if((chcksum & 0xFF) == 0){
            puts( rsdp->Revision == 0 ? "ACPI 1.0\n" : "ACPI 2.0\n" );
            return (uint32_t *)rsdp->RsdtAddress;
        }
    }
    
    return NULL;
}


unsigned int * acpiGetRSDPtr(void){

   uint32_t *addr;
   uint32_t *rsdp;

   // search below the 1mb mark for RSDP signature
   for (addr = (uint32_t *) 0x000E0000; (int) addr<0x00100000; addr += 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr(addr);
      if (rsdp != NULL)
         return (unsigned int *)rsdp;
   }

   // at address 0x40:0x0E is the RM segment of the ebda
   int ebda = *((short *) 0x40E);   // get pointer
   ebda = ebda*0x10 &0x000FFFFF;   // transform segment into linear address

   // search Extended BIOS Data Area for the Root System Description Pointer signature
   for (addr = (uint32_t *) ebda; (int) addr<ebda+1024; addr+= 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr(addr);
      if (rsdp != NULL)
         return (unsigned int *)rsdp;
   }
   return NULL;
}




