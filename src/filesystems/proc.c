#include <filesystems/proc.h>




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


    PROCESS_ATTRIBUTES_ENUM__EOL
};

const char * process_attribute[] = {
    [CMD_LINE] = "cmdline",
    
    
    
    [PROCESS_ATTRIBUTES_ENUM__EOL] = NULL,
};


static read_type_t proc_pid_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer);

static finddir_type_t proc_pid_finddir(struct fs_node* node, char *name){

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
	fnode->read    = proc_pid_read;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;
	fnode->readdir = NULL;
	fnode->finddir = NULL;
	fnode->ioctl   = NULL;
    return fnode;    
}






static struct dirent * proc_pid_readdir(fs_node_t *node, uint32_t index) {
	// fb_console_printf("pid_readdir: index:%u::%s\n", index, node->name);

	if(node->flags != FS_DIRECTORY){
        node->offset = 0;
		return NULL;
	}


    //we are going to list attributes of process
    uint32_t l_index;
    for(l_index = 0 ; l_index < index; l_index++){
        
        if(l_index != PROCESS_ATTRIBUTES_ENUM__EOL){    
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



static read_type_t proc_pid_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
        
    pcb_t * proc = process_get_by_pid(node->impl);

    if(node->offset > strlen(proc->filename) )
        return 0; //eof;

    
    size_t left_size = strlen(proc->filename) - node->offset;

    if(size > left_size){
        memcpy(buffer, &proc->filename[node->offset], left_size);
        node->offset += left_size;
        return left_size;
    }

    memcpy(buffer, &proc->filename[node->offset], size);
    node->offset += size;
    return size;

}
 


finddir_type_t proc_finddir(struct fs_node* node, char *name){

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
	        fnode->readdir = proc_pid_readdir;
	        fnode->finddir = proc_pid_finddir;
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
            fnode->flags   = FS_SYMLINK;
	        fnode->read    = NULL;
	        fnode->write   = NULL;
	        fnode->open    = NULL;
	        fnode->close   = NULL;
	        fnode->readdir = proc_pid_readdir;
	        fnode->finddir = proc_pid_finddir;
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
	fnode->readdir = proc_readdir;
	fnode->finddir = proc_finddir;
	fnode->ioctl   = NULL;
	return fnode;

}