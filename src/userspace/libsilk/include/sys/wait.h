#ifndef __WAIT_H__
#define __WAIT_H__


#ifndef WIFEXITED
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#endif

#ifndef WIFSTOPPED
#define WIFSTOPPED(status)  (((status) & 0xff) == 0x7f)
#endif

#ifndef WSTOPSIG
#define WSTOPSIG(status)    (((status) >> 8) & 0xff)
#endif

#ifndef WTERMSIG
#define WTERMSIG(status)    ((status) & 0x7f)
#endif



struct rusage{

};

typedef int pid_t;

#endif