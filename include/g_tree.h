#ifndef __G_TREE_H__
#define __G_TREE_H__

#include <sys.h>
#include <pmm.h>

#include <g_list.h>

typedef struct treenode {
	void * val;

	list_t* leafs;
    struct treenode* parent;

}treenode_t;


typedef struct tree {
    size_t nodes;
    treenode_t * root;
}tree_t;



treenode_t* g_tree_insert(treenode_t * leaf, void * val);





#endif