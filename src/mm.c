#include <mm.h>
extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t bootstrap_pte2[1024];

uint32_t * K_PDE = bootstrap_pde;

void memory_manager_init(){
    
    return;
}
