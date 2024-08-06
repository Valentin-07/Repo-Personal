#include <utils/bufferlib.h>

// Socket.

int server_start(char *ip, char *port) {
	int socket_server_fd = -1;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &servinfo) != 0) {
        perror("getaddrinfo");
        return -1;
    }

	for (p = servinfo; p != NULL; p = p -> ai_next) {
		if ((socket_server_fd = socket(p -> ai_family, p->ai_socktype, p -> ai_protocol)) == -1) {
			printf("socket");
			continue;
		}

		int yes = 1;
        if (setsockopt(socket_server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("setsockopt");
            close(socket_server_fd);
            continue;
        }

		if (bind(socket_server_fd, p->ai_addr, p->ai_addrlen) != 0) {
			printf("bind");
			close(socket_server_fd);
			continue;
		}

		break;
	}

    if (p == NULL) {
        fprintf(stderr, "Failed to bind\n");
        return -1;
    }

    if (listen(socket_server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(socket_server_fd);
        return -1;
    }

    freeaddrinfo(servinfo);

	return socket_server_fd;
}

int server_await(int socket_server_fd) {
    int socket_client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    socket_client_fd = accept(socket_server_fd, (void *) &client_addr, &addr_size);
    if (socket_client_fd == -1) {
        perror("accept");
    }
	return socket_client_fd;
}

void server_end(int socket_server_fd) {
    close(socket_server_fd);
}


int client_connect(char *ip, char *port) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &server_info) != 0) {
        perror("getaddrinfo");
        return -1;
    }

	int socket_cliente = socket(server_info -> ai_family, server_info -> ai_socktype, server_info -> ai_protocol);
    if (socket_cliente == -1) {
        perror("socket");
        freeaddrinfo(server_info);
        return -1;
    }

	if (connect(socket_cliente, server_info -> ai_addr, server_info -> ai_addrlen) == -1) {
        perror("connect");
        close(socket_cliente);
        freeaddrinfo(server_info);
        return -1;
    }

	freeaddrinfo(server_info);

	return socket_cliente;
}

void client_disconnect(int socket_cliente) {
	close(socket_cliente);
}


// Buffer.

t_buffer* buffer_create(t_buffer_type type) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer -> type = type;
    buffer -> size = 0;
    buffer -> offset = 0;
    buffer -> stream = NULL;
    return buffer;
}

int buffer_destroy(t_buffer* buffer) {
    if (!buffer) return 0; else {
        free(buffer->stream);
        free(buffer);
        return 1;
    }
}

int buffer_free(t_buffer* buffer) {
    if (!buffer) return 0; else {
        if (buffer -> stream != NULL) {
            free(buffer -> stream);
            buffer -> stream = NULL;
        }
        buffer -> size = 0;
        buffer -> offset = 0;
    }
    return 1;
}

int buffer_increase(t_buffer* buffer, size_t size) {
    void* new_stream = NULL;
    if (buffer -> stream == NULL) new_stream = malloc(size); else new_stream = realloc(buffer -> stream, buffer -> size + size);

    if (new_stream == NULL) return 0;

    buffer -> stream = new_stream;
    buffer -> size += size;

    return 1;
}

int buffer_add(t_buffer* buffer, void* data, size_t size) {
    if (buffer -> offset + size > buffer -> size) return 0;

    memcpy((char*) buffer -> stream + buffer -> offset, data, size);
    buffer -> offset += size;

    return 1;
}

int buffer_pack(t_buffer* buffer, void* data, size_t size) {
    if (!buffer_increase(buffer, size) || !buffer_add(buffer, data, size)) {
        buffer_free(buffer);
        return 0;
    }
    return 1;
}

int buffer_send(t_buffer* buffer, int socket_target) {
    if (buffer == NULL) return 0;

    size_t sz = buffer -> offset == 0 ? sizeof(t_buffer_type) : sizeof(t_buffer_type) + sizeof(size_t) + buffer -> offset;

    void* pack = malloc(sz);

    if (pack == NULL) return 0;

    
    memcpy((char*) pack, &(buffer -> type), sizeof(t_buffer_type));

    if (buffer -> offset != 0) {
        size_t _offset = sizeof(t_buffer_type);
        memcpy((char*) pack + _offset, &(buffer -> offset), sizeof(size_t));
        _offset += sizeof(size_t);
        memcpy((char*) pack + _offset, buffer -> stream, buffer -> offset);
    }

    ssize_t sent = send(socket_target, pack, sz, 0);
    free(pack);

    if (sent == -1) return 0;

    buffer_destroy(buffer);

    return 1;
}


t_buffer_type buffer_scan(int socket) {
    t_buffer_type type;
    if (recv(socket, &type, sizeof(t_buffer_type), MSG_WAITALL) > 0) return type; else {
        close(socket);
        return -1;
    }
}

t_buffer* buffer_receive(int socket) {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer -> type = -1;
    recv(socket, &(buffer -> size), sizeof(size_t), MSG_WAITALL);
    buffer -> stream = malloc(buffer -> size);
    recv(socket, buffer -> stream, buffer -> size, MSG_WAITALL);
    buffer -> offset = 0;
    return buffer;
}

void buffer_unpack(t_buffer* buffer, void* data, size_t size) {
    if (buffer -> offset + size > buffer -> size || data == NULL) return;
    memcpy(data, (char*) buffer -> stream + buffer -> offset, size);
    buffer -> offset += size;
}

void buffer_repurpose(t_buffer* buffer, t_buffer_type type) {
    buffer_free(buffer);
    buffer -> type = type;
}


void buffer_pack_string(t_buffer* buffer, char* str) {
    size_t len = (size_t) (string_length(str) + 1);
    buffer_pack(buffer, &len, sizeof(size_t));
    buffer_pack(buffer, str, len);
}

char* buffer_unpack_string(t_buffer* buffer) {
    size_t len; buffer_unpack(buffer, &len, sizeof(size_t));
    char* str = malloc(len);
    buffer_unpack(buffer, str, len);
    return str;
}
