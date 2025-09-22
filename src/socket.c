#include <sys/socket.h>

int fd = socket(AF_INET, SOCK_STREAM, 0);

int val = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set the port to reusable when the program is restarted

struct sockaddr_in addr ={};
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);  //port
addr.sin_addr.s_addr = htonl(0) //wildcard ip 0.0.0.0    INADDR_ANY; // listen to all addresses

int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
if (rv) { die("bind()"); }

listen(fd)
while True:
    conn_fd = accept(fd)
    do_something(conn_fd)
    close(conn_fd)