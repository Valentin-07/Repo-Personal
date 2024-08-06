#include <__es.h>

t_log* logger;
t_config* config;
char* nombre_interfaz;


// Variables de configuración.

t_tipo_interfaz tipo_interfaz;
int tiempo_unidad_trabajo;
char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
char* path_base_dialfs;
int block_size;
int block_count;
int retraso_compactacion;



// Conexiones.

int server__kernel;
int server__memoria;

bool ac__kernel, ac__memoria;


// Operaciones.

sem_t sem__operaciones[2];

sem_t sem__memoria[2];


// DialFS - Archivo.


// DialFS - Bitmap.

t_bitmap* bitmap;


// DialFS - Bloques.

t_bloques* bloques;


// Funciones.

int iniciar_modulo(int arg_count, char* nombre, char* cfg_path) {
    if (nombre == NULL) return 0;

    nombre_interfaz = nombre;

    char* nombre_log = string_duplicate(nombre_interfaz);
    string_append(&nombre_log, ".log");

    logger = log_create(nombre_log, nombre_interfaz, true, LOG_LEVEL_INFO);

    free(nombre_log);

    if (arg_count != required_arg_count) {
        log_error(logger, "%s fallo inicialización. (%s)", nombre_interfaz, (arg_count < required_arg_count) ? "Falta un argumento" : "Se exceden los argumentos");
        free(nombre_interfaz);
        log_destroy(logger);
        return 0;
    }

    config = config_create(cfg_path);

    if (config == NULL) {
        log_error(logger, "%s fallo inicialización. (No existe el path de configuración \"%s\")", nombre_interfaz, cfg_path);
        free(nombre_interfaz);
        log_destroy(logger);
        return 0;
    }

    log_info(logger, "%s inicializado.", nombre_interfaz);

    return 1;
}

int configurar_modulo() {
    char* cad_t = config_get_string_value(config, "TIPO_INTERFAZ");
    if ((tipo_interfaz = (!strcmp(cad_t, "GENERICA") ? io_generica : !strcmp(cad_t, "STDIN") ? io_stdin : !strcmp(cad_t, "STDOUT") ? io_stdout : !strcmp(cad_t, "DIALFS") ? io_dialfs : -1)) == (t_tipo_interfaz) -1) {
        log_error(logger, "Tipo interfaz invalido. (%s)", cad_t);
        return 0;
    }

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    switch (tipo_interfaz) {
        case io_generica:
            tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
            break;
        case io_stdin:
            tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
            break;
        case io_stdout:
            tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
            break;
        case io_dialfs:
            path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
            block_size = config_get_int_value(config, "BLOCK_SIZE");
            block_count = config_get_int_value(config, "BLOCK_COUNT");
            retraso_compactacion = config_get_int_value(config, "RETRASO_COMPACTACION");

            int tam_bloques = bloques__tam(), tam_bitmap = bitmap__tam();

            if (block_size * block_count != tam_bloques || block_count != tam_bitmap * 8) {
                if (!bloques__crear() || !bitmap__crear()) return 0; else {
                    printf("Se creó un bitmap de %d bits (%d bytes).\n", block_count / 8, block_count);
                    printf("Se crearon %d bloques de %d bytes.\n", block_count, block_size);
                }
            } else {
                printf("Bitmap (%d bytes, %d bits) y Bloques (%d bytes) preparados.\n", tam_bitmap, tam_bitmap * 8, tam_bloques);
            }

            bitmap__abrir();
            bloques__abrir();

            bitmap__imprimir();

            break;
    }


    for (int i = 0; i < 2; i++) sem_init(&sem__operaciones[i], 0, 0);
    for (int i = 0; i < 2; i++) sem_init(&sem__memoria[i], 0, 0);

    return 1;
}

int terminar_modulo(int success) {
    log_info(logger, "%s terminado.", nombre_interfaz);

    if (success) {
        for (int i = 0; i < 2; i++) sem_destroy(&sem__operaciones[i]);
        for (int i = 0; i < 2; i++) sem_destroy(&sem__memoria[i]);

        if (tipo_interfaz == io_dialfs) {
            bitmap__cerrar();
            bloques__cerrar();
        }
    }

    config_destroy(config);
    log_destroy(logger);
    return 0;
}
