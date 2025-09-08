#include <g_tree.h>
#include <sys.h>
#include <pmm.h>
#include <uart.h>


tree_t * tree_create() {
	/* Create a new tree */
	tree_t * out = kmalloc(sizeof(tree_t));
	if(!out)
		error("tree creationg failed");

	out->nodes  = 0;
	out->root   = NULL;
	return out;
}


void tree_set_root(tree_t * tree, void * value) {
	
	/* Set the root node for a new tree. */
	tree_node_t * root = tree_node_create(value);
	tree->root = root;
	tree->nodes = 1;
}


tree_node_t * tree_node_create(void * value) {
	/* Create a new tree node pointing to the given value */
	tree_node_t * out = kmalloc(sizeof(tree_node_t));
	if(!out)
		error("treenode creation failed");

	out->value = value;
	out->parent = NULL;
	
	out->children = kcalloc(1, sizeof(list_t));
    *out->children = list_create();
	return out;
}



void tree_node_insert_child_node(tree_t * tree, tree_node_t * parent, tree_node_t * node){
    list_insert_end(parent->children, node);
	node->parent = parent;
	tree->nodes++;

}

tree_node_t * tree_node_insert_child(tree_t * tree, tree_node_t * parent, void * value) {
	/* Insert a (fresh) node as a child of parent */
	tree_node_t * out = tree_node_create(value);
	tree_node_insert_child_node(tree, parent, out);
	return out;
}

#include <filesystems/vfs.h>
static void tree_traverse_helper_func( tree_node_t * node, int height){ //smart
    
    if (!node) return; //for invalid and stop recursion
    
    //indentation
    for(int i = 0; i < height; ++i){
        fb_console_put("  ");
    }
    
    fs_node_t* entry = node->value;
    
    fb_console_printf("->name:%s\n", entry->name );
    
    

    for(listnode_t* lnode = node->children->head; lnode ; lnode = lnode->next){
        tree_traverse_helper_func(lnode->val, height + 1);
    }


}

void tree_traverse(tree_t* tree){
    tree_traverse_helper_func(tree->root, 0);
}


