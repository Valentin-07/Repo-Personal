#include <__memoria.h>

t_log* logger;
t_config* config;


// Variables de configuración.

char* ip_escucha;
char* puerto_escucha;
int tam_memoria;
int tam_pagina;
char* path_instrucciones;
int retardo_respuesta;


// Conexiones.

int client__cpu;
int client__kernel;
int local__memoria;

bool ac__cpu, ac__kernel, ac__interfaces;

sem_t sem__sec_hilos[2];


// Gestion de espacio de usuario.

void* espacio_usuario;
char* bitmap_marcos;
int cantidad_bits;

t_bitarray* bitarray_marcos;


// Gestión de imagenes de procesos.

t_list* imagenes_procesos;


// Gestión de interfaces.

t_list* interfaces;

sem_t sem__interfaces;


// Funciones.

int iniciar_modulo(int arg_count, char* cfg_path) {
    logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);

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
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    if (tam_memoria % tam_pagina != 0) {
        log_error(logger, "Paginación mal dividida. (Restan %d)", tam_memoria % tam_pagina);
        return 0;
    }
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    
    for (int i = 0; i < 2; i++) sem_init(&sem__sec_hilos[i], 0, 0);

    // Gestión de espacio de usuario.
    espacio_usuario = malloc(tam_memoria);
    bitmap_marcos = malloc(tam_memoria / tam_pagina);
    memset(bitmap_marcos, 0, tam_memoria / tam_pagina);
    bitarray_marcos = bitarray_create_with_mode(bitmap_marcos, tam_memoria / tam_pagina, LSB_FIRST);
    cantidad_bits = bitarray_get_max_bit(bitarray_marcos) / 8;

    // Gestión de imagenes de procesos.
    imagenes_procesos = list_create();

    // Gestión de interfaces.
    sem_init(&sem__interfaces, 0, 0);

    return 1;
}

int terminar_modulo(int success) {
    log_info(logger, "%s terminado.", module_name);

    if (success) {
        for (int i = 0; i < 2; i++) sem_destroy(&sem__sec_hilos[i]);

        // Gestión de espacio de usuario.
        free(espacio_usuario);
        bitarray_destroy(bitarray_marcos);
        free(bitmap_marcos);

        // Gestión de imagenes de procesos.
        list_destroy_and_destroy_elements(imagenes_procesos, (void (*) (void*)) imagen_proceso__destruir);

        // Gestión de interfaces.
        sem_destroy(&sem__interfaces);
    }

    
    config_destroy(config);
    log_destroy(logger);
    return 0;
}
