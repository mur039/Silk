#include <filesystems/nulldev.h>

uint32_t read_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	if (size < 1) {
		return 0;
	}
	memset(buffer, 0x00, 1);
	return 0;
}

uint32_t write_null(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	return size;
}

void open_null(fs_node_t * node, uint8_t read, uint8_t write) {
	return;
}

void close_null(fs_node_t * node) {
	return;
}

fs_node_t * null_device_create() {
	fs_node_t * fnode = kmalloc(sizeof(fs_node_t));
	memset(fnode, 0x00, sizeof(fs_node_t));
	fnode->inode = 0;
	strcpy(fnode->name, "null");
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_CHARDEVICE;
	fnode->read    = read_null;
	fnode->write   = write_null;
	fnode->open    = open_null;
	fnode->close   = close_null;
	fnode->readdir = NULL;
	fnode->finddir = NULL;
	fnode->ioctl   = NULL;
	return fnode;
}

//test


