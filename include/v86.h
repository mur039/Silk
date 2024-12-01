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
#endif