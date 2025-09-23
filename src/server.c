#include <sys/socket.h>


static void do_something(int conn_fd) {
    char rbuf[64] = {};

    ssize_t n = read (conn_fd, rbuf, sizeof(rbuf) - 1); // can be replaced with recv(), requires additional flags
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("Client says: %s\n", rbuf);

    char wbuf[] = "world"; // sizeof also includes the null terminator
    write(conn_fd, wbuf, strlen(wbuf)); // can be replaced with send(), requires additional flags
}

int fd = socket(AF_INET, SOCK_STREAM, 0);

int val = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set the port to reusable when the program is restarted

struct sockaddr_in addr ={};
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);  //port
addr.sin_addr.s_addr = htonl(0) //wildcard ip 0.0.0.0    INADDR_ANY; // listen to all addresses

// bind
int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
if (rv) { die("bind()"); } 

// listen
rv = listen(fd, SOMAXCONN);
if (rv) {die("listen()");} 

while (true) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t client_addr_len = sizeof(client_addr);

    // Here we pass the struct of sockaddr for client address, along with the length of the struct
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len); 
    if (client_fd < 0) {
        continue; //error handling
    }
    do_something(client_fd); //handle the client connection
    close(client_fd);
}