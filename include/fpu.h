#ifndef __FPU_H__
#define __FPU_H__

#include <pmm.h>
#include <process.h>


#define FPU_CR0_X87_EM_BIT 2
#define FPU_CR0_TS_BIT 3

#define FPU_CR0_X87_EM  (1ul << FPU_CR0_X87_EM_BIT)         
#define FPU_CR0_TS      (1ul << FPU_CR0_TS_BIT)     


#define FPU_CR4_OSFXSR_BIT       9 // Enable SSE instructions and FXSAVE/FXRSTOR.
#define FPU_CR4_OSXMMEXCPT_BIT  10    // Enable unmasked SIMD FP exceptions.

#define FPU_CR4_OSFXSR ( 1ul << FPU_CR4_OSFXSR_BIT)
#define FPU_CR4_OSXMMEXCPT ( 1ul << FPU_CR4_OSXMMEXCPT_BIT)


void initialize_fpu();
void initialize_fpu_user(pcb_t* process);
void fpu_save_to_buffer(fpu_state_t fpu);
void fpu_load_from_buffer(const fpu_state_t fpu);
#endif