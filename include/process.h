#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>
#include <isr.h>
#include <g_list.h>
#include <timer.h>
#include <fb.h>
#include <filesystems/tar.h>



#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_ZOMBIE             4
#define TASK_STOPPED            8
#define TASK_SWAPPING           16
#define TASK_EXCLUSIVE          32
#define TASK_CREATED            64
#define TASK_LOADING            128

typedef unsigned int pid_t;
typedef struct context {
    u32 eax; // 0
    u32 ecx; // 4
    u32 edx; // 8
    u32 ebx; // 12
    u32 esp; // 16
    u32 ebp; // 20
    u32 esi; // 24
    u32 edi; // 28
    u32 eflags; //32
    u32 cr3; // 36
    u32 eip; //40
}context_t;

typedef struct {
    char filename[512];
    context_t regs;
    pid_t pid;
    listnode_t * self;
    void * stack;
    uint32_t state;
    uint32_t time_slice;
    uint32_t * page_dir;
}pcb_t;


extern list_t * process_list;
extern pcb_t * current_process;


void process_init();
void schedule(struct regs * r);
pcb_t * create_process(char * filename);
int context_switch_into_process(struct regs  *r, pcb_t * process);
void print_processes();
void print_current_process();


pcb_t * terminate_process(pcb_t * p);



#endif