#ifndef BUFFERLIB_H_
#define BUFFERLIB_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <readline/readline.h>

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>


// Estructuras.

typedef enum {
    kernel__handshake__cpu_dispatch,            /*  [      KERNEL     ]     -->     [  CPU_DISPATCH   ] */
    kernel__contexto__cpu_dispatch,

    kernel__handshake__cpu_interrupt,           /*  [      KERNEL     ]     -->     [  CPU_INTERRUPT  ] */
    kernel__interrupcion__cpu_interrupt,

    cpu_dispatch__handshake__kernel,            /*  [  CPU_DISPATCH   ]     -->     [      KERNEL     ] */
    cpu_dispatch__contexto__kernel,

    cpu_interrupt__handshake__kernel,           /*  [  CPU_INTERRUPT  ]     -->     [      KERNEL     ] */

    kernel__handshake__memoria,                 /*  [      KERNEL     ]     -->     [     MEMORIA     ] */
    kernel__creacion_de_proceso__memoria,
    kernel__eliminacion_de_proceso__memoria,

    memoria__handshake__kernel,                 /*  [     MEMORIA     ]     -->     [      KERNEL     ] */
    memoria__creacion_de_proceso__kernel,
    memoria__eliminacion_de_proceso__kernel,

    cpu__handshake__memoria,                    /*  [       CPU       ]     -->     [     MEMORIA     ] */
    cpu__instruccion__memoria,
    cpu__traduccion__memoria,
    cpu__operacion__memoria,

    memoria__handshake__cpu,                    /*  [     MEMORIA     ]     -->     [       CPU       ] */
    memoria__instruccion__cpu,
    memoria__traduccion__cpu,
    memoria__operacion__cpu,

    interfaz__handshake__kernel,                /*  [     INTERFAZ    ]     -->     [      KERNEL     ] */
    interfaz__validacion__kernel,
    interfaz__recepcion__kernel,
    interfaz__operacion__kernel,

    kernel__handshake__interfaz,                /*  [      KERNEL     ]     -->     [     INTERFAZ    ] */
    kernel__validacion__interfaz,
    kernel__operacion__interfaz,

    interfaz__handshake__memoria,               /*  [     INTERFAZ    ]     -->     [     MEMORIA     ] */
    interfaz__operacion__memoria,

    memoria__handshake__interfaz,               /*  [     MEMORIA     ]     -->     [     INTERFAZ    ] */
    memoria__operacion__interfaz,
    
	terminar
} t_buffer_type;

typedef struct {
	t_buffer_type type;
	size_t size;
	size_t offset;
	void* stream;
} t_buffer;


// Socket.

/**
 * @brief Inicia un servidor en la dirección IP y puerto especificados.
 * 
 * @param ip La dirección IP a la que se enlaza el servidor.
 * @param port El número de puerto en el que se escucha.
 * @return El descriptor de archivo del socket del servidor, o -1 en caso de fallo.
 */
int server_start(char *ip, char *port);

/**
 * @brief Espera una conexión de cliente en el socket de servidor especificado.
 * 
 * @param socket_server_fd El descriptor de archivo del socket del servidor.
 * @return El descriptor de archivo del socket del cliente, o -1 en caso de fallo.
 */
int server_await(int socket_server_fd);

/**
 * @brief Cierra el socket de servidor especificado.
 * 
 * @param socket_server_fd El descriptor de archivo del socket del servidor a cerrar.
 */
void server_end(int socket_server_fd);


/**
 * @brief Conecta a un servidor con la dirección IP y puerto especificados.
 * 
 * @param ip La dirección IP del servidor.
 * @param port El número de puerto del servidor.
 * @return El descriptor de archivo del socket del cliente, o -1 en caso de fallo.
 */
int client_connect(char *ip, char *port);

/**
 * @brief Desconecta el socket del cliente.
 * 
 * @param socket_cliente El descriptor de archivo del socket del cliente a desconectar.
 */
void client_disconnect(int socket_cliente);


// Buffer.

/**
 * @brief Crea un nuevo buffer con el tipo especificado.
 * 
 * @param type El tipo del buffer.
 * @return Un puntero al buffer recién creado.
 */
t_buffer* buffer_create(t_buffer_type type);

/**
 * @brief Destruye el buffer especificado y libera memoria.
 * 
 * @param buffer Un puntero al buffer a destruir.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_destroy(t_buffer* buffer);

/**
 * @brief Libera los recursos asignados por el buffer.
 * 
 * @param buffer Un puntero al buffer a liberar.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_free(t_buffer* buffer);

/**
 * @brief Aumenta el tamaño del buffer.
 * 
 * @param buffer Un puntero al buffer.
 * @param size El tamaño a incrementar el buffer.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_increase(t_buffer* buffer, size_t size);

/**
 * @brief Agrega datos al buffer.
 * 
 * @param buffer Un puntero al buffer.
 * @param data Un puntero a los datos a agregar.
 * @param size El tamaño de los datos a agregar.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_add(t_buffer* buffer, void* data, size_t size);

/**
 * @brief Empaqueta datos en el buffer.
 * 
 * @param buffer Un puntero al buffer.
 * @param data Un puntero a los datos a empaquetar.
 * @param size El tamaño de los datos a empaquetar.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_pack(t_buffer* buffer, void* data, size_t size);

/**
 * @brief Envía los datos del buffer sobre el socket especificado.
 * 
 * @param buffer Un puntero al buffer que contiene los datos a enviar.
 * @param socket_target El descriptor de archivo del socket de destino.
 * @return 1 en caso de éxito, 0 en caso de fallo.
 */
int buffer_send(t_buffer* buffer, int socket_target);


/**
 * @brief Escanea el tipo de buffer recibido sobre el socket especificado.
 * 
 * @param socket El descriptor de archivo del socket para recibir.
 * @return El tipo del buffer recibido, o -1 en caso de fallo.
 */
t_buffer_type buffer_scan(int socket);

/**
 * @brief Recibe un buffer sobre el socket especificado.
 * 
 * @param socket El descriptor de archivo del socket para recibir.
 * @return Un puntero al buffer recibido.
 */
t_buffer* buffer_receive(int socket);

/**
 * @brief Desempaqueta datos del buffer.
 * 
 * @param buffer Un puntero al buffer.
 * @param data Un puntero al lugar donde almacenar los datos desempaquetados.
 * @param size El tamaño de los datos a desempaquetar.
 */
void buffer_unpack(t_buffer* buffer, void* data, size_t size);

/**
 * @brief Reutiliza el buffer para un nuevo tipo.
 * 
 * @param buffer Un puntero al buffer a reutilizar.
 * @param type El nuevo tipo del buffer.
 */
void buffer_repurpose(t_buffer* buffer, t_buffer_type type);


void buffer_pack_string(t_buffer*, char*);

char* buffer_unpack_string(t_buffer*);


#endif