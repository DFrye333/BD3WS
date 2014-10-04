/******************************************************************************
	BD3WS Web Server
	BD3WS.c
******************************************************************************/

#include "BD3WS.h"

/******************************************************************************
	main: Initializes the program and enters the main accept/response loop.
******************************************************************************/
int main(int argc, char **argv)
{
	char buffer[BD3WS_MaxLengthData];
	int client = -1;

	memset(buffer, 0, sizeof(buffer));

	// Server start up.
	initialize(argc, argv);
	//

	// Server main loop.
	while (1)
	{
		// Accept connection request from client or continue looping.
		client = accept_client();
		if (-1 == client)
		{
			continue;
		}
		//

		// Spawn a thread to handle incoming client request or log the resulting error.
		if (0 != pthread_create(&(server.clients[client].thread), NULL, handle_client_request, client))
		{
			sprintf(buffer, "Error creating thread!\n");
			log(buffer, STDERR);
		}
		else
		{
			// Detach thread so that it will free itself after it has finished.
			pthread_detach(server.clients[client].thread);
			//

			client = -1;
		}
		//
	}
	//
}

/******************************************************************************
	initialize: Initializes the program by setting up structs and sockets,
parsing command-line arguments, and listening for client connections.
******************************************************************************/
void initialize(int argc, char** argv)
{
	char buffer[BD3WS_MaxLengthData];

	memset(buffer, 0, sizeof(buffer));

	// Open log file, ensuring that its directory exists beforehand.
	mkdir(BD3WS_LogDirectory, S_IRWXU | S_IRWXG | S_IROTH);
	log_handle = fopen(BD3WS_Log, "w");
	//

	sprintf(buffer, "Initializing...\n");
	log(buffer, STDOUT);

	// Set initial server configuration.
	memset(&(server.hints), 0, sizeof(server.hints));
	server.hints.ai_family = AF_UNSPEC;
	server.hints.ai_socktype = SOCK_STREAM;
	server.hints.ai_flags = AI_PASSIVE;
	//

	// Process command-line arguments.
	process_CLA(argc, argv);
	//

	// Extract server connection information.
	extract_connection_information();
	//

	// Setup server socket and bind it to a port.
	setup_socket();
	//

	// Finished with the server information structure.
	freeaddrinfo(server.info);
	//

	// Initialize client occupany states.
	for (int i = 0; i < BD3WS_MaxNumberClients; ++i)
	{
		server.clients[i].occupied = 0;
	}
	//

	// Ignore broken pipes.
	signal(SIGPIPE, SIG_IGN);
	//

	// Begin listening on the socket.
	listen(server.socket, 3);
	sprintf(buffer, "Listening on %s:%hu\n", server.ip, server.port);
	log(buffer, STDOUT);
	//
}

/******************************************************************************
	finalize: Performs program shutdown tasks before exiting.
******************************************************************************/
void finalize(int exit_code)
{
	char buffer[BD3WS_MaxLengthData];

	memset(buffer, 0, sizeof(buffer));

	sprintf(buffer, "Finalizing...\n");
	log(buffer, STDOUT);

	// Close connection sockets.
	for (int i = 0; i < BD3WS_MaxNumberClients; ++i)
	{
		if (-1 != server.clients[i].socket)
		{
			close(server.clients[i].socket);
		}
	}
	//

	// Close server socket.
	if (-1 != server.socket)
	{
		close(server.socket);
	}
	//

	// Close log file.
	if (NULL != log_handle)
	{
		fclose(log_handle);
	}
	//

	exit(exit_code);
}	

/******************************************************************************
	process_CLA: Parses program command-line arguments.
******************************************************************************/
void process_CLA(int argc, char** argv)
{
	char buffer[BD3WS_MaxLengthData];

	memset(buffer, 0, sizeof(buffer));

	// Return address information for the specified hostname and service.
	if (3 == argc && 0 != getaddrinfo(argv[1], argv[2], &(server.hints), &(server.info)))
	{
		sprintf(buffer, "Cannot get specified address information!\n");
		log(buffer, STDERR);
		finalize(1);
	}
	//

	// Return address information for the default hostname and service.
	else if (3 > argc && 0 != getaddrinfo(BD3WS_DefaultIP, BD3WS_DefaultPort, &(server.hints), &(server.info)))
	{
		sprintf(buffer, "Cannot get default address information!\n");
		log(buffer, STDERR);
		finalize(1);
	}
	//
}

/******************************************************************************
	extract_connection_information: Extracts information pertaining to the 
server-client connection (server IP address and port, etc.).
******************************************************************************/
void extract_connection_information()
{
	struct sockaddr_in* address;

	// Extract address and port information and print it out.
	address = (struct sockaddr_in*)server.info->ai_addr;
	inet_ntop(server.info->ai_family, &(address->sin_addr), server.ip, sizeof(server.ip));
	server.port = ntohs(address->sin_port);
	//
}

/******************************************************************************
	setup_socket: Establishes server socket/port pairing to prepare the server 
to listen for incoming client connections.
******************************************************************************/
void setup_socket()
{
	char buffer[BD3WS_MaxLengthData];

	memset(buffer, 0, sizeof(buffer));

	// Get a server socket descriptor.
	if (0 > (server.socket = socket(server.info->ai_family, server.info->ai_socktype, server.info->ai_protocol)))
	{
		sprintf(buffer, "Invalid server socket descriptor!\n");
		log(buffer, STDERR);
		finalize(1);
	}
	//

	// Set socket options.
	int optval = 1;
	setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	//

	// Associate socket with port.
	if (-1 == bind(server.socket, server.info->ai_addr, server.info->ai_addrlen))
	{
		sprintf(buffer, "Cannot bind to socket!\n");
		log(buffer, STDERR);
		finalize(1);
	}
	//
}

/******************************************************************************
	accept_client: Waits on client requests and sets up server-client 
connections upon receiving them.
******************************************************************************/
int accept_client()
{
	char buffer[BD3WS_MaxLengthData];
	int i = 0;

	memset(buffer, 0, sizeof(buffer));

	// Get a connection socket descriptor.
	for (i = 0; i < BD3WS_MaxNumberClients; ++i)
	{
		// Find vacant client.
		if (0 == server.clients[i].occupied)
		{
			// Secure client structure.
			server.clients[i].occupied = 1;
			//

			// Wait on client connection.
			server.clients[i].address_size = sizeof(server.clients[i].address_storage);
			if (-1 == (server.clients[i].socket = accept(server.socket, (struct sockaddr *)&(server.clients[i].address_storage), &(server.clients[i].address_size))))
			{
				sprintf(buffer, "Invalid connection socket descriptor!\n");
				log(buffer, STDERR);
				return -1;
			}
			//

			// Indicate successful client acceptance and return newly-occupied index.
			sprintf(buffer, "Accepted connection request from client.\n");
			log(buffer, STDOUT);
			return i;
			//
		}
		//
	}
	//

	// Return occupied client's index in server's client array.
	return -1;
	//
}

/******************************************************************************
	handle_client_request: Upon receiving a client request, the server spawns 
a thread. The thread's main task is to execute this function, which parses 
incoming client requests and responds to them accordingly. Before returning, 
connection sockets are closed and client structures are vacated for future 
tasks.
******************************************************************************/
void* handle_client_request(void* client)
{
	char file_path[BD3WS_MaxLengthData];
	char content_type[BD3WS_MaxLengthData];

	memset(file_path, 0, sizeof(file_path));
	memset(content_type, 0, sizeof(content_type));

	// Receive file data request from client.
	parse_client_request((int)client, &file_path, &content_type);
	//

	// Send requested file data to client.
	send_server_response((int)client, &file_path, &content_type);
	//

	// Close connection socket.
	if (-1 != server.clients[(int)client].socket)
	{
		close(server.clients[(int)client].socket);
	}
	//

	// Vacate client.
	server.clients[(int)client].occupied = 0;
	//
}

/******************************************************************************
	parse_client_request: Parses incoming client request to determine an
appropriate server response.
******************************************************************************/
void parse_client_request(int client, char* file_path, char* content_type)
{
	char buffer[BD3WS_MaxLengthData];
	char request[BD3WS_MaxLengthData];
	char line_file[BD3WS_MaxLengthData];
	char line_type[BD3WS_MaxLengthData];
	char* token = NULL;

	memset(buffer, 0, sizeof(buffer));
	memset(request, 0, sizeof(request));

	// Attempt to receive a client request.
	if (-1 != recv(server.clients[client].socket, request, BD3WS_MaxLengthData, 0))
	{
		// Print client request.
		strcat(buffer, "\n===========================================================\n");
		strcat(buffer, "\t\t\tClient Request Header: ");
		strcat(buffer, "\n===========================================================\n");
		sprintf((buffer + strlen(buffer)), "%s\n", request);
		strcat(buffer, "\n===========================================================\n");
		log(buffer, NONE);
		//

		token = strtok(request, "\n");

		// Isolate string containing file path.
		while (NULL != token)
		{
			// Isolate line containing file path.
			if (NULL != strstr(token, "GET"))
			{
				strcpy(line_file, token);
			}
			//

			// Isolate line containing content type.
			else if (NULL != strstr(token, "Accept:"))
			{
				strcpy(line_type, token);
			}
			//

			token = strtok(NULL, "\n");
		}
		//

		// Grab the file path piece of the request.
		token = strtok(line_file, " ");
		token = strtok(NULL, " ");
		//

		// Copy requested file path for output parameter.
		strcpy(file_path, token);
		//

		// Grab the content type piece of the request.
		token = strtok(line_type, " ");
		token = strtok(NULL, " ");
		strcpy(line_type, token);
		token = strtok(line_type, ",");
		//

		// Copy requested content type for output parameter.
		strcpy(content_type, token);
		//
	}
	//
}

/******************************************************************************
	send_server_response: Sends server response to client request. This 
involves checking the requested file for existence and validity, building an 
HTTP response header, filling the response body, and sending it through the 
client connection.
******************************************************************************/
void send_server_response(int client, const char* file_name, const char* content_type)
{
	FILE* file_handle = NULL;
	struct stat file_stat;
	char file_path[BD3WS_MaxLengthData];
	char buffer[BD3WS_MaxLengthData];
	char response_header[BD3WS_MaxLengthData];
	int bytes_read = 0;

	BD3WS_HTTPResponseState response_state;

	memset(buffer, 0, sizeof(buffer));
	memset(response_header, 0, sizeof(response_header));

	// Create full file path.
	strcpy(file_path, BD3WS_PublicDirectory);
	strcat(file_path, file_name);
	//

	stat(file_path, &file_stat);

	// If requested file is a directory.
	if (S_ISDIR(file_stat.st_mode))
	{
		strcat(file_path, "/index.html");
		stat(file_path, &file_stat);
	}
	//

	// Clean the file path.
	clean_file_path(file_path);

	// Open file.
	file_handle = fopen(file_path, "r");
	//

	// Check file existence to create proper response header.
	if (NULL == file_handle)
	{
		char error_buffer[256];
		strerror_r(errno, error_buffer, 256);
		sprintf(buffer, "Cannot serve file: \"%s\"! Details: %s\n", file_path, error_buffer);
		log(buffer, STDERR);
		response_state = NOTFOUND;
	}
	else
	{
		response_state = OK;
	}
	//

	// If HTTP 404 occurs, serve error 404 page.
	if (NOTFOUND == response_state)
	{
		strcpy(file_path, BD3WS_FileHTTP404);
		file_handle = fopen(file_path, "r");
		stat(file_path, &file_stat);
	}
	//

	build_response_header(&file_stat, content_type, response_header, response_state);

	// Send response header to client.
	if (-1 == send(server.clients[client].socket, response_header, strlen(response_header), 0))
	{
		char error_buffer[256];
		strerror_r(errno, error_buffer, 256);
		sprintf(buffer, "Cannot send response header to client! Details: %s\n", error_buffer);
		log(buffer, STDERR);
		fclose(file_handle);
		return;
	}
	//

	// Read file and send it to client.
	while (!feof(file_handle))
	{
		// Read requested file data.
		bytes_read = fread(buffer, 1, BD3WS_MaxLengthData, file_handle);
		//

		// If there are no more bytes to read, quit serving file.
		if (0 >= bytes_read)
		{
			break;
		}
		//

		// Send requested file data to client.
		if (-1 == send(server.clients[client].socket, buffer, BD3WS_MaxLengthData, 0))
		{
			char error_buffer[256];
			strerror_r(errno, error_buffer, 256);
			sprintf(buffer, "Cannot send file data to client! Details: %s\n", error_buffer);
			log(buffer, STDERR);
			fclose(file_handle);
			return;
		}
		//

		memset(buffer, 0, sizeof(buffer));
	}
	//

	fclose(file_handle);

	sprintf(buffer, "File \"%s\" sent successfully!\n", file_path);
	log(buffer, STDOUT);
}

/******************************************************************************
	build_response_header: Constructs the HTTP response header that will be 
sent to a client.
******************************************************************************/
void build_response_header(struct stat* file_stat, const char* content_type, char* response_header, BD3WS_HTTPResponseState response_state)
{
	char buffer[BD3WS_MaxLengthData];

	memset(buffer, 0, sizeof(buffer));

	build_response_header_state(response_header, response_state);

	strcat(response_header, "\n");
	strcat(response_header, "Server: ");
	strcat(response_header, BD3WS_ServerName);
	strcat(response_header, " v");
	strcat(response_header, BD3WS_ServerVersion);
	strcat(response_header, "\n");

	build_response_header_content(file_stat, content_type, response_header);

	strcat(response_header, "\n\n");

	strcat(buffer, "\n===========================================================\n");
	strcat(buffer, "\t\t\tServer Response Header:");
	strcat(buffer, "\n===========================================================\n");
	strcat(buffer, response_header);
	sprintf((buffer + strlen(buffer)), "\n===========================================================\n");
	log(buffer, NONE);
}

/******************************************************************************
	build_response_header_state: Constructs the HTTP status (200 OK, 404 NOT 
FOUND, etc.) portion of the server HTTP response header.
******************************************************************************/
void build_response_header_state(char* response_header, BD3WS_HTTPResponseState response_state)
{
	// Determine HTTP response state of requested file.
	if (OK == response_state)
	{
		strcat(response_header, HTTP_200_OK);
	}
	else if (NOTFOUND == response_state)
	{
		strcat(response_header, HTTP_404_NOTFOUND);
	}
	//
}

/******************************************************************************
	build_response_header_content: Constructs the Content fields of the server 
HTTP response header (Content-Type, Content-Length, etc.).
******************************************************************************/
void build_response_header_content(struct stat* file_stat, const char* content_type, char* response_header)
{
	char file_size[16];

	memset(file_size, 0, sizeof(file_size));

	strcat(response_header, "Content-Type: ");

	// Content-Type: text/plain
	if (0 == strcmp(CONTENT_TEXT_PLAIN, content_type))
	{
		strcat(response_header, CONTENT_TEXT_PLAIN);
		// strcat(response_header, ";charset=UTF-8");
		strcat(response_header, ";charset=Windows-1252");
	}
	//

	// Content-Type: text/html
	else if (0 == strcmp(CONTENT_TEXT_HTML, content_type))
	{
		strcat(response_header, CONTENT_TEXT_HTML);
		// strcat(response_header, ";charset=UTF-8");
		strcat(response_header, ";charset=Windows-1252");
	}
	//

	// Content-Type: text/css
	else if (0 == strcmp(CONTENT_TEXT_CSS, content_type))
	{
		strcat(response_header, CONTENT_TEXT_CSS);
	}
	//

	// Content-Type: image/png
	else if (0 == strcmp(CONTENT_IMAGE_PNG, content_type))
	{
		strcat(response_header, CONTENT_IMAGE_PNG);
	}
	//

	// Content-Type: image/jpeg
	else if (0 == strcmp(CONTENT_IMAGE_JPEG, content_type))
	{
		strcat(response_header, CONTENT_IMAGE_JPEG);
	}
	//

	// Content-Type: image/x-icon
	else if (0 == strcmp(CONTENT_IMAGE_XICON, content_type))
	{
		strcat(response_header, CONTENT_IMAGE_XICON);
	}
	//

	// Content-Type: audio/webm
	else if (0 == strcmp(CONTENT_AUDIO_WEBM, content_type))
	{
		strcat(response_header, CONTENT_AUDIO_WEBM);
	}
	//

	// Content-Tye: audio/ogg
	else if (0 == strcmp(CONTENT_AUDIO_OGG, content_type))
	{
		strcat(response_header, CONTENT_AUDIO_OGG);
	}
	//

	// Content-Type: video/webm
	else if (0 == strcmp(CONTENT_VIDEO_WEBM, content_type))
	{
		strcat(response_header, CONTENT_VIDEO_WEBM);
	}
	//

	// Content-Type: video/ogg
	else if (0 == strcmp(CONTENT_VIDEO_OGG, content_type))
	{
		strcat(response_header, CONTENT_VIDEO_OGG);
	}
	//

	// Content-Type: application/octet-stream
	else if (0 == strcmp(CONTENT_APPLICATION_OCTETSTREAM, content_type))
	{
		strcat(response_header, CONTENT_APPLICATION_OCTETSTREAM);
	}
	//

	// Content-Type: */*
	else
	{
		strcat(response_header, CONTENT_ANY);
	}
	//

	sprintf(file_size, "%d", file_stat->st_size);
	strcat(response_header, "\nContent-Length: ");
	strcat(response_header, file_size);
}

/******************************************************************************
	server_information: Constructs a string describing server program 
meta-data (name, version, etc.).
******************************************************************************/
void server_information(char* information)
{
	strcat(information, BD3WS_ServerName);
	strcat(information, " ");
	strcat(information, BD3WS_ServerVersion);
}

/******************************************************************************
	log: Logs server activity in the system log file.
******************************************************************************/
void log(const char* message, BD3WS_Output output)
{
	// Lock the log mutex.
	pthread_mutex_lock(&mutex_log);
	//

	char log_message[BD3WS_MaxLengthData];

	// Ensure that the log file is open.
	if (NULL == log_handle)
	{
		// Ensure that if the log file is not open, it can be opened (or created, if necessary).
		if (-1 == mkdir(BD3WS_LogDirectory, S_IRWXU | S_IRWXG | S_IROTH))
		{
			// Unlock the log mutex before returning prematurely.
			pthread_mutex_unlock(&mutex_log);
			//

			return;
		}
		//

		log_handle = fopen(BD3WS_Log, "w");
	}
	//

	// Prepend a timestamp to the log message.
	time_t log_time = time(NULL);
	strftime(log_message, BD3WS_MaxLengthData, "%D %T", localtime(&log_time));
	//
	

	// Determine output to write to.
	if (NONE == output)
	{
		// Print timestamp and raw message content only.
		sprintf(log_message, "%s : %s", log_message, message);
		//
	}
	else if (STDOUT == output)
	{
		// Denote message as an informational message.
		sprintf(log_message, "%s (%s | Information): %s", log_message, BD3WS_ServerName, message);
		//

		// Write to stdout.
		fprintf(stdout, "%s", log_message);
		fflush(stdout);
		//
	}
	else if (STDERR == output)
	{
		// Denote message as an error message.
		sprintf(log_message, "%s (%s | Error): %s", log_message, BD3WS_ServerName, message);
		//

		// Write to stdout.
		fprintf(stdout, "%s", log_message);
		fflush(stdout);
		//
	}
	//

	// Write to log file.
	fprintf(log_handle, "%s", log_message);
	fflush(log_handle);
	//

	// Unlock the log mutex.
	pthread_mutex_unlock(&mutex_log);
	//
}

/******************************************************************************
	clean_file_path: Cleans up a given file path in-place. Currently, this
simply means removing multiple successive path separators (forward slashes).
******************************************************************************/
void clean_file_path(char* file_path)
{
	// Get the length of the file path.
	unsigned int length = strlen(file_path);
	///

	// Initialize string iterators to the beginning of the string.
	unsigned int trailing = 0;
	unsigned int leading = 0;
	//

	// Iterate through the string and remove redundant file path separators.
	while (leading < length)
	{
		// If the iterators are offset from each other, redundant separators have been found and the string elements should be shifted.
		if (trailing != leading)
		{
			file_path[trailing] = file_path[leading];
		}
		//

		// If a path separator has been encountered, check for redundant separators immediately following it.
		if (file_path[trailing] == '/')
		{
			// Move the leading iterator to the end of the redundant separator chain.
			while (file_path[leading] == '/')
			{
				++leading;
			}
			//

			// Advance the trailing iterator to the new write-point (directly following the first, non-redundant separator).
			++trailing;
			//
		}
		//

		// If a path separator has not been encountered, continue iteration normally.
		else
		{
			++trailing;
			++leading;
		}
		//
	}
	//

	// Null-terminate the path.
	file_path[trailing] = '\0';
	//
}