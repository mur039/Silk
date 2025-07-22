#include <filesystems/pts.h>
#include <filesystems/vfs.h>
#include <tty.h>
#include <syscalls.h>

static struct fs_node* ptsnode = NULL;


struct ptypair{
    struct tty master;
    struct tty slave;
};

#define MAX_PTS_COUNT 32
static struct ptypair** masterslave_table = NULL;


static int is_tty_master(tty_t* tty){
    return (&masterslave_table[tty->index]->master == tty) ? 1 : 0;
}

void pts_driver_close(struct tty* tty){
     
    int index = tty->index;
    struct ptypair* pair = masterslave_table[index];
    tty_t *master, *slave;
    master = &pair->master;
    slave = &pair->slave;

    
    if(tty->refcount == 0){
        circular_buffer_destroy(&tty->linebuffer);
        circular_buffer_destroy(&tty->read_buffer);
        circular_buffer_destroy(&tty->write_buffer);


        if(tty == master){
            slave->eof_pending = 1;
            process_wakeup_list(&slave->read_wait_queue);
            //flush write buffer to the slave
        }
        else{
            master->eof_pending = 1;
            process_wakeup_list(&master->read_wait_queue);
            //flush write buffer to the master
        }

    }


    if(!master->refcount && !slave->refcount){ //both side are closed then
        kfree(pair);
        masterslave_table[index] = NULL;
    }
}

void pts_driver_open(struct tty* tty){

    int index = tty->index;
    struct ptypair* pair = masterslave_table[index];
    tty_t *master, *slave;
    master = &pair->master;
    slave = &pair->slave;

    if(master == tty){ // i mean master has to be open anyway sooooooo

    }
    else{ // if slave opens we can wakeup the master if its sleeping on the write_queue
    }

}



void pts_driver_putchar(struct tty* tty, char c){
    
    struct ptypair* pair = masterslave_table[tty->index];

    if(&pair->master ==  tty){ //we are the master
        tty_ld_write(&pair->slave, &c, 1);
    }
    else{
        
        circular_buffer_putc(&pair->master.read_buffer, c);
        process_wakeup_list(&pair->master.read_wait_queue); // notify master
        // tty_ld_write(&pair->master, &c, 1);
    }

}

void pts_driver_write(struct tty* tty, const char* c, size_t nbytes){
    
    for(size_t i = 0; i < nbytes; ++i){
        tty->driver->put_char(tty,c[i]);
    }
}



struct tty_driver pts_driver = {
    
    .name = "pts", 
    .num = 3,
    .close = &pts_driver_close,
    .write = &pts_driver_write,
    .put_char = &pts_driver_putchar
};



struct ptypair* pts_allocate_slot(){
    
    int index = -1;
    for(int i = 0; i < MAX_PTS_COUNT; ++i){

        if(!masterslave_table[i]){ //an empty entry goooooood
            index = i;
            break;
        }
    }

    if(index < 0){
        return NULL;
    }

    struct ptypair* pair = kcalloc(1, sizeof(struct ptypair));
    if(!pair){
        return NULL;
    }

    masterslave_table[index] = pair;

    pair->master.size.ws_col = 80;
    pair->master.size.ws_row = 25;
    pair->master.index = index;
    pair->master.termio.c_lflag = ISIG | ICANON ;
    pair->master.read_wait_queue = list_create();
    pair->master.read_buffer = circular_buffer_create(4096);
    pair->master.write_buffer = circular_buffer_create(4096);
    pair->master.linebuffer = circular_buffer_create(512);
    pair->master.driver = &pts_driver;

    pair->slave.size.ws_col = 80;
    pair->slave.size.ws_row = 25;
    pair->slave.index = index;
    pair->slave.termio.c_lflag = ISIG | ICANON | ECHO ;
    pair->slave.read_wait_queue = list_create();
    pair->slave.read_buffer = circular_buffer_create(4096);
    pair->slave.write_buffer = circular_buffer_create(4096);
    pair->slave.linebuffer = circular_buffer_create(512);
    pair->slave.driver = &pts_driver;

}


struct dirent * pts_readdir(struct fs_node* fnode, size_t index){
    
    if(index >= MAX_PTS_COUNT){ //max pts + ptmx hopefully
        fnode->offset = 0;
        return NULL;
    }
    
    if(index == 0){
        struct dirent* out = kcalloc(1, sizeof(struct dirent));
        out->ino = index;
        out->type = FS_CHARDEVICE;
        out->off = index;
        out->reclen = 0;
        strcpy(out->name, "ptmx");
        fnode->offset++;
        return out;
    }
    else if(masterslave_table[index - 1]){
        struct dirent* out = kcalloc(1, sizeof(struct dirent));
        out->ino = index;
        out->type = FS_CHARDEVICE;
        out->off = index;
        out->reclen = 0;
        sprintf(out->name, "%u", index - 1);
        fnode->offset++;
        return out;
    }

    fnode->offset = 0;
    return NULL;
}

int pts_ioctl(fs_node_t* fnode, int op, void* argp){

    tty_t* tty = ((device_t*)fnode->device)->priv;
    
    switch(op){

        case TIOCGPTN:
        if(!is_tty_master(tty)){
            return -ENOTTY;
        }
        *(uint32_t*)argp = tty->index;
        break;

        case TIOCSPTLCK:
        break;

        case TIOCGPTLCK:
        break;

        default:
            return tty_ioctl(fnode, op, argp);
        break;
    }

    return 0;

}


struct fs_node * pts_finddir(struct fs_node* fnode, char* name){
    
    //its simple the name is either ptmx or a decimal 0-31
    if(!strcmp(name, "ptmx")){ //ptmx
        
        struct ptypair* pair = pts_allocate_slot();
        if(!pair)
            return NULL;
        
        struct fs_node* fnode = kcalloc(1, sizeof(struct fs_node));
        sprintf(fnode->name, "ptmx");

        fnode->open  = (open_type_t)tty_open;
        fnode->close = (close_type_t)tty_close;
        fnode->write = (write_type_t)tty_write;
        fnode->read = (read_type_t)tty_read;
        fnode->ioctl = (ioctl_type_t)pts_ioctl; //????

        device_t* dev =  kcalloc(1, sizeof(device_t));
        dev->priv = &pair->master;
        fnode->device = (fs_node_t*)dev;

        return fnode;

    }
    else{ //must be a number
        //check if name is all numbers
        for(int i = 0; name[i] ; ++i){
            if(!IS_NUMBER(name[i])){
                return NULL;
            }
        }

        //a string consisting of decimal numbers
        int index = atoi(name);
        if(index < 0 || index > MAX_PTS_COUNT){
            return NULL;
        }

        struct ptypair* pair = masterslave_table[index];
        if(!pair){
            return NULL;
        }
        
        struct fs_node* fnode = kcalloc(1, sizeof(struct fs_node));
        sprintf(fnode->name, "%u", index);

        fnode->open  = (open_type_t)tty_open;
        fnode->close = (close_type_t)tty_close;
        fnode->write = (write_type_t)tty_write;
        fnode->read = (read_type_t)tty_read;
        fnode->ioctl = (ioctl_type_t)tty_ioctl; //????

        device_t* dev =  kcalloc(1, sizeof(device_t));
        dev->priv = &pair->slave;
        fnode->device = (fs_node_t*)dev;
        

        return fnode;


    }
    return NULL;
}

struct fs_node*  pts_create_node(){

    if(!ptsnode){
        ptsnode = kcalloc(1, sizeof(struct fs_node));
        if(!ptsnode)
            return NULL;
        
        ptsnode->inode = 26; //idk
        sprintf(ptsnode->name, "devpts");
        ptsnode->flags = FS_DIRECTORY;
        
        ptsnode->readdir = (readdir_type_t)pts_readdir;
        ptsnode->finddir = (finddir_type_t)pts_finddir;

        masterslave_table = kcalloc(MAX_PTS_COUNT, sizeof(struct ptypair*));
        if(!masterslave_table)
            return NULL;
    }



    //so some init about the pts node
    return ptsnode;
}