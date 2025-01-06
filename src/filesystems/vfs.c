#include <filesystems/vfs.h>
#include <g_tree.h>

// typedef struct gtreenode {
//     gtreenode_t * children;
//     void * value;
// }gtreenode_t;

// typedef struct gtree {
//     gtreenode_t * root;
// }gtree_t;





static char** _vfs_parse_path(const char * path){
    
    list_t* paths;
    paths = kcalloc(1, sizeof(list_t));
    paths->size = 0;
    paths->head = NULL;
    paths->tail = NULL;

    char * previous_head = path;
    for(int i = 0; ; ++i){

    

        if(path[i] == '\0'){

            if(previous_head != &path[i]){

                size_t size = (size_t)(&path[i] - previous_head);
                char * name = kcalloc(1,size + 1);
                memcpy(name, previous_head, size);

                // uart_print(COM1, "%s\r\n", name);
                list_insert_end(paths, name);

            }
            break;
        }

        if(path[i] == '/'){
            
            size_t size = (size_t)(&path[i + 1] - previous_head);
            char * name = kcalloc(1,size + 1);
            memcpy(name, previous_head, size);

            // uart_print(COM1, "%s\r\n", name);
            list_insert_end(paths, name);
            
            previous_head = &path[i + 1];            
        }
        

    }
    
    
    //printing the list and construct an array of strings;
    char** retval = kcalloc(paths->size + 1, sizeof(char*));
    char** head = retval;
    
    for(listnode_t * node = paths->head;  node != NULL; node = node->next){
        
        char * val = node->val;
        *(head++) = val;
        *head = NULL;
        uart_print(COM1, "\t->%s\r\n", val);
    }


    //purge the list
    for(listnode_t * node = paths->head;  node != NULL; node = node->next){
        list_remove(paths, node); //uh
    }
    kfree(paths);


    return retval;
}



tree_t * vfs_tree = NULL;

void vfs_init() {

    if(!vfs_tree) 
        vfs_tree = kmalloc(sizeof(tree_t));
        vfs_tree->root = NULL;

    if(!vfs_tree->root) 
        vfs_tree->root = kcalloc(1, sizeof(treenode_t));
        


    vfs_node_t * root = kcalloc(1, sizeof(vfs_node_t));


    strcpy(root->name, "root");
    vfs_tree->root->val = root;
    vfs_tree->root->parent = vfs_tree->root;


    
}


int vfs_mount(const char* path, vfs_node_t * node){

    char ** paths = _vfs_parse_path(path);

    for(int i = 0; paths[i] != NULL; ++i){
        uart_print(COM1, "\t %x::%s\r\n", i, paths[i]);
    }

    
    //traverse the vfs node graph to find node specified in path

    
    char** phead = paths;

    treenode_t * t_node = vfs_tree->root; 
    vfs_node_t* v_node = t_node->val;
    
    
    for(int i = 0; phead[i] != NULL; ++i){
        if( !strcmp( ((vfs_node_t*)t_node->val)->name , phead[i] )){
            break;
        }

        //something something will implement later
        fb_console_put("VFS_MOUNT: IN LOOP TODO\n");
        break;
    }



    

    v_node = t_node->val;

    v_node->fs_type = VFS_NODE_TYPE_MOUNT_POINT;
    v_node->refcount++;
    // v_node->





    //free the array
    for(int i = 0; paths[i] != NULL; ++i){
        kfree(paths[i]);
    }
    kfree(paths);
}



vfs_node_t* vfs_open(const char* path, uint32_t flags){

    char ** paths = _vfs_parse_path(path);






    //free the array
    for(int i = 0; paths[i] != NULL; ++i){
        kfree(paths[i]);
    }
    kfree(paths);

    return NULL;
}