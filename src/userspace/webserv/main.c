#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>



const int days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

int is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

void unix_to_utc(uint32_t epoch, int* year, int* month, int* day,
                 int* hour, int* minute, int* second) {
    uint32_t days = epoch / 86400;
    uint32_t secs = epoch % 86400;

    *hour = secs / 3600;
    secs %= 3600;
    *minute = secs / 60;
    *second = secs % 60;

    int y = 1970;
    while (1) {
        int days_in_year = is_leap(y) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            y++;
        } else {
            break;
        }
    }

    *year = y;
    int m = 0;
    while (1) {
        int dim = days_in_month[m];
        if (m == 1 && is_leap(y)) dim++; // February leap year

        if (days >= dim) {
            days -= dim;
            m++;
        } else {
            break;
        }
    }

    *month = m + 1;
    *day = days + 1;
}

void format_unix_time(uint32_t epoch, char* buf) {
    int year, month, day, hour, minute, second;
    unix_to_utc(epoch, &year, &month, &day, &hour, &minute, &second);
    sprintf(buf, "%d-%d-%d %d:%d:%d UTC",
            year, month, day, hour, minute, second);
}



int main(int argc, char* argv[]){
 

    setvbuf(stdout, NULL, _IONBF, 0); //kodummun buffering i
    int err;
    
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t socklen = sizeof(server_addr);

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to get stream socket");
        return 1;
    }

    puts("[+]Retrieved a inet stream socket\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80); //hmmmmmmm
    server_addr.sin_addr = 0; //INANYADDR

    err = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){
        perror("Failed to bind...");
        return 1;
    }
    puts("[+]Bound to 0.0.0.0:80\n");

    listen(sockfd, 1); //only 1 connection
    puts("[+]Adjusted backlog to 1\n");

    
    const char* reply_template = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %u\r\n"
                        "\r\n";
    const char http_page_template[] =
                                "<!DOCTYPE html>"
                                "<html lang=\"en\">"
                                "<head>"
                                "    <meta charset=\"UTF-8\">"
                                "    <title>silkOS status</title>"
                                "</head>"
                                "<body style=\" background: #f9f9f9; color: #222;\">"
                                "    <h1>silkOS</h1>"
                                "    <hr>"
                                "    <p><b>System time:</b> %u:%u:%u (UTC+0)</p>"
                                "    <p><b>System date:</b> %u/%u/%u</p>"
                                "    <hr>"
                                "    <pre>"
                                "Server: silkOS test HTTP server\n"
                                "Status: OK\n"
                                "Connections: %u\n"
                                "    </pre>"
                                "</body>"
                                "</html>";




    int clientfd;
    while((clientfd = accept(sockfd, (struct sockaddr*)&server_addr, &socklen)) > 0){
        printf(
            "[+]Accepted a new connection %u.%u.%u.%u:%u\n",
            (server_addr.sin_addr >> 0) & 0xff,
            (server_addr.sin_addr >> 8) & 0xff,
            (server_addr.sin_addr >> 16) & 0xff,
            (server_addr.sin_addr >> 24) & 0xff,
            ntohs(server_addr.sin_port)
        );


        //get the current time
        
        uint32_t epoch = time(NULL);
        int year, month, day, hour, minute, second;
        unix_to_utc(epoch, &year, &month, &day, &hour, &minute, &second);
        
        size_t pagelen = 0;
        char lbuff[768];
        sprintf(lbuff, http_page_template, hour, minute, second, day, month, year, ntohs(server_addr.sin_port));
        pagelen = strlen(lbuff);
        
        sprintf(lbuff, reply_template, pagelen); //actually.. oh nvm
    
        write(clientfd, lbuff, strlen(lbuff));
    
        sprintf(lbuff, http_page_template, hour, minute, second, day, month, year, ntohs(server_addr.sin_port));
        write(clientfd, lbuff, pagelen);
        sleep(1);
        close(clientfd);
    }
    return 0;
}