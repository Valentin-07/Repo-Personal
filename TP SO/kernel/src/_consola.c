#include <__kernel.h>

t_list* extraer_script(char* script) {
    char* path = string_from_format("%s%s", path_scripts, script);

    FILE* f = fopen(path, "r");
    if (f == NULL) {
        log_error(logger, "No se pudo encontrar el script. (%s)", path);
        free(path);
        return NULL;
    }

    t_list* comandos = list_create();

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, f)) != -1) { // inv
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }

        char *comando = string_duplicate(line);

        list_add(comandos, comando);
    }

    free(line);

    fclose(f);

    free(path);

    return comandos;
}

void ejecutar_script(char* scr) {
    t_list* comandos = extraer_script(scr);

    int tam = list_size(comandos);

    for (int i = 0; i < tam; i++) {
        char* comando = list_get(comandos, i);

        char** arr = string_split(comando, " ");

        if (!strcmp(arr[0], "EJECUTAR_SCRIPT") && string_array_size(arr) == 2) {
            ejecutar_script(arr[1]);
        } else if (!strcmp(arr[0], "TERMINAR") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            buffer_send(buffer_create(terminar), server__cpu_dispatch);
            buffer_send(buffer_create(terminar), server__cpu_interrupt);
            buffer_send(buffer_create(terminar), server__memoria);
            list_destroy_and_destroy_elements(interfaces, (void (*) (void*)) interfaz__desconectar);
            ac__consola = false;
            ac__pl__largo_plazo = false;
            ac__pl__corto_plazo = false;
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "INICIAR_PROCESO") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            proceso__iniciar(arr[1]);
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "FINALIZAR_PROCESO") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            proceso__finalizar(atoi(arr[1]));
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "INICIAR_PLANIFICACION") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            if (!planificacion_activa) {
                if (interrupcion_pausada) sem_post(&sem__interrupcion);
                planificacion_activa = true;
            }
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "DETENER_PLANIFICACION") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            if (planificacion_activa) planificacion_activa = false;
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "MULTIPROGRAMACION") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            grado_multiprogramacion = atoi(arr[1]);
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "PROCESO_ESTADO") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            listas_estado();
            pthread_mutex_unlock(&mx__procesos);
        }

        string_array_destroy(arr);
    }

    list_destroy_and_destroy_elements(comandos, free);
}



void* consola() {
    for (int i = 0; i < 4; i++) sem_wait(&sem__consola);

    ac__consola = ac__cpu_dispatch && ac__cpu_interrupt && ac__memoria && ac__interfaces;

    if (ac__consola) log_info(logger, "Consola interactiva habilitada.");

    char* input;

    while (ac__consola) {
        input = readline("");
        char** arr = string_split(input, " ");

        if (!strcmp(arr[0], "EJECUTAR_SCRIPT") && string_array_size(arr) == 2) {
            ejecutar_script(arr[1]);
        } else if (!strcmp(arr[0], "TERMINAR") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            buffer_send(buffer_create(terminar), server__cpu_dispatch);
            buffer_send(buffer_create(terminar), server__cpu_interrupt);
            buffer_send(buffer_create(terminar), server__memoria);
            list_destroy_and_destroy_elements(interfaces, (void (*) (void*)) interfaz__desconectar);
            ac__consola = false;
            ac__pl__largo_plazo = false;
            ac__pl__corto_plazo = false;
            pthread_mutex_unlock(&mx__procesos);
            sem_post(&sem__pl__largo_plazo);
            sem_post(&sem__pl__corto_plazo);
            sem_post(&sem__interrupcion);
            planificacion_activa = true;
        } else if (!strcmp(arr[0], "INICIAR_PROCESO") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            proceso__iniciar(arr[1]);
            pthread_mutex_unlock(&mx__procesos);
            sem_post(&sem__pl__largo_plazo);
        } else if (!strcmp(arr[0], "FINALIZAR_PROCESO") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            proceso__finalizar(atoi(arr[1]));
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "INICIAR_PLANIFICACION") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            if (!planificacion_activa) {
                if (interrupcion_pausada) sem_post(&sem__interrupcion);
                for (int i = 0; i < list_size(interfaces); i++) {
                    t_interfaz* interfaz = list_get(interfaces, i);
                    if (interfaz -> operacion_terminada == true) sem_post(&(interfaz -> semaforo_operacion));
                }
                planificacion_activa = true;
            }
            pthread_mutex_unlock(&mx__procesos);
            sem_post(&sem__pl__largo_plazo);
        } else if (!strcmp(arr[0], "DETENER_PLANIFICACION") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            if (planificacion_activa) planificacion_activa = false;
            pthread_mutex_unlock(&mx__procesos);
        } else if (!strcmp(arr[0], "MULTIPROGRAMACION") && string_array_size(arr) == 2) {
            pthread_mutex_lock(&mx__procesos);
            grado_multiprogramacion = atoi(arr[1]);
            pthread_mutex_unlock(&mx__procesos);
            sem_post(&sem__pl__largo_plazo);
        } else if (!strcmp(arr[0], "PROCESO_ESTADO") && string_array_size(arr) == 1) {
            pthread_mutex_lock(&mx__procesos);
            listas_estado();
            pthread_mutex_unlock(&mx__procesos);
        }

        string_array_destroy(arr);
        free(input);
    }
    
    return NULL;
}
