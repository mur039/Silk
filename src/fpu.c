#include <fpu.h>

static uint8_t initial_fpu_state[512] __attribute__((aligned(16))); //as a template of sort

//due to how i allocate my task struct the fpu state probably is not 16-byte aligned so an intermediate buffer
static uint8_t fpu_scratch[512] __attribute__((aligned(16))); //as a template of sort


void initialize_fpu(){
    //actually check if fpu exists tho

    asm volatile("finit\r\n"); //reset fpu intto a known state
    
    uint32_t cr0 = get_cr0();
    cr0 &= ~((FPU_CR0_X87_EM | FPU_CR0_TS)); //clear em and ts fields
    set_cr0(cr0);

    uint32_t cr4 = get_cr4();
    cr4 |= FPU_CR4_OSFXSR_BIT | FPU_CR4_OSXMMEXCPT_BIT; //set osfxsr and osxmmexcpt

    asm volatile ("fxsave %0" : "=m"(initial_fpu_state)); //save it for later
    return;
}

void initialize_fpu_user(pcb_t* process){
    memcpy(process->fpu, initial_fpu_state, 512);
}

void fpu_save_to_buffer(fpu_state_t fpu){
    asm volatile ("fxsave %0" : : "m" (fpu_scratch));
    memcpy(fpu, fpu_scratch, 512);
    return;
}

void fpu_load_from_buffer(const fpu_state_t fpu){
    memcpy(fpu_scratch, fpu, 512);
    asm volatile ("fxrstor %0" : : "m" (fpu_scratch));
    return;
}