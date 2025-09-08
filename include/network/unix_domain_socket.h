#ifndef __UNIX_DOMAIN_SOCKET_H__
#define __UNIX_DOMAIN_SOCKET_H__

#include  "pmm.h"
#include  "vmm.h"
#include  "sys.h"

struct sockaddr_un {
    sa_family_t sun_family;               /* AF_UNIX */
    char        sun_path[108];            /* Pathname */
};


struct unix_sock{
    struct sock sk;
    struct sockaddr_un addr;
    struct unix_sock* peer;
    struct socket* other;
};


struct socket* unix_create_socket(int type, int options);



#endif