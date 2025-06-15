#include <stdio.h>
#include <unistd.h>


int daemonize(){

    pid_t pid;

    // Step 1: Fork the parent process
    if ((pid = fork()) < 0) {
        return -1; // Fork failed
    } else if (pid != 0) {
        exit(0); // Parent process exits
    }

    // Step 2: Create a new session
    if (setsid() < 0) {
        return -1; // Failed to create a new session
    }

    // Step 3: Change working directory to root
    if (chdir("/") < 0) {
        return -1; // Failed to change directory
    }

    // Step 4: Reset file permissions
    umask(0);

    // Step 5: Close standard file descriptors
    close(FILENO_STDIN);
    close(FILENO_STDOUT);
    close(FILENO_STDERR);

    // Step 6: Redirect standard input, output, and error to /dev/null
    open("/dev/null", O_RDONLY); // Redirect standard input
    open("/dev/null", O_WRONLY); // Redirect standard output
    open("/dev/null", O_WRONLY); // Redirect standard error

    return 0; // Daemon initialized successfully
}



#include <stdio.h>
#include <setjmp.h>


jmp_buf buf;


// int main() {
//     int ret = setjmp(buf);
//     if (ret == 0) {
//         printf("First time through setjmp, ret = %d\n", ret);
//         test();  // longjmp from here is safe
//     } else {
//         printf("Returned via longjmp, ret = %d\n", ret);
//     }
//     return 0;
// }





int main(int  argc, char* argv[]){
    
    if(daemonize() < -1){
        printf("Failed to daemonize");
        return 1;
    }
    

    int fd = open("/dev/tty1", O_RDWR | O_NOCTTY);
    if(fd < 0){
        return 1;
    }

    while(1){
        write(fd, "Fly me to the moon\n", 20);
    }

}   