#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>
#include <vmm.h>
#include <isr.h>
#include <g_list.h>
#include <timer.h>
#include <fb.h>
#include <filesystems/tar.h>
#define MAX_OPEN_FILES  16


#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_ZOMBIE             4
#define TASK_STOPPED            8
#define TASK_SWAPPING           16
#define TASK_EXCLUSIVE          32
#define TASK_CREATED            64
#define TASK_LOADING            128

typedef int pid_t;
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
    int argc;
    char **argv;
    context_t regs;
    void * stack;
    file_t open_descriptors[MAX_OPEN_FILES]; //max opened files
    u32 state;


    //identifiying info, different between childs and parents
    pid_t pid;
    listnode_t * self;
    u32 time_slice;
    u32 * page_dir;
    listnode_t* parent;
    list_t* childs;

    char * cwd;
    list_t * mem_mapping;
    int recv_signals;
    u8 * kstack;

}pcb_t;


extern list_t * process_list;
extern pcb_t * current_process;


void process_init();
void schedule(struct regs * r);
pcb_t * create_process(char * filename, char **_argv);
int context_switch_into_process(struct regs  *r, pcb_t * process);
void save_context(struct regs * r, pcb_t * process);

void print_processes();
void print_current_process();

pcb_t * terminate_process(pcb_t * p);
pcb_t * process_get_by_pid(pid_t pid);
pcb_t * process_get_runnable_process();
pcb_t * load_process(pcb_t * proc);

void list_vmem_mapping(pcb_t * process);

pcb_t * create_kernel_process(void * stack_base, void* esp, void * eip, void* arg);
void save_current_context(pcb_t * proc);




#endif