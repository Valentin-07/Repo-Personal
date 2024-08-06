#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/bufferlib.h>
#include <utils/operaciones_memoria.h>

#define module_name "Memoria"
#define required_arg_count 2

extern t_log* logger;
extern t_config* config;

int iniciar_modulo(int, char*);
int configurar_modulo();
int terminar_modulo(int);


// Estructuras.

typedef struct {
    char* nombre;
    int socket;
    sem_t semaforo[2];
} t_interfaz;

typedef struct {
    int pid;
    t_list* instrucciones;
    int tam;
    struct {
        int cantidad_paginas;
        int* marco;
    } tabla_paginas;
} t_imagen_proceso;


// Variables de configuración.

extern char*    ip_escucha;
extern char*    puerto_escucha;
extern int      tam_memoria;
extern int      tam_pagina;
extern char*    path_instrucciones;
extern int      retardo_respuesta;



// Conexiones.

extern int client__cpu;
extern int client__kernel;
extern int local__memoria;

extern bool ac__cpu, ac__kernel, ac__interfaces;

extern sem_t sem__sec_hilos[2];

void* cx__cpu();
void* cx__kernel();
void* cx__interfaces();


// Comunicación con CPU.

void operacion_cpu();


// Comunicación con una Interfaz.

void operacion_interfaz(t_interfaz*);


// Gestion de espacio de usuario.

extern void* espacio_usuario;
extern char* bitmap_marcos;
extern int cantidad_bits;

extern t_bitarray* bitarray_marcos;

bool espacio__ocupar_marco(int);
void espacio__desocupar_marco(int);

void espacio__escribir(int, void*, int, size_t);
void espacio__leer(int, void*, int, size_t);


// Gestión de interfaces.

extern t_list* interfaces;

extern sem_t sem__interfaces;

t_interfaz* interfaz__conectar(int);
void interfaz__desconectar(t_interfaz*);


// Gestion de imagenes de procesos.

extern t_list* imagenes_procesos;

t_imagen_proceso* imagen_proceso__crear(int, char*);
void imagen_proceso__destruir(t_imagen_proceso*);

void almacenar_proceso();
void liberar_proceso();

void enviar_instruccion();

bool imagen_proceso__ajustar(t_imagen_proceso*, int);

bool imagen_proceso__escribir(t_imagen_proceso*, void*, int, size_t);
bool imagen_proceso__leer(t_imagen_proceso*, void*, int, size_t);


// Paginación.

void obtener_marco();

void redimensionar(t_buffer*);

void cpu_escribir(t_buffer*);
void cpu_leer(t_buffer*);
void cpu_copiar(t_buffer*);

void io_escribir(t_interfaz*, t_buffer*);
void io_leer(t_interfaz*, t_buffer*);


#endif