#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void read_bytes(int fd, uint32_t len, void* buffer){
	uint32_t bytesRead = 0;
	int result;
	while (bytesRead < len){
		result = read(fd, buffer + bytesRead, len - bytesRead);
		if (result < 1){
			printf("Read failed: %s\n", strerror(errno));
			exit(1);
		}
		bytesRead += result;
	}
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	//Uncomment this block to pass the first stage

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	// Create a socket
	// AF_INET: Address Family for IPv4
	// SOCK_STREAM: Provides sequenced, reliable, two-way, connection-based byte streams
	// 0: Protocol value for Internet Protocol (IP)
	// Returns a file descriptor for the socket (a non-negative integer)
	// On error, -1 is returned

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	// setsockopt() is used to set the socket options
	// SO_REUSEPORT: Allows multiple sockets to be bound to the same port
	// &reuse: Pointer to the value of the option
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}
	
	// create a sockaddr_in struct for the server
	// sin_family: Address family (AF_INET)
	// sin_port: Port number (in network byte order) / htons converts the port number to network byte order
	// sin_addr: IP address (INADDR_ANY: Accept any incoming messages) / htonl converts the IP address to network byte order

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	// Bind the socket to the address and port number specified in serv_addr
	// Returns 0 on success, -1 on error
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	// Listen for incoming connections
	// connection_backlog: Maximum length of the queue of pending connections
	// Returns 0 on success, -1 on error


	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");

	// Accept a connection
	// client_addr: Address of the client
	// client_addr_len: Length of the client address

	client_addr_len = sizeof(client_addr);
	
	// Returns a file descriptor for the accepted socket (a non-negative integer)
	// On error, -1 is returned

	accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");
	//print the client socket structure

	
	uint32_t size = 0;
	char* buffer = 0;
	read_bytes(server_fd, sizeof(size), (void*)&size);
	buffer = malloc(size);
	read_bytes(server_fd, size, (void*)buffer);

	printf("Received: %s\n", buffer);
	free(buffer);

	char *response = "HTTP/1.1 200 OK\r\n\r\n";
	ssize_t bytes_written = write(server_fd, response, strlen(response));

	if (bytes_written < 0) {
		printf("Write failed: %s\n", strerror(errno));
		return 1;
	} else if (bytes_written < strlen(response)) {
		printf("Write failed: wrote less than expected\n");
		return 1;
	} else {
		printf("Response sent\n");
	}


	close(server_fd);

	return 0;
}
