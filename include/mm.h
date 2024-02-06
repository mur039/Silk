#ifndef __MM_H_
#define __MM_H_
/*only works after high-kernel initialized*/
void memory_manager_init();
uint32_t * mm_kalloc_frame();
uint32_t * mm_kfree_frame();
#endif