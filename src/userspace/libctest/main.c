#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

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

int main(int  argc, char* argv[]){
    
    int err;
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t socklen = sizeof(server_addr);

        // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(128);
    server_addr.sin_addr = 0;

    err = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){
        return 1;
    }

    unsigned char buffer[64];
    
    //simple echo server
    while(1){

        err = recvfrom(sockfd, &buffer, 64, 0, (struct sockaddr*)&server_addr, &socklen);
        if(err < 0){
            return 1;
        }

        sendto(sockfd, buffer, err, 0, (struct sockaddr*)&server_addr, socklen);
    }

    


}   