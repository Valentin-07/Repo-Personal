#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/bufferlib.h>
#include <utils/operaciones_io.h>
#include <utils/interrupciones.h>

#define module_name "Kernel"
#define required_arg_count 2

extern t_log* logger;
extern t_config* config;

int iniciar_modulo(int, char*);
int configurar_modulo();
int terminar_modulo(int);


// Estructuras.

typedef enum {FIFO, RR, VRR} t_algoritmo_planificacion;

typedef struct {
    char* nombre;
    int socket;
    bool operacion_terminada;
    sem_t semaforo_operacion;
    sem_t semaforo_validacion[2];
    sem_t semaforo_recepcion;
    t_list* lista__blocked;
} t_interfaz;

typedef struct {
    char* nombre;
    int cantidad;
    t_list* lista__blocked;
} t_recurso;

typedef struct {
    int identificador;
    enum {NEW, READY, READY_Q, EXEC, BLOCKED_IO, BLOCKED_RES, EXIT} estado;
    struct {
        uint32_t PC;
        uint8_t _8[4];
        uint32_t _32[4];
        uint32_t SI;
        uint32_t DI;
    } registros_cpu;
    int* recursos_asignados;
    struct {
        int numero_ejecucion;
        t_temporal* tiempo_ejecucion;
        int64_t quantum_restante;
        enum {SUCCESS, INVALID_RESOURCE, INVALID_INTERFACE, OUT_OF_MEMORY, INTERRUPTED_BY_USER} salida;
    } planificacion_cpu;
    t_buffer* buffer_es;
} t_pcb;


// Variables de configuración.

extern char*                        ip_escucha;
extern char*                        puerto_escucha;
extern char*                        ip_memoria;
extern char*                        puerto_memoria;
extern char*                        ip_cpu;
extern char*                        puerto_cpu_dispatch;
extern char*                        puerto_cpu_interrupt;
extern t_algoritmo_planificacion    algoritmo_planificacion;
extern int                          quantum;
extern int                          tam_array_recursos;
extern int                          grado_multiprogramacion;

extern char*                        path_scripts;


// Conexiones.

extern int server__cpu_dispatch;
extern int server__cpu_interrupt;
extern int server__memoria;
extern int local__kernel;

extern bool ac__cpu_dispatch, ac__cpu_interrupt, ac__memoria, ac__interfaces;

void* cx__cpu_dispatch();
void* cx__cpu_interrupt();
void* cx__memoria();
void* cx__interfaces();


// Consola.

extern bool ac__consola;

extern sem_t sem__consola;

void* consola();


// Planificación.

extern bool ac__pl__largo_plazo, ac__pl__corto_plazo, interrupcion_pausada;

extern sem_t sem__pl__largo_plazo, sem__pl__corto_plazo, sem__interrupcion;

extern void (*planificador) ();

extern void (*aceptar_proceso) (t_pcb*);
extern void (*desalojar_proceso) (t_pcb*);
extern void (*desbloquear_proceso) (t_pcb*);

extern int nivel_multiprogramacion;

extern bool planificacion_activa;

void first_in_first_out();
void round_robin();
void virtual_round_robin();

void* pl__largo_plazo();
void* pl__corto_plazo();


// Comunicación con CPU.

void enviar_interrupcion(t_interrupcion);
void enviar_contexto(t_pcb*);


// Comunicación con Interfaz.

bool validar_operacion(t_interfaz*, t_operacion_io);


// Comunicación con Memoria.

void solicitar_almacenamiento_proceso(char*);
void solicitar_liberacion_proceso(int);
bool validar_realizacion();


// Gestión de interfaces.

extern t_list* interfaces;

extern sem_t sem__interfaces;

t_interfaz* interfaz__conectar(int);
void interfaz__desconectar(t_interfaz*);

t_interfaz* buscar_interfaz(char*);

void proceso__operacion_io(t_pcb*, t_buffer*);

void interfaz__completar_operacion(t_interfaz*);


// Gestión de procesos.

extern int asignacion_pid;

extern t_list* procesos;

extern t_list* lista__new;
extern t_list* lista__ready[2];
extern t_list* lista__exec;
extern t_list* lista__exit;

extern pthread_mutex_t mx__procesos;
extern sem_t sem__creacion_proceso[2];
extern sem_t sem__eliminacion_proceso[2];

t_pcb* pcb__crear();
void pcb__destruir(t_pcb*);
void pcb__cambiar(t_pcb*, int);

void listas_estado();

void proceso__iniciar(char*);
void proceso__finalizar(int);

void proceso__priorizar(t_pcb*);
void proceso__preparar(t_pcb*);

void* proceso__quantum(t_pcb*);

void proceso__ejecutar(t_pcb*);

void proceso__interrumpir(t_pcb*);

void proceso__destruir(t_pcb*);


// Gestión de recursos.

extern t_recurso* recursos_sistema;

void iniciar_recursos();
void finalizar_recursos();

int recurso__buscar(char*);

bool recurso__asignar(int);
void recurso__liberar(int);

void proceso__solicitar_recurso(t_pcb*, t_buffer*);
void proceso__liberar_recurso(t_pcb*, t_buffer*);


#endif