#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

bool directory = false;
char *filepath;

char *copy_str(char *str)
{
	size_t len = strlen(str) + 1;
	char *copy = malloc(len);
	if (copy == NULL)
	{
		return NULL;
	}
	memcpy(copy, str, len);
	return copy;
}

void *handle_client(void *client_socket)
{

	int client_fd = *(int *)client_socket;

	uint32_t size = 0;
	char buffer[1024];

	uint32_t bytesRead = 0;
	bytesRead = read(client_fd, buffer, 1024);
	if (bytesRead < 1)
	{
		printf("Read failed: %s\n", strerror(errno));
		return (void *)1;
	}
	printf("Buffer: %s\n", buffer);

	char *buffer_copy = copy_str(buffer);
	char *token = strtok_r(buffer_copy, "\r\n", &buffer_copy);
	char *req_ln = token;
	char *user_agent_ln;
	char *content_length_ln;
	while (token != NULL)
	{
		if (strstr(token, "User-Agent: ") != 0)
		{
			user_agent_ln = token;
		}
		if (strstr(token, "Content-Length: ") != 0)
		{
			content_length_ln = token;
		}
		token = strtok_r(NULL, "\r\n", &buffer_copy);
	}
	free(buffer_copy);
	char *req_body = strstr(buffer, "\r\n\r\n");
	if (req_body != NULL)
	{
		req_body += 4;
	}
	if (req_ln == NULL)
	{
		printf("Invalid request\n");
		return (void *)1;
	}
	char *method = strtok(req_ln, " ");
	printf("Method: %s\n", method);
	char *path = strtok(NULL, " ");
	printf("Path: %s\n", path);
	char *version = strtok(NULL, " ");
	printf("Version: %s\n", version);
	char *response = "HTTP/1.1 200 OK\r\n\r\n";
	char *not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
	if (method == NULL || path == NULL || version == NULL)
	{
		printf("Invalid request\n");
		return (void *)1;
	}
	char *data;
	if ((data = strstr(path, "/echo/")) != NULL)
	{
		char *content = data + strlen("/echo/");
		printf("Content: %s\n", content);
		char *responseFormat = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s";
		char response[1024];
		sprintf(response, responseFormat, strlen(content), content);
		send(client_fd, response, strlen(response), 0);
	}
	else if ((data = strstr(path, "/user-agent")) != NULL)
	{
		char *content = user_agent_ln + strlen("User-Agent: ");
		printf("CONTENT: %s\n", content);
		char response[1024];
		char responseFormat[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s";
		sprintf(response, responseFormat, strlen(content), content);
		send(client_fd, response, strlen(response), 0);
	}
	else
	{

		if (directory == true)
		{
			char *fl_name = strstr(path, "/files/");
			char *filename = fl_name + strlen("/files/");
			printf("Filename: %s\n", filename);
			printf("Filepath: %s\n", filepath);
			char *fullpath = malloc(strlen(filepath) + strlen(filename) + 1);
			if (fullpath == NULL)
			{
				printf("Memory allocation failed: %s\n", strerror(errno));
				return (void *)1;
			}
			strcpy(fullpath, filepath);
			strcat(fullpath, filename);

			if (strcmp(method, "GET") == 0)
			{

				FILE *file = fopen(fullpath, "rb");
				if (file == NULL)
				{
					printf("File not found\n");
					write(client_fd, not_found, strlen(not_found));
				}
				else
				{
					char *filebuffer;
					fseek(file, 0, SEEK_END);
					int64_t filesize = ftell(file);
					if (filesize < 0)
					{
						printf("Filesize failed: %s\n", strerror(errno));
						return (void *)1;
					}
					fseek(file, 0, SEEK_SET);
					filebuffer = malloc(filesize + 1);
					if (filebuffer == NULL)
					{

						printf("Memory allocation failed on file buffer: %s\nFilesize: %d", strerror(errno), filesize);
						return (void *)1;
					}
					size_t bytesRead = fread(filebuffer, 1, filesize, file);
					if (bytesRead < filesize)
					{
						printf("Read failed: %s\n", strerror(errno));
						return (void *)1;
					}
					buffer[bytesRead] = '\0';
					char responseFormat[] = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s";
					size_t responseSize = strlen(responseFormat) + filesize;
					char *response = malloc(responseSize + 1);
					if (response == NULL)
					{
						printf("Memory allocation failed on response buffer: %s\n", strerror(errno));
						return (void *)1;
					}
					snprintf(response, responseSize + 1, responseFormat, filesize, filebuffer);
					send(client_fd, response, responseSize, 0);
					free(filebuffer);
					free(response);
					fclose(file);
				}
			}
			else if (strcmp(method, "POST") == 0)
			{
				uint32_t content_length = atoi(content_length_ln + strlen("Content-Length: "));
				printf("Post data: %s\n", req_body);
				printf("Content length: %d\n", content_length);
				printf("Data length: %d\n", strlen(req_body));
				printf("Fullpath: %s\n", fullpath);
				FILE *file = fopen(fullpath, "w");
				fprintf(file, "%s", req_body);
				fclose(file);
				char *response_created = "HTTP/1.1 201 CREATED\r\n\r\n";
				write(client_fd, response_created, strlen(response_created));
			}
			else
			{
				write(client_fd, not_found, strlen(not_found));
			}
		}
		else
		{
			if (strcmp(path, "/") == 0)
			{
				write(client_fd, response, strlen(response));
			}
			else
			{
				write(client_fd, not_found, strlen(not_found));
			}
		}
	}

	// Close the socket
	// Returns 0 on success, -1 on
	close(client_fd);
	free(client_socket);
	return (void *)0;
}

int main(int argc, char **argv)
{

	// Disable output buffering
	setbuf(stdout, NULL);

	printf("argc: %d\n", argc);
	for (int i = 0; i < argc; i++)
	{
		printf("argv[%d]: %s\n", i, argv[i]);
		if (strcmp(argv[i], "--directory") == 0)
		{
			directory = true;
			filepath = argv[i + 1];
			break;
		}
	}

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

	int server_fd, client_addr_len;
	int client_fd;
	struct sockaddr_in client_addr;

	// Create a socket
	// AF_INET: Address Family for IPv4
	// SOCK_STREAM: Provides sequenced, reliable, two-way, connection-based byte streams
	// 0: Protocol value for Internet Protocol (IP)
	// Returns a file descriptor for the socket (a non-negative integer)
	// On error, -1 is returned

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	// setsockopt() is used to set the socket options
	// SO_REUSEPORT: Allows multiple sockets to be bound to the same port
	// &reuse: Pointer to the value of the option
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}

	// create a sockaddr_in struct for the server
	// sin_family: Address family (AF_INET)
	// sin_port: Port number (in network byte order) / htons converts the port number to network byte order
	// sin_addr: IP address (INADDR_ANY: Accept any incoming messages) / htonl converts the IP address to network byte order

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(4221),
		.sin_addr = {htonl(INADDR_ANY)},
	};

	// Bind the socket to the address and port number specified in serv_addr
	// Returns 0 on success, -1 on error
	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	// Listen for incoming connections
	// connection_backlog: Maximum length of the queue of pending connections
	// Returns 0 on success, -1 on error

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0)
	{
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	uint16_t threadCounter = 0;

	printf("Waiting for a client to connect...\n");

	// Accept a connection
	// client_addr: Address of the client
	// client_addr_len: Length of the client address

	client_addr_len = sizeof(client_addr);

	// Returns a file descriptor for the accepted socket (a non-negative integer)
	// On error, -1 is returned
	for (;;)
	{
		int *pclient_fd = malloc(sizeof(int));
		if (pclient_fd == NULL)
		{
			printf("Memory allocation failed: %s \n", strerror(errno));
			return 1;
		}
		if ((*pclient_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
		{
			printf("Accept failed: %s \n", strerror(errno));
			free(pclient_fd);
			return 1;
		}
		printf("Client connected\n");
		pthread_t t;
		pthread_create(&t, NULL, handle_client, pclient_fd);
		pthread_detach(t);
	}

	// print the client socket structure

	return 0;
}
