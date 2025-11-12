// C stdlib
#include <assert.h>      // assert
#include <cstddef>
#include <errno.h>       // errno
#include <stdint.h>      // uint8_t, uint32_t
#include <stdio.h>       // printf
#include <string.h>      // memcpy

// POSIX / system (socket API, inet helpers, read/write, poll)
#include <sys/types.h>   // ssize_t
#include <unistd.h>      // read, write, close
#include <arpa/inet.h>   // htons, htonl, ntohs, inet_ntop
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // socket, bind, listen, accept, setsockopt
#include <poll.h>        // poll, struct pollfd

// C++ stdlib
#include <vector>        // std::vector

// local
#include "core/sys.h" // msg, msg_error, die, fd_set_nb, append_buffer, consume_buffer
#include "core/sys_server.h" // next_timer_ms, process_timers
#include "net/netio.h" // Connection, handle_read, handle_write
#include "storage/commands.h" // server_data


// Handle the client connection
static int32_t handle_accept(int listen_fd) {
    //accept the connection
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (conn_fd < 0) { 
        msg_error("accept() error");
        return -1;
    }
    
    // get the client ip address, remember that IP is in little endian
    // eg: 192.168.1.100 is 0xc0a80164 in little endian
    // uint8_t byte0 = ip & 0xFF;           // 0xC0 = 192
    // uint8_t byte1 = (ip >> 8) & 0xFF;    // 0xA8 = 168
    // uint8_t byte2 = (ip >> 16) & 0xFF;   // 0x01 = 1
    // uint8_t byte3 = (ip >> 24) & 0xFF;   // 0x0A = 10
    uint32_t ip = client_addr.sin_addr.s_addr;
    fprintf(stderr, "[server] accepted connection from %u.%u.%u.%u:%u\n", 
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, (ip >> 24), // moving 8 bits bcuz uint32_t is 4 bytes
        ntohs(client_addr.sin_port));

    // set the connection to non-blocking
    fd_set_nb(conn_fd);
    
    // create a new connection
    Connection *conn = new Connection();
    conn->socket_fd = conn_fd;
    conn->want_read = true;
    conn->last_activity_ms = get_current_time_ms();
    dlist_insert_before(&server_data.idle_conn_list, &conn->idle_node);

    // Put the connection into the map and check if inserted correctly
    if (server_data.fd2conn.size() <= (size_t)conn->socket_fd) { server_data.fd2conn.resize(conn->socket_fd + 1); }
    assert(!server_data.fd2conn[conn->socket_fd]);
    server_data.fd2conn[conn->socket_fd] = conn;

    return 0;
}

/**
 * Main function
 * 
 * Initializes a TCP server that listens on port 8080 for incoming client connections.
 * - Creates a non-blocking listening socket bound to 0.0.0.0:8080.
 * - Uses poll() to multiplex I/O across multiple client connections.
 * - Accepts new connections and tracks them using a vector indexed by file descriptor.
 * - Handles readable and writable events for each client socket.
 * - Cleans up connections on error or when marked for closure.
 * 
 * This function runs an infinite event loop and only exits on fatal error.
 * 
 * Return 0 on successful execution.
*/
int main() {

    // initialize the connection timeout list
    dlist_init(&server_data.idle_conn_list);

    // the listening socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { die("socket()"); }
    msg("[server] socket created");

    int val = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set the port to reusable when the program is restarted

    // declare the socket connection
    struct sockaddr_in addr ={};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);  //port
    addr.sin_addr.s_addr = htonl(0); //wildcard ip 0.0.0.0    INADDR_ANY; // listen to all addresses

    // bind
    int rv = bind(listen_fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); } 
    msg("[server] bind successful on 0.0.0.0:8080");

    // set the listen fd to nonblocking mode
    fd_set_nb(listen_fd);

    // listen
    rv = listen(listen_fd, SOMAXCONN);
    if (rv) {die("listen()");} 
    msg("[server] listen successful on 0.0.0.0:8080");

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true) {
        // prepare the arguments for the poll()
        poll_args.clear();

        // put the listening socket into the poll_args in the first position
    struct pollfd pfd = {listen_fd, POLLIN, 0};
        poll_args.push_back(pfd);

        // the rest are the connection sockets
        for (Connection *conn : server_data.fd2conn) {
            if (!conn) { continue; }

            // always poll() for errors
            struct pollfd pfd = {conn->socket_fd, POLLERR, 0};
            if (conn->want_read) { pfd.events |= POLLIN; } // if we want to read, add POLLIN to the events
            if (conn->want_write) { pfd.events |= POLLOUT; } // if we want to write, add POLLOUT to the events
            poll_args.push_back(pfd);
        }

        // poll() for events, polling for readiness, -1 means wait forever
        int32_t timeout_ms = next_timer_ms();
        rv = poll(poll_args.data(), (nfds_t)poll_args.size(), timeout_ms);
        if (rv < 0) { 
            if (errno == EINTR) { continue; }
            die("poll()"); 
        }
        
        // handle the listening socket, poll_args[0] is the listening socket
        // revents is the events that happened on the socket
        if (poll_args[0].revents) {
            handle_accept(listen_fd);
        }

        // handle the client connections sockets
        for (size_t i = 1; i < poll_args.size(); i++) { // skipping the first, as we put it there
            uint32_t ready = poll_args[i].revents;
            if (ready == 0) { continue; }

            // get the connection from the server_data map
            Connection *conn = server_data.fd2conn[poll_args[i].fd];
            
            // Update the Idle timer and the list
            conn->last_activity_ms = get_current_time_ms();
            dlist_detach(&conn->idle_node);
            dlist_insert_before(&server_data.idle_conn_list, &conn->idle_node);
            
            // Handle the read, write, and error events
            if ((ready & POLLIN) && conn->want_read)  { handle_read(conn); }
            if ((ready & POLLOUT) && conn->want_write) { handle_write(conn); }
            if ((ready & POLLERR) || conn->want_close) { handle_destroy(conn); }
        }
        // for each connection socket
        process_timers();
    }
    // for event loop
    return 0;
}