#include <filesystems/pex.h>

#define PEX_OK 0
#define PEX_FAIL 1

//forward declarations
static int pex_bind(fs_node_t* node, const char* name);
static int pex_connect(fs_node_t* node, const char* name);


/*
well since i have no netwrok interface or capabilities and don't want to implement syscall just for sockets
in order to make it more everything is a file, ioctl interface would be like bind, accept  kind of
syscall. instead or a raw socket it's more like message passing
*/

static uint8_t pex_id = 0;

int install_pex(){

    fb_console_printf("Installing pex device ");
    //create a device and register it through the device manager
    device_t* pex_dev = kcalloc(1, sizeof(device_t));
    pex_dev->unique_id = 16;
    pex_dev->dev_type = DEVICE_CHAR;
    
    pex_dev->name = strdup("pexxxx"); //possibly 256 devices
    sprintf(pex_dev->name, "pex%u", pex_id);

    //fill with the functions
    pex_dev->ioctl = pex_ioctl;
    pex_dev->read  = pex_read;
    pex_dev->write = pex_write;
    pex_dev->open  = pex_open;
    pex_dev->close = pex_close;

    pex_data_t* pex = kcalloc(1, sizeof(pex_data_t));
    pex->pex_id = pex_id;
    pex->services = list_create();
    
    pex_dev->priv = pex;
    dev_register(pex_dev);
    
    fb_console_printf(": pex%u\n", pex_id);

    pex_id++;
    return 0;
}


open_type_t  pex_open(fs_node_t* node, uint8_t read, uint8_t write){

    device_t* pex_dev = node->device;
    pex_data_t* pex = pex_dev->priv;    
    //let's discard read and write thigies,  file_t handles that kid of shit
    pex_descriptor_specific_t* pds = kcalloc(1 ,sizeof(pex_descriptor_specific_t));
    pds->pex_itself = pex_dev;
    pds->clientserver = NULL;
    pds->fd_state = PEX_CAPABILITIES;

    node->device = pds;
    return;
}


close_type_t pex_close( fs_node_t* node ){
    

    pex_descriptor_specific_t* pds = node->device;
    //depending on the state do some shit

    switch (pds->fd_state){

        case PEX_CAPABILITIES:
            //well do nothing maybe deallocate the node?
            kfree(node);
            return; break;

        default: break;
    }

    return;
}





static char pex_capability_str[] = 
                                    "1 BIND\n"
                                    "2 CONNECT\n"
                                    ;

read_type_t pex_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    
    pex_descriptor_specific_t* pds = node->device;
    device_t* pex_dev = pds->pex_itself;
    pex_data_t* pex = pex_dev->priv;


    switch(pds->fd_state){
        case PEX_CAPABILITIES:
            ;
            size_t capability_length = strlen(pex_capability_str);
            if(offset > capability_length){
                return 0; //eof
            }

            size_t data_left = capability_length - offset;
            if(size > data_left){
                size = data_left;
            }

            memcpy(buffer, &pex_capability_str[offset], size);
            return size;
            break;
        
        case PEX_BIND:
            //should check if client sent messages
            fb_console_printf("pex fds state: bind\n");
            break;
        
        case PEX_CONNECT:
            //should check if connected server sent back any messages
            fb_console_printf("pex fds state: connectt\n");
            break;
    }
    
    return 0;
}


write_type_t pex_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){

    device_t* pex_dev = node->device;
    pex_data_t* pex = pex_dev->priv;

    
}



ioctl_type_t pex_ioctl(fs_node_t* node, unsigned long request, void* argp){

    int result = 0;
    switch(request){
        case PEX_CAPABILITIES:

            break;
        
        case PEX_BIND:
            ;
            char* bind_service_name = argp;
            result = pex_bind(node, bind_service_name);
            break;
        
        case PEX_CONNECT:
            ;
            char* connect_service_name = argp;
            result = pex_connect(node, connect_service_name);
            return 0;
            break;
    }

    return result;
}


static int pex_bind(fs_node_t* node, const char* name){
    
    pex_descriptor_specific_t* pds = node->device;
    device_t* pex_dev = pds->pex_itself;
    pex_data_t* pex = pex_dev->priv;

    for(listnode_t* node = pex->services.head; node ; node = node->next){
        pex_service_t* serv = node->val;
        
        if(!strcmp(serv->service_name, name) ){ //duplicate name
            return PEX_FAIL;
        }
    }

    //no duplicate name exits so binded

    pex_service_t* service = kcalloc(1, sizeof(pex_service_t));
    strcpy(service->service_name, name);
    service->owner_task = current_process;
    service->clients = list_create();
    service->file = node;
    list_insert_end( &pex->services, service);

    pds->fd_state = PEX_BIND;
    pds->clientserver = service;

    return PEX_OK;
}

static int pex_connect(fs_node_t* node, const char* name){
    pex_descriptor_specific_t* pds = node->device;
    device_t* pex_dev = pds->pex_itself;
    pex_data_t* pex = pex_dev->priv;

    pex_service_t* service = NULL;
    for(listnode_t* node = pex->services.head; node ; node = node->next){
        pex_service_t* serv = node->val;
        
        if(!strcmp(serv->service_name, name) ){ //duplicate name
            service = serv;
            break;
        }
    }


    if(!service){
        return PEX_FAIL;
    }

    
    pds->fd_state = PEX_CONNECT;
    pex_client_t* client = kcalloc(1, sizeof(pex_client_t));
    pds->clientserver = client;
    client->server = service;
    client->file = node;
    return PEX_OK;
}


