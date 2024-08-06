#include <__cpu.h>

t_log* logger;
t_config* config;


// Variables de configuración.

char* ip_escucha;
char* puerto_escucha_dispatch;
char* puerto_escucha_interrupt;
char* ip_memoria;
char* puerto_memoria;
int cantidad_entradas_tlb;
t_algoritmo_tlb algoritmo_tlb;

int tam_pagina; // Extra de memoria.


// Conexiones.

int local__cpu_dispatch;
int client__kernel_dispatch;
int local__cpu_interrupt;
int client__kernel_interrupt;
int server__memoria;

bool ac__kernel__dispatch, ac__kernel__interrupt, ac__memoria;


// Procesamiento.

bool ac_procesamiento;

int pid;
t_modo_ejecucion modo_ejecucion;
t_controlador_de_interrupciones controlador_de_interrupciones;

void* registro[11];
t_tlb tlb;

sem_t sem__instruccion[2];
sem_t sem__marco[2];
sem_t sem__memoria[2];

sem_t sem__ciclo;


// Ciclo de instrucción.

pthread_mutex_t mx__contexto;


// Funciones.

int iniciar_modulo(int arg_count, char* cfg_path) {
    logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);

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
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    cantidad_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    char* cad_tlb = config_get_string_value(config, "ALGORITMO_TLB");
    if ((algoritmo_tlb = (!strcmp(cad_tlb, "FIFO") ? FIFO : !strcmp(cad_tlb, "LRU") ? LRU : -1)) == (t_algoritmo_tlb) -1) {
        log_error(logger, "Algoritmo TLB invalido. (%s)", cad_tlb);
        return 0;
    }

    // Procesamiento.
    for (int i = 0; i < 2; i++) sem_init(&sem__instruccion[i], 0, 0);
    for (int i = 0; i < 2; i++) sem_init(&sem__marco[i], 0, 0);
    for (int i = 0; i < 2; i++) sem_init(&sem__memoria[i], 0, 0);
    
    // Ciclo de instrucción.
    sem_init(&sem__ciclo, 0, 0);
    pthread_mutex_init(&mx__contexto, NULL);

    return 1;
}

int terminar_modulo(int success) {
    log_info(logger, "%s terminado.", module_name);

    if (success) {
        // Procesamiento.
        for (int i = 0; i < 2; i++) sem_destroy(&sem__instruccion[i]);
        for (int i = 0; i < 2; i++) sem_destroy(&sem__marco[i]);
        for (int i = 0; i < 2; i++) sem_destroy(&sem__memoria[i]);
        
        // Ciclo de instrucción.
        sem_destroy(&sem__ciclo);
        pthread_mutex_destroy(&mx__contexto);
    }

    config_destroy(config);
    log_destroy(logger);
    return 0;
}
