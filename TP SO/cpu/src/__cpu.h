#ifndef CPU_H_
#define CPU_H_

#include <utils/bufferlib.h>
#include <utils/operaciones_io.h>
#include <utils/operaciones_memoria.h>
#include <utils/interrupciones.h>

#define module_name "CPU"
#define required_arg_count 2

extern t_log* logger;
extern t_config* config;

int iniciar_modulo(int, char*);
int configurar_modulo();
int terminar_modulo(int);


// Estructuras.

typedef enum {FIFO, LRU} t_algoritmo_tlb;

typedef enum {PC, AX, BX, CX, DX, EAX, EBX, ECX, EDX, SI, DI} t_indice_registro;

typedef enum {SET, MOV_IN, MOV_OUT, SUM, SUB, JNZ, RESIZE, COPY_STRING, WAIT, SIGNAL, IO_GEN_SLEEP, IO_STDIN_READ, IO_STDOUT_WRITE, IO_FS_CREATE, IO_FS_DELETE, IO_FS_TRUNCATE, IO_FS_WRITE, IO_FS_READ, EXIT} t_instruccion;

typedef enum {usuario, kernel} t_modo_ejecucion;

typedef struct {
    t_interrupcion tipo_interrupcion;
    struct {
        char* nombre;
    } recurso;
    struct {
        char* nombre;
        t_operacion_io tipo_operacion;
        struct {
            int unidades_trabajo;
        } _io_gen_sleep;
        struct {
            int direccion_fisica;
            size_t bytes;
        } _io_stdin_read;
        struct {
            int direccion_fisica;
            size_t bytes;
        } _io_stdout_write;
        struct {
            char* nombre_archivo;
            struct {
                size_t bytes;
            } _truncate;
            struct {
                int direccion_fisica;
                size_t bytes;
                int puntero;
            } _write;
            struct {
                int direccion_fisica;
                size_t bytes;
                int puntero;
            } _read;
        } _io_dialfs;
    } interfaz;
} t_controlador_de_interrupciones;

typedef struct {
    int pid;
    int pagina;
    int marco;
    int utilizacion[2]; // {[AGREGACION (FIFO)], [ACCESO (LRU)]}
} t_bloque_tlb;

typedef struct {
    int tam;
    t_bloque_tlb* bloque;
} t_tlb;


// Variables de configuración.

extern char*            ip_escucha;
extern char*            puerto_escucha_dispatch;
extern char*            puerto_escucha_interrupt;
extern char*            ip_memoria;
extern char*            puerto_memoria;
extern int              cantidad_entradas_tlb;
extern t_algoritmo_tlb  algoritmo_tlb;

extern int              tam_pagina; // Extra de memoria.


// Conexiones.

extern int local__cpu_dispatch;
extern int client__kernel_dispatch;
extern int local__cpu_interrupt;
extern int client__kernel_interrupt;
extern int server__memoria;

extern bool ac__kernel__dispatch, ac__kernel__interrupt, ac__memoria;

void* cx__memoria();
void* cx__kernel__dispatch();
void* cx__kernel__interrupt();


// Procesamiento.

extern bool ac_procesamiento;

extern int pid;
extern t_modo_ejecucion modo_ejecucion;
extern t_controlador_de_interrupciones controlador_de_interrupciones;

extern void* registro[11];
extern t_tlb tlb;

extern sem_t sem__instruccion[2];
extern sem_t sem__marco[2];
extern sem_t sem__memoria[2];


void* procesamiento();


// Ciclo de instrucción.

extern pthread_mutex_t mx__contexto;

extern sem_t sem__ciclo;


void cargar_contexto();

char* busqueda_instruccion();
char** decodificacion_instruccion(char*);
void ejecucion_instruccion(char**);
bool check_interrupt();
void manejar_interrupcion();

void guardar_contexto();


// Comunicación con Memoria.

void solicitar_marco(int);
int recibir_marco();


// Instrucciones.

int instruccion_set(t_indice_registro, int);
int instruccion_mov_in(t_indice_registro, t_indice_registro);
int instruccion_mov_out(t_indice_registro, t_indice_registro);
int instruccion_sum(t_indice_registro, t_indice_registro);
int instruccion_sub(t_indice_registro, t_indice_registro);
int instruccion_jnz(t_indice_registro, int);
int instruccion_resize(int);
int instruccion_copy_string(int);
int instruccion_wait(char*);
int instruccion_signal(char*);
int instruccion_io_gen_sleep(char*, int);
int instruccion_io_stdin_read(char*, t_indice_registro, t_indice_registro);
int instruccion_io_stdout_write(char*, t_indice_registro, t_indice_registro);
int instruccion_io_fs_create(char*, char*);
int instruccion_io_fs_delete(char*, char*);
int instruccion_io_fs_truncate(char*, char*, t_indice_registro);
int instruccion_io_fs_write(char*, char*, t_indice_registro, t_indice_registro, t_indice_registro);
int instruccion_io_fs_read(char*, char*, t_indice_registro, t_indice_registro, t_indice_registro);
void instruccion_exit();


// Memory Management Unit.

int mmu__calculo(int, int);


// Translation Look-aside Buffer.

int tlb__index_reemplazo();
int tlb__actualizar(int, int);

int tlb__apuntar(int);
int tlb__acceder_marco(int);
void tlb__remover_marco(int);


#endif