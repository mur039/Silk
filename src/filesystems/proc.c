#include <filesystems/proc.h>
#include <filesystems/fs.h>
 
// #include <tty.h>
// #include <filesystems/pts.h>
// static void proc_pid_open(struct fs_node *node , int flags){
//     //impl contains pid, inode contains the file index which specifies the data
//     pid_t proc_pid = node->impl;
//     uint32_t attr_index = node->inode;
    
//     struct filestr* file = kcalloc(1, sizeof(struct filestr));

//     //should exist. this is what happens when you blindly copy paste :/
//     pcb_t* proc = process_get_by_pid(proc_pid);
//     uint8_t *byteptr;
//     char localbuffer[256];

//     switch(attr_index){
//         case CMD_LINE:
//             node->device = file;
//             for(int i = 0; i < proc->argc; ++i){
//                 file->strlen += strlen(proc->argv[i]) + 1;
//             }

//             file->str_data = kcalloc(file->strlen, 1);

//             byteptr = file->str_data;
//             for(int i = 0; i < proc->argc; ++i){
                
//                 strcpy(byteptr, proc->argv[i]);
//                 byteptr += strlen(proc->argv[i]) + 1;
                
//             }

//         pmm_alloc_is_chain_corrupted();
//         break;

//         case CMD_MMAP:
//         node->device = file;
//         file->strlen = 0;
        
//         for(listnode_t* lnode = proc->mem_mapping->head; lnode ; lnode = lnode->next){
//             vmem_map_t* vmem = lnode->val;
//             if(vmem->attributes & VMM_ATTR_MASK){

//                 int attribute = vmem->attributes & VMM_ATTR_MASK;
//                 int page_flags = get_virtaddr_flags((void*)vmem->vmem);
//                 file->strlen += sprintf(
//                                     localbuffer, "%x-%x r%cx%c\n", 
//                                     vmem->vmem,  vmem->vmem + 0x1000, 
//                                     page_flags & PAGE_READ_WRITE ? 'w' : '-' ,
//                                     attribute & VMM_ATTR_PHYSICAL_SHARED ? 's' : 'p'
//                                 );
//             }
//         }
        
//         file->str_data = kcalloc(file->strlen + 1, 1);
//         byteptr = file->str_data;
//         for(listnode_t* lnode = proc->mem_mapping->head; lnode ; lnode = lnode->next){
//             vmem_map_t* vmem = lnode->val;
            
//             if(vmem->attributes & VMM_ATTR_MASK){
                    
//                 int attribute = vmem->attributes & VMM_ATTR_MASK;
//                 int page_flags = get_virtaddr_flags((void*)vmem->vmem);
//                 byteptr += sprintf(
//                     byteptr, "%x-%x r%cx%c\n", 
//                     vmem->vmem,  vmem->vmem + 0x1000, 
//                     page_flags & PAGE_READ_WRITE ? 'w' : '-' ,
//                     attribute & VMM_ATTR_PHYSICAL_SHARED ? 's' : 'p'
//                 );
                
//             }
//         }

//         pmm_alloc_is_chain_corrupted();

//         break;

//         case CMD_STAT:
//         node->device = file;
//         file->strlen = 0;

//         int tty_nr;
//         tty_t* tty = ((tty_t*)proc->ctty);
//         if(!tty) tty_nr = 0;
//         else{
            
//             int major =  0, minor = 0;
//             switch(tty->driver->num){
                
//                 case 1: //serial
//                     major = 4;
//                     minor = 64; 
//                     minor += tty->index;
//                     break;
//                 case 2: //vt
//                     major = 4;
//                     minor += tty->index;
//                 break;

//                 case 3: //pts
                
//                     major = 136;
//                     minor = tty->index;
//                     break;
//                 default: 
//                 break;
//             }
            
//             tty_nr = (major << 8) | minor;
//         }

//         char task_state_ch;
//         if(proc->state == TASK_RUNNING) task_state_ch = 'R';
//         else if(proc->state == TASK_INTERRUPTIBLE) task_state_ch = 'S';
//         else if(proc->state == TASK_UNINTERRUPTIBLE) task_state_ch = 'D';
//         else if(proc->state == TASK_ZOMBIE) task_state_ch = 'Z';
//         else if(proc->state == TASK_STOPPED) task_state_ch = 'T';
        
//         file->strlen = sprintf(
//                                 localbuffer, "%u (%s) %c %u %u %u %u", 
//                                 proc->pid, proc->filename, 
//                                 task_state_ch,
//                                 proc->parent ? ((pcb_t*)(proc->parent->val))->pid : 0,
//                                 proc->pgid,
//                                 proc->sid,
//                                 tty_nr
//                             );
//         file->str_data = kcalloc(file->strlen, 1);
//         memcpy(file->str_data, localbuffer, file->strlen);

//         break;

//         default:break;
//     }
//     return;
// }



struct proc_entry* root;
struct proc_entry* proc_create_entry(const char* name, uint32_t mode, struct proc_entry* parent){
    struct proc_entry* fparent;
    if(parent) fparent = parent;
    else fparent = root;

    struct proc_entry* entry = kcalloc(1, sizeof(struct proc_entry));
    entry->parent = fparent;
    entry->proc_mode = mode;
    entry->subdir = list_create();
    entry->proc_name = strdup(name);
    list_insert_end(&fparent->subdir, entry);

    return entry;
}

void remove_proc_entry(const char* name, struct proc_entry* parent){
    struct proc_entry* fparent;
    if(parent) fparent = parent;
    else fparent = root;

    for(listnode_t* head = fparent->subdir.head; head ; head = head->next){
        struct proc_entry* e = head->val;
        if(!strcmp(e->proc_name, name)){
            list_remove(&fparent->subdir, head);
            kfree(e);
            //i should cleanup subdirectories as well...
            break;
        }
    }

    return;
}


struct dirent* proc_readdir(struct fs_node* fnode , uint32_t index){
    
    struct proc_entry* entry = fnode->device;
    
    //out of index
    if(index > entry->subdir.size){
        fnode->offset = 0;
        return NULL;
    }

    listnode_t* n = entry->subdir.head;
    for(uint32_t i = 0; i < index; ++i) n = n->next;
    
    if(!n){
        fnode->offset = 0;
        return NULL;
    }
    entry = n->val;
    
    struct dirent* out = kcalloc(1, sizeof(struct dirent));
    out->ino = 1;
    out->off = index;
    out->reclen = 0;
    strcpy(out->name, entry->proc_name);
    switch(entry->proc_mode & _IFMT){
        case _IFDIR:
            out->type = FS_DIRECTORY;
            break;
        case _IFCHR:
            out->type = FS_CHARDEVICE;
            break;
        case _IFBLK:
            out->type = FS_BLOCKDEVICE;
            break;
        case _IFREG:
            out->type = FS_FILE;
            break;
        case _IFLNK:
            out->type = FS_SYMLINK;
            break;
        case _IFSOCK:
            out->type = FS_SOCKET;
            break;
        case _IFIFO:
            out->type = FS_PIPE;
            break;
        default :
            break;
    }

    fnode->offset++;
    return out;
}

struct fs_node* proc_finddir(struct fs_node* fnode , char* name){
    
    struct proc_entry* entry = fnode->device;
    for(listnode_t* head = entry->subdir.head; head ; head = head->next ){
        
        struct proc_entry* e = head->val;
        if(!strcmp(name, e->proc_name)){
            //found the entry
            struct fs_node* node = kcalloc(1, sizeof(struct fs_node));
            strcpy(node->name, name);

            node->device = e;
            node->ops = e->proc_ops;

            switch(e->proc_mode & _IFMT){
                case _IFDIR:
                    node->flags = FS_DIRECTORY;
                    node->ops.readdir = (readdir_type_t)proc_readdir;
                    node->ops.finddir = (finddir_type_t)proc_finddir;
                    break;

                case _IFREG:
                    node->flags = FS_FILE;
                    break;

                case _IFCHR:
                    node->flags = FS_CHARDEVICE;
                    break;
                case _IFBLK:
                    node->flags = FS_BLOCKDEVICE;
                    break;
                    
                case _IFLNK:
                    node->flags = FS_SYMLINK;
                    break;
                case _IFSOCK:
                    node->flags = FS_SOCKET;
                    break;
                case _IFIFO:
                    node->flags = FS_PIPE;
                    break;
                default :
                    break;
            }
            return node;
        }
    }
    
    return NULL;
}




int proc_probe(fs_node_t* nodev){ //what to probe my dear

    if(nodev != NULL){
        return 0;
    }

    return 1;
}

fs_node_t* proc_mount(fs_node_t* nodev, const char* option){
    if(nodev != NULL){
        return NULL;
    }

    fs_node_t * fnode = kcalloc(sizeof(fs_node_t), 1);
    if(!fnode){
        return NULL;
    }
        
	fnode->inode = 1; //root inode ofcs
	strcpy(fnode->name, "proc");
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_DIRECTORY;
    
	fnode->ops.readdir = (readdir_type_t)proc_readdir;
	fnode->ops.finddir = (finddir_type_t)proc_finddir;
    fnode->device = root;
        
    return fnode;
}

struct filesystem procfs = {
    .fs_name = "proc",
    .fs_flags = 0,
    .probe = &proc_probe,
    .mount = &proc_mount,
    .next = NULL
};


int proc_init(){

    //initialize the root node
    root = kcalloc(1, sizeof(struct proc_entry));
    root->proc_mode = _IFDIR | 0644;
    fs_register_filesystem(&procfs);
    return 0;
}