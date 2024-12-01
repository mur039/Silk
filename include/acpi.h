#ifndef __ACPI_H_
#define __ACPI_H_

#include <sys.h>
#include <pmm.h>
#include <stdint-gcc.h>

typedef struct RSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed)) rsdp_t;

struct XSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0
 
 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__ ((packed));

typedef struct{
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__((packed)) ACPISDTHeader_t;


rsdp_t find_rsdt();
int rsdp_is_valid(rsdp_t rdsp);

typedef struct {
  ACPISDTHeader_t h;
  uint32_t* PointerToOtherSDT;
  // [(h.Length - sizeof(h)) / 4];
} __attribute__((packed)) RSDT_t;


int doSDTChecksum(ACPISDTHeader_t *tableHeader);
void *find_sdt_by_signature(void *RootSDT, const char * signature);


#endif