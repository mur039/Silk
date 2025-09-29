#include <sharedmem.h>
#include <syscalls.h>
#include <pmm.h>
#include <vmm.h>

#define MAX_SHARED_MEMORY_OBJECTS 16
struct shm_segment* shared_objs[MAX_SHARED_MEMORY_OBJECTS];

#define SHM_GETINDEX(obj) (obj)
static struct shm_segment* sharedmem_get_by_key(key_t key){
    for(int i = 0; i < MAX_SHARED_MEMORY_OBJECTS; ++i){
        if(shared_objs[i] && shared_objs[i]->key == key){
            return shared_objs[i];
        }
    }

    return NULL;
}

static int sharedmem_empty_entry(){
    for(int i = 0; i < MAX_SHARED_MEMORY_OBJECTS; ++i){
        if(!shared_objs[i]){
            return i;
        }
    }

    return -1;
}

static int sharedmem_create_segment(key_t key, size_t size, int flags){

    int index = sharedmem_empty_entry();
    if(index < 0){
        return -ENOBUFS;
    }

    //roll the size;
    if(size < SHMMIN || size > SHMMAX){
        return -EINVAL;
    }


    struct shm_segment* s;
    s = kcalloc(1, sizeof(struct shm_segment));
    s->flags = flags & 0xfff;
    s->size = size;
    s->key = key;
    s->shmid = index;
    s->refcount = 1;
    s->pages = kpalloc(size / 4096);
    struct shm_segment** entry = &shared_objs[index];
    *entry = s;

    return index;
}


// int shmget(key_t key, size_t size, int shmflg);
void syscall_shmget(struct regs* r){
    key_t key = (key_t)r->ebx;
    size_t size = (size_t)r->ecx;
    int shmflag = (int)r->edx;

    int flags = shmflag & SHM_MODE_MASK;
    struct shm_segment *seg = NULL;

    
    
    if(shmflag & (IPC_CREAT | IPC_EXCL) ){
        
        if(shmflag & IPC_CREAT){
            
            //if there's exiting segment with same key, return it
            struct shm_segment* s = sharedmem_get_by_key(key);
            if(s){  

                if(size > s->size){
                    r->eax = -EINVAL;
                    return;
                }
                r->eax = s->shmid;
                return;
            }

            r->eax = sharedmem_create_segment(key, size, shmflag);
            return;
        }
        else if(shmflag & IPC_EXCL){
            
            struct shm_segment* s = sharedmem_get_by_key(key);
            if(s){  
                r->eax = -EEXIST;
                return;
            }

            r->eax = sharedmem_create_segment(key, size, shmflag);
            return;
        }

    }
    else{ //only look
           
       struct shm_segment* s = sharedmem_get_by_key(key);
       if(!s){  
           r->eax = -ENOENT;
           return;
        }
        
        r->eax = s->shmid; //just table index
        return;
    }
    
    
    r->eax = -EINVAL;
    return;
}


// void shmat(int shmid, const void * shmaddr, int shmflg);
void syscall_shmat(struct regs* r){
    r->eax = -ENOSYS;
    return;
}

// int shmdt(const void *shmaddr);
void syscall_shmdt(struct regs* r){
    r->eax = -ENOSYS;
    return;
}



int sharedmem_initialize(){
    memset(shared_objs, 0, MAX_SHARED_MEMORY_OBJECTS * sizeof(struct shm_segment*));
    install_syscall_handler(SYSCALL_SHMGET, syscall_shmget);
    install_syscall_handler(SYSCALL_SHMAT, syscall_shmat);
    install_syscall_handler(SYSCALL_SHMDT, syscall_shmdt);
    return 0;
}
