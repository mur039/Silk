#ifndef __V86_H__
#define __V86_H__

#include <process.h>

#define V86_VM_BIT 17
#define V86_IF_BIT 9

#define V86_PUSHF 0x9c
#define V86_POPF 0x9d
#define V86_INT_N 0xCD
#define V86_OUT_AL_DX 0xEE
#define V86_OUT_AX_DX 0xEF

#define V86_IN_AL_DX 0xEC
#define V86_IN_AX_DX 0xED

#define V86_CLI 0xFA
#define V86_STI 0xFB
#define V86_IRET 0xCF

pcb_t * v86_create_task(char * filename);

void v86_monitor(struct regs * r, pcb_t * task);
int v86_int(int number, context_t* ctx); //?????? how nigga how??

struct VbeInfoBlock {
   char     VbeSignature[4];         // == "VESA"
   uint16_t VbeVersion;              // == 0x0300 for VBE 3.0
   uint16_t OemStringPtr[2];         // isa vbeFarPtr
   uint8_t  Capabilities[4];
   uint16_t VideoModePtr[2];         // isa vbeFarPtr
   uint16_t TotalMemory;             // as # of 64KB blocks
   uint8_t  Reserved[492];
} __attribute__((packed));

typedef struct VbeInfoBlock vbe_info_block_t;

#endif