#include <filesystems/fs.h>
#include <filesystems/proc.h>
#include <module.h>
#include <str.h>

filesystem_t* registered_filesystems = NULL;


int fs_register_filesystem( filesystem_t* fs){
    fs->next = registered_filesystems;
    registered_filesystems = fs;
    return 0;
}
EXPORT_SYMBOL(fs_register_filesystem);


filesystem_t* fs_get_by_name(const char* fs_name){
    for(filesystem_t* head = registered_filesystems; head != NULL; head = head->next){
        if(!strcmp(head->fs_name, fs_name)){
            return head;
        }
    }

    return NULL;
}




uint32_t fs_proc_read(fs_node_t* nnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    size_t wholelength = 0;
    for(filesystem_t* head = registered_filesystems; head != NULL; head = head->next){
        char lb[32];
        sprintf(lb, "%s %s\n", head->fs_flags & FS_FLAGS_REQDEV ? "     ":"nodev", head->fs_name);
        wholelength += strlen(lb);
    }

    wholelength += 16; //guard

    char* table = kcalloc(wholelength, 1);
    for(filesystem_t* head = registered_filesystems; head != NULL; head = head->next){
        char lb[32];
        sprintf(lb, "%s %s\n", head->fs_flags & FS_FLAGS_REQDEV ? "     ":"nodev", head->fs_name);
        strcat(table, lb);
    }

    if(offset > wholelength){
        kfree(table);
        return 0;
    }

    size_t remain = wholelength - offset;
    if(size > remain){
        size = remain;
    }

    memcpy(buffer, &table[offset], size);
    kfree(table);
    return size;
}

static int fs_filesystem_init(){
    struct proc_entry* e = proc_create_entry("filesystems", _IFREG | 644, NULL);
    e->proc_ops.read = &fs_proc_read;
    return 1;
}

MODULE_INIT(fs_filesystem_init)