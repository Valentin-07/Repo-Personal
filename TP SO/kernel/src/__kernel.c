#include <__kernel.h>

t_log* logger;
t_config* config;


// Variables de configuración.

char* ip_escucha;
char* puerto_escucha;
char* ip_memoria;
char* puerto_memoria;
char* ip_cpu;
char* puerto_cpu_dispatch;
char* puerto_cpu_interrupt;
t_algoritmo_planificacion algoritmo_planificacion;
int quantum;
int tam_array_recursos;
int grado_multiprogramacion;

char* path_scripts;


// Conexiones.

bool ac__cpu_dispatch, ac__cpu_interrupt, ac__memoria, ac__interfaces;

int server__cpu_dispatch;
int server__cpu_interrupt;
int server__memoria;
int local__kernel;


// Consola.

bool ac__consola;

sem_t sem__consola;


// Planificación.

bool ac__pl__largo_plazo, ac__pl__corto_plazo, interrupcion_pausada;

sem_t sem__pl__largo_plazo, sem__pl__corto_plazo, sem__interrupcion;

void (*planificador) ();

void (*aceptar_proceso) (t_pcb*);
void (*desalojar_proceso) (t_pcb*);
void (*desbloquear_proceso) (t_pcb*);

int nivel_multiprogramacion;

bool planificacion_activa;


// Gestión de interfaces.

t_list* interfaces;

sem_t sem__interfaces;


// Gestión de procesos.

int asignacion_pid;

t_list* procesos;

t_list* lista__new;
t_list* lista__ready[2];
t_list* lista__exec;
t_list* lista__exit;

pthread_mutex_t mx__procesos;
sem_t sem__creacion_proceso[2];
sem_t sem__eliminacion_proceso[2];


// Gestión de recursos.

t_recurso* recursos_sistema;


// Funciones.

int iniciar_modulo(int arg_count, char* cfg_path) {
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);

    if (arg_count != required_arg_count) {
        log_error(logger, "%s fallo inicialización. (%s)", module_name, arg_count < required_arg_count ? "Falta un argumento" : "Se exceden los argumentos");
        log_destroy(logger);
        return 0;
    }

    config = config_create(cfg_path);

    if (config == NULL) {
        log_error(logger, "%s fallo inicialización. (No existe el path de configuración \"%s\")", module_name, cfg_path);
        log_destroy(logger);
        return 0;
    }

    log_info(logger, "%s inicializado.", module_name);

    return 1;
}

int configurar_modulo() {
    ip_escucha = config_get_string_value(config, "IP_ESCUCHA");
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_value(config, "IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    char* cad_plan = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    if ((algoritmo_planificacion = (!strcmp(cad_plan, "FIFO") ? FIFO : ! strcmp(cad_plan, "RR") ? RR : !strcmp(cad_plan, "VRR") ? VRR: -1)) == (t_algoritmo_planificacion) -1) {
        log_error(logger, "Algoritmo de planificación invalido. (%s)", cad_plan);
        return 0;
    }
    quantum = config_get_int_value(config, "QUANTUM");
    
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    
    // Consola.
    sem_init(&sem__consola, 0, 0);

    // Planificación.
    sem_init(&sem__pl__largo_plazo, 0, 0);
    sem_init(&sem__pl__corto_plazo, 0, 0);
    sem_init(&sem__interrupcion, 0, 0);
    nivel_multiprogramacion = 0;
    interrupcion_pausada = false;
    switch (algoritmo_planificacion) {
        case FIFO:
            planificador = first_in_first_out;
            aceptar_proceso = proceso__preparar;
            desalojar_proceso = proceso__preparar;
            desbloquear_proceso = proceso__preparar;
            break;
        case RR:
            planificador = round_robin;
            aceptar_proceso = proceso__preparar;
            desalojar_proceso = proceso__preparar;
            desbloquear_proceso = proceso__preparar;
            break;
        case VRR:
            planificador = virtual_round_robin;
            aceptar_proceso = proceso__preparar;
            desalojar_proceso = proceso__preparar;
            desbloquear_proceso = proceso__priorizar;
            break;
    }
    
    // Gestión de interfaces.
    sem_init(&sem__interfaces, 0, 0);

    // Gestión de procesos.
    asignacion_pid = 0;
    procesos = list_create();

    pthread_mutex_init(&mx__procesos, NULL);
    for (int i = 0; i < 2; i++) sem_init(&sem__creacion_proceso[i], 0, 0);
    for (int i = 0; i < 2; i++) sem_init(&sem__eliminacion_proceso[i], 0, 0);
    
    // Gestión de recursos.
    iniciar_recursos();

    return 1;
}

int terminar_modulo(int success) {
    log_info(logger, "%s terminado.", module_name);

    if (success) {
        // Consola.
        sem_destroy(&sem__consola);

        // Planificación.
        sem_destroy(&sem__pl__largo_plazo);
        sem_destroy(&sem__pl__corto_plazo);
        sem_destroy(&sem__interrupcion);

        // Gestión de interfaces.
        sem_destroy(&sem__interfaces);

        // Gestión de procesos.
        list_destroy_and_destroy_elements(procesos, (void (*) (void*)) pcb__destruir);

        pthread_mutex_destroy(&mx__procesos);
        for (int i = 0; i < 2; i++) sem_destroy(&sem__creacion_proceso[i]);
        for (int i = 0; i < 2; i++) sem_destroy(&sem__eliminacion_proceso[i]);
        
        // Gestión de recursos.
        finalizar_recursos();
    }

    if (path_scripts != NULL) free(path_scripts);

    config_destroy(config);
    log_destroy(logger);
    return success;
}
