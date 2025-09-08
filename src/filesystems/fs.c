#include <filesystems/fs.h>
#include <module.h>
#include <str.h>

filesystem_t* registered_filesystems = NULL;


int fs_register_filesystem( filesystem_t* fs){
    fs->next = registered_filesystems;
    registered_filesystems = fs;
    return 0;
}


filesystem_t* fs_get_by_name(const char* fs_name){
    for(filesystem_t* head = registered_filesystems; head != NULL; head = head->next){
        if(!strcmp(head->fs_name, fs_name)){
            return head;
        }
    }

    return NULL;
}

EXPORT_SYMBOL(fs_register_filesystem);