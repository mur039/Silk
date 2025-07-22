#include <filesystems/proc.h>

//since these entries are readonly i can set them with length as well as linked list or well a buffer 
//where i allocate on open and deallocate on close?
// tho i could use a null terminated string, tho in cmdline, each argc is seperated by '\0'

struct filestr{
    char* str_data;
    size_t strlen;
};


static struct dirent * proc_readdir(fs_node_t *node, uint32_t index) {
	// fb_console_printf("prof_readdir: index:%u::%s\n", index, node->name);

	if(node->flags != FS_DIRECTORY){
        node->offset = 0;
		return NULL;
	}

    //we are going to list pid directories first then other files

    
    listnode_t* head = process_list->head;
    pcb_t * proc = head->val;

    uint32_t l_index;
    int out_of_pid = 0;

    for(l_index = 0  ; l_index < index;  l_index++){
    
        if(head){
            // proc = head->val;
            // fb_console_printf("\t->%u\n", proc->pid);
            head = head->next;
            continue;
        }

        node->offset = 0;
        return NULL;
    }



    if(head){
        proc = head->val;
        struct dirent* out = kcalloc(1, sizeof(struct dirent));
        out->ino = index;
        out->type = FS_DIRECTORY;
        out->off = index;
        out->reclen = 0;
        sprintf(out->name, "%u", proc->pid);
        node->offset++;
        return out;
    
    }
    else{ //out of pid
        int new_index = index - l_index;
        // fb_console_printf("new index : %u\n", new_index);
        switch(new_index){
            case 0:
                ;
                struct dirent* out = kcalloc(1, sizeof(struct dirent));
                out->ino = index;
                out->type = FS_SYMLINK;
                out->off = index;
                out->reclen = 0;
                strcpy(out->name, "self");
                node->offset++;
                return out;
    
                break;
            default:
                //not bitches
                break;

        }
        
    }





    node->offset = 0;
    return NULL;

}


enum process_attributes_enum{
    CMD_LINE = 0,
    CMD_MMAP,
    CMD_STAT,

    PROCESS_ATTRIBUTES_ENUM__EOL
};

const char * process_attribute[] = {
    [CMD_LINE] = "cmdline",
    [CMD_MMAP] = "maps",
    [CMD_STAT] = "stat",
    
    
    
    [PROCESS_ATTRIBUTES_ENUM__EOL] = NULL,
};


static uint32_t proc_pid_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);
static void proc_pid_open(struct fs_node *node , uint8_t read, uint8_t write);
static void proc_pid_close(struct fs_node *node);

static struct fs_node* proc_pid_finddir(struct fs_node* node, char *name){

    fb_console_printf("proc_pid_finddir: %s::%s\n", node->name, name);

    int  str_index = -1;
    for(int i = 0; process_attribute[i]; ++i){
        if(!strcmp(process_attribute[i], name)){
            str_index = i;
            break;
        }
    }

    if(str_index == -1){
        return NULL;
    }


    struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
    fnode->inode = str_index;
    strcpy(fnode->name, name);
    fnode->uid = 0;
	fnode->gid = 0;

    fnode->impl = node->impl;
    fnode->flags   = FS_FILE;
	fnode->read    = (read_type_t) proc_pid_read;
	fnode->write   = NULL;
	fnode->open    = (open_type_t)proc_pid_open;
	fnode->close   = (close_type_t)proc_pid_close;
	fnode->readdir = NULL;
	fnode->finddir = NULL;
	fnode->ioctl   = NULL;
    return fnode;    
}





static struct dirent * proc_pid_readdir(fs_node_t *node, uint32_t index) {
	// fb_console_printf("pid_readdir: index:%u::%s\n", index, node->name);

	if( !(node->flags  & (FS_DIRECTORY | FS_SYMLINK)) ){
        node->offset = 0;
		return NULL;
	}


    //we are going to list attributes of process
    uint32_t l_index;

    if(index >= PROCESS_ATTRIBUTES_ENUM__EOL){
        node->offset = 0;
        return NULL;
    }


    for(l_index = 0 ; l_index < index; l_index++){
        
        if(l_index == PROCESS_ATTRIBUTES_ENUM__EOL){    
            node->offset = 0;
            return NULL;
        }

    }


    struct dirent* out = kcalloc(1, sizeof(struct dirent));
    out->ino = index;
    out->type = FS_FILE;
    out->off = index;
    out->reclen = 0;
    strcpy(out->name, process_attribute[l_index]);
    node->offset++;
    return out;
}



static uint32_t proc_pid_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
        
    pcb_t * proc = process_get_by_pid(node->impl);
    int property_index = node->inode;
    
    struct filestr *file = node->device;

    if(!file)
        return 0;

    if(node->offset > file->strlen) 
        return 0;


    size_t left_size = file->strlen - offset;
    if(size > left_size){
        size = left_size; 
    }

    memcpy(buffer, &file->str_data[offset], size);
    node->offset += size;
    return size;

}
 
#include <tty.h>
#include <filesystems/pts.h>
static void proc_pid_open(struct fs_node *node , uint8_t read, uint8_t write){
    //impl contains pid, inode contains the file index which specifies the data
    pid_t proc_pid = node->impl;
    uint32_t attr_index = node->inode;
    
    struct filestr* file = kcalloc(1, sizeof(struct filestr));

    //should exist. this is what happens when you blindly copy paste :/
    pcb_t* proc = process_get_by_pid(proc_pid);
    uint8_t *byteptr;
    char localbuffer[256];

    switch(attr_index){
        case CMD_LINE:
            node->device = file;
            for(int i = 0; i < proc->argc; ++i){
                file->strlen += strlen(proc->argv[i]) + 1;
            }

            file->str_data = kcalloc(file->strlen, 1);

            byteptr = file->str_data;
            for(int i = 0; i < proc->argc; ++i){
                
                strcpy(byteptr, proc->argv[i]);
                byteptr += strlen(proc->argv[i]) + 1;
                
            }

        pmm_alloc_is_chain_corrupted();
        break;

        case CMD_MMAP:
        node->device = file;
        file->strlen = 0;
        
        for(listnode_t* lnode = proc->mem_mapping->head; lnode ; lnode = lnode->next){
            vmem_map_t* vmem = lnode->val;
            if(vmem->attributes >> 20){

                int attribute = vmem->attributes & 0xFF;
                int page_flags = get_virtaddr_flags((void*)vmem->vmem);
                file->strlen += sprintf(
                                    localbuffer, "%x-%x r%cx%c\n", 
                                    vmem->vmem,  vmem->vmem + 0x1000, 
                                    page_flags & PAGE_READ_WRITE ? 'w' : '-' ,
                                    attribute & VMM_ATTR_PHYSICAL_SHARED ? 's' : 'p'
                                );
            }
        }
        
        file->str_data = kcalloc(file->strlen + 1, 1);
        byteptr = file->str_data;
        for(listnode_t* lnode = proc->mem_mapping->head; lnode ; lnode = lnode->next){
            vmem_map_t* vmem = lnode->val;
            
            if(vmem->attributes >> 20){
                    
                int attribute = vmem->attributes & 0xFF;
                int page_flags = get_virtaddr_flags((void*)vmem->vmem);
                byteptr += sprintf(
                    byteptr, "%x-%x r%cx%c\n", 
                    vmem->vmem,  vmem->vmem + 0x1000, 
                    page_flags & PAGE_READ_WRITE ? 'w' : '-' ,
                    attribute & VMM_ATTR_PHYSICAL_SHARED ? 's' : 'p'
                );
                
            }
        }

        pmm_alloc_is_chain_corrupted();

        break;

        case CMD_STAT:
        node->device = file;
        file->strlen = 0;

        int tty_nr;
        tty_t* tty = ((tty_t*)proc->ctty);
        if(!tty) tty_nr = 0;
        else{
            
            int major =  0, minor = 0;
            switch(tty->driver->num){
                
                case 1: //serial
                    major = 4;
                    minor = 64; 
                    minor += tty->index;
                    break;
                case 2: //vt
                    major = 4;
                    minor += tty->index;
                break;

                case 3: //pts
                
                    major = 136;
                    minor = tty->index;
                    break;
                default: 
                break;
            }
            
            tty_nr = (major << 8) | minor;
        }
        file->strlen = sprintf(
                                localbuffer, "%u (%s) %c %u %u %u %u", 
                                proc->pid, proc->filename, 
                                proc->state  == TASK_RUNNING ? 'R' : '?',
                                ((pcb_t*)(proc->parent->val))->pid,
                                proc->pgid,
                                proc->sid,
                                tty_nr
                            );
        file->str_data = kcalloc(file->strlen, 1);
        memcpy(file->str_data, localbuffer, file->strlen);

        break;

        default:break;
    }
    return;
}

static void proc_pid_close(struct fs_node *node){

    struct filestr* file = node->device;
    if(file){

        kfree(file->str_data);
        kfree(file);
    }

    kfree(node); //?
    return;
}






struct fs_node* proc_finddir(struct fs_node* node, char *name){

    char* nname;
    nname = kcalloc(strlen(node->name) + strlen(name) + 1, 1);
    strcat(nname, node->name);    
    strcat(nname, name);

    
    fb_console_printf("proc_finddir: searching for %s::%s \n", nname, name);
    kfree(nname);


    int is_number = 1;
    for(size_t i = 0; i < strlen(name); ++i){
        if(!(name[i] >= '0' && name[i] <= '9')){
            is_number = 0;
            break;
        }
    }


    if(is_number){ //probably a pid
        pid_t pid = atoi(name);
        if(process_get_by_pid(pid)){ //such process exist with this pid then
            struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
            fnode->inode = 2;
            strcpy(fnode->name, name);
            fnode->uid = 0;
	        fnode->gid = 0;
            fnode->impl = pid;
            fnode->flags   = FS_DIRECTORY;
	        fnode->read    = NULL;
	        fnode->write   = NULL;
	        fnode->open    = NULL;
	        fnode->close   = NULL;
	        fnode->readdir = (readdir_type_t)proc_pid_readdir;
	        fnode->finddir = (finddir_type_t)proc_pid_finddir;
	        fnode->ioctl   = NULL;
            return fnode;
        }
        return NULL;
    
    }
    else{

        //we need to check the list but now i will return nothing
        if(!strcmp(name, "self")){
            struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
            fnode->inode = 3;
            strcpy(fnode->name, name);
            
            fnode->uid = 0;
	        fnode->gid = 0;
            fnode->impl = current_process->pid;
            fnode->flags   = FS_DIRECTORY;
	        fnode->read    = NULL;
	        fnode->write   = NULL;
	        fnode->open    = NULL;
	        fnode->close   = NULL;
	        fnode->readdir = (readdir_type_t)proc_pid_readdir;
	        fnode->finddir = (finddir_type_t)proc_pid_finddir;
	        fnode->ioctl   = NULL;
            return fnode;

        }

        return NULL;

    }

    return NULL;
}



fs_node_t * proc_create(){

	fs_node_t * fnode = kmalloc(sizeof(fs_node_t));
    if(!fnode)
        error("failed allocate fnode");

	memset(fnode, 0x00, sizeof(fs_node_t));
	fnode->inode = 0;
	strcpy(fnode->name, "proc");
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_DIRECTORY;
	fnode->read    = NULL;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;
	fnode->readdir = (readdir_type_t)proc_readdir;
	fnode->finddir = (finddir_type_t)proc_finddir;
	fnode->ioctl   = NULL;
	return fnode;
    
    
    return fnode;
}