#include <filesystems/vfs.h>
#include <sys.h>
#include <fb.h>
#include <g_list.h>
#include <process.h>

typedef struct{
    int pex_id;
    list_t services;
} pex_data_t;

typedef struct{
    uint8_t fd_state;
    void* clientserver; //a string for client or server
    pex_data_t* pex_itself;
} pex_descriptor_specific_t;


enum pex_request_enum{
    PEX_CAPABILITIES = 0,
    PEX_BIND,
    PEX_CONNECT,
};

//shoudl i rename it to binder? because it feels like one :/
#define PEX_SERVICE_MAX_NAME 64
typedef struct{
    char service_name[PEX_SERVICE_MAX_NAME];
    pcb_t* owner_task;
    list_t clients;
    fs_node_t *file;
} pex_service_t;

typedef struct{
    
    pex_service_t* server;
    fs_node_t *file;

} pex_client_t;




int install_pex();
open_type_t  pex_open(fs_node_t* node, uint8_t read, uint8_t write);
close_type_t pex_close( fs_node_t* node );

ioctl_type_t pex_ioctl(fs_node_t* node, unsigned long request, void* argp);
read_type_t pex_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
write_type_t pex_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

