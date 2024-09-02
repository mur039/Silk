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
    uint32_t eax; // 0
    uint32_t ecx; // 4
    uint32_t edx; // 8
    uint32_t ebx; // 12
    uint32_t esp; // 16
    uint32_t ebp; // 20
    uint32_t esi; // 24
    uint32_t edi; // 28
    uint32_t eflags; //32
    uint32_t cr3; // 36
    uint32_t eip; //40

    //segment registers
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t ss;
    uint32_t fs;
    uint32_t gs;
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
    int argc;
    char **argv;
    file_t open_descriptors[8]; //max opened files
}pcb_t;


extern list_t * process_list;
extern pcb_t * current_process;


void process_init();
void schedule(struct regs * r);
pcb_t * create_process(char * filename, char **_argv);
int context_switch_into_process(struct regs  *r, pcb_t * process);
void print_processes();
void print_current_process();


pcb_t * terminate_process(pcb_t * p);



#endif