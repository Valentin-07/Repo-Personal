#include <__kernel.h>


void iniciar_recursos() {
    char** nombres = config_get_array_value(config, "RECURSOS");
    char** instancias = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    tam_array_recursos = string_array_size(nombres);
    recursos_sistema = malloc((size_t) ((int) sizeof(t_recurso) * tam_array_recursos));

    for (int i = 0; i < tam_array_recursos; i++) {
        recursos_sistema[i] . nombre = string_duplicate(nombres[i]);
        recursos_sistema[i] . cantidad = atoi(instancias[i]);
        recursos_sistema[i] . lista__blocked = list_create();
    }

    string_array_destroy(nombres);
    string_array_destroy(instancias);
}

void finalizar_recursos() {
    for (int i = 0; i < tam_array_recursos; i++) {
        free(recursos_sistema[i] . nombre);
        list_destroy(recursos_sistema[i] . lista__blocked);
    }

    free(recursos_sistema);
}


int recurso__buscar(char* nombre) {
    int index = -1;
    for (int i = 0; i < tam_array_recursos; i++) {
        if (!strcmp(recursos_sistema[i] . nombre, nombre)) {
            index = i;
            break;
        }
    }
    return index;
}


bool recurso__asignar(int index) {
    recursos_sistema[index] . cantidad--;
    return recursos_sistema[index] . cantidad >= 0;
}

void recurso__liberar(int index) {
    recursos_sistema[index] . cantidad++;
    if (recursos_sistema[index] . cantidad <= 0) {
        t_pcb* pcb = list_remove(recursos_sistema[index] . lista__blocked, 0);
        pcb -> recursos_asignados[index]++;
        desbloquear_proceso(pcb);
        sem_post(&sem__pl__corto_plazo);
    }
}


void proceso__solicitar_recurso(t_pcb* pcb, t_buffer* buffer) {
    char* nombre_recurso = buffer_unpack_string(buffer);
    int index_recurso = recurso__buscar(nombre_recurso);
    free(nombre_recurso);

    if (index_recurso != -1) {
        if (recurso__asignar(index_recurso)) {
            temporal_resume(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb -> recursos_asignados[index_recurso]++;
            list_add(lista__exec, pcb);
            enviar_contexto(pcb);
        } else {
            pcb -> planificacion_cpu . quantum_restante -= temporal_gettime(pcb -> planificacion_cpu . tiempo_ejecucion);
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);

            pcb__cambiar(pcb, BLOCKED_RES);
            list_add(recursos_sistema[index_recurso] . lista__blocked, pcb);
            
            log_info(logger, "PID: %d - Bloqueado por: %s", pcb -> identificador, recursos_sistema[index_recurso] . nombre);

            pcb -> planificacion_cpu . numero_ejecucion++;

            sem_post(&sem__pl__corto_plazo);
        }
    } else {
        temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
        pcb -> planificacion_cpu . salida = INVALID_RESOURCE;
        pcb__cambiar(pcb, EXIT);
        list_add(lista__exit, pcb);
        sem_post(&sem__pl__largo_plazo);
        sem_post(&sem__pl__corto_plazo);
    }
}

void proceso__liberar_recurso(t_pcb* pcb, t_buffer* buffer) {
    char* nombre_recurso = buffer_unpack_string(buffer);
    int index_recurso = recurso__buscar(nombre_recurso);
    free(nombre_recurso);

    if (index_recurso != -1) {
        recurso__liberar(index_recurso);
        if (pcb -> recursos_asignados[index_recurso] > 0) pcb -> recursos_asignados[index_recurso]--;
        temporal_resume(pcb -> planificacion_cpu . tiempo_ejecucion);
        list_add(lista__exec, pcb);
        enviar_contexto(pcb);
    } else {
        temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
        pcb -> planificacion_cpu . salida = INVALID_RESOURCE;
        pcb__cambiar(pcb, EXIT);
        list_add(lista__exit, pcb);
        sem_post(&sem__pl__largo_plazo);
        sem_post(&sem__pl__corto_plazo);
    }
}
