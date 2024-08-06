#include <__kernel.h>

char* cadena_salida[5] = {"SUCCESS", "INVALID_RESOURCE", "INVALID_INTERFACE", "OUT_OF_MEMORY", "INTERRUPTED_BY_USER"};
char* cadena_estado[7] = {"NEW", "READY", "READY", "EXEC", "BLOCKED", "BLOCKED", "EXIT"};


t_pcb* pcb__crear() {
    t_pcb* pcb = malloc(sizeof(t_pcb));

    pcb -> identificador = asignacion_pid++;

    pcb -> estado = NEW;

    pcb -> registros_cpu . PC = 0;
    for (int i = 0; i < 4; i++) {
        pcb -> registros_cpu . _8[i] = 0;
        pcb -> registros_cpu . _32[i] = 0;
    }
    pcb -> registros_cpu . SI = 0;
    pcb -> registros_cpu . DI = 0;

    pcb -> recursos_asignados = calloc(tam_array_recursos, sizeof(int));

    pcb -> planificacion_cpu . numero_ejecucion = 0;
    pcb -> planificacion_cpu . tiempo_ejecucion = NULL;
    pcb -> planificacion_cpu . quantum_restante = (int64_t) quantum;
    pcb -> planificacion_cpu . salida = -1;

    pcb -> buffer_es = NULL;
    
    return pcb;
}

void pcb__destruir(t_pcb* pcb) {
    for (int i = 0; i < tam_array_recursos; i++) {
        while (pcb -> recursos_asignados[i] > 0) {
            pcb -> recursos_asignados[i]--;
            recurso__liberar(i);
        }
    }
    free(pcb -> recursos_asignados);
    free(pcb);
}

void pcb__cambiar(t_pcb* pcb, int _estado) {
    int estado_anterior = pcb -> estado;
    pcb -> estado = _estado;
    if (estado_anterior == NEW && _estado != EXIT) {
        nivel_multiprogramacion++;
    } else if (estado_anterior != NEW && _estado == EXIT) {
        nivel_multiprogramacion--;
    }
    if (estado_anterior != _estado) log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb -> identificador, cadena_estado[estado_anterior], cadena_estado[pcb -> estado]);
}


void proceso__iniciar(char* path) {
    solicitar_almacenamiento_proceso(path);

    sem_wait(&sem__creacion_proceso[0]);

    bool validacion = validar_realizacion();
    
    sem_post(&sem__creacion_proceso[1]);

    if (validacion) {
        t_pcb* pcb = pcb__crear();
        list_add(procesos, pcb);
        list_add(lista__new, pcb);
        log_info(logger, "Se crea el proceso %d en NEW", pcb -> identificador);
    } else {
        log_error(logger, "No se pudo crear proceso %d.", asignacion_pid);
    }    
}

void proceso__finalizar(int pid) {
    bool pcb__buscar(t_pcb* _pcb) {
        return _pcb -> identificador == pid;
    }

    t_pcb* pcb = list_find(procesos, (bool (*) (void*)) pcb__buscar);
    if (pcb) {
        switch (pcb -> estado) {
            case NEW:
                list_remove_element(lista__new, pcb);
                pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
                pcb__cambiar(pcb, EXIT);
                list_add(lista__exit, pcb);
                sem_post(&sem__pl__largo_plazo);
                break;
            case READY:
                list_remove_element(lista__ready[0], pcb);
                pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
                pcb__cambiar(pcb, EXIT);
                list_add(lista__exit, pcb);
                sem_post(&sem__pl__largo_plazo);
                break;
            case READY_Q:
                list_remove_element(lista__ready[1], pcb);
                pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
                pcb__cambiar(pcb, EXIT);
                list_add(lista__exit, pcb);
                sem_post(&sem__pl__largo_plazo);
                break;
            case EXEC:
                enviar_interrupcion(interrupcion__terminacion);
                break;
            case BLOCKED_IO:
                for (int i = 0; i < list_size(interfaces); i++) {
                    t_interfaz* interfaz = list_get(interfaces, i);
                    if (list_remove_element(interfaz -> lista__blocked, pcb)) {
                        pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
                        pcb__cambiar(pcb, EXIT);
                        list_add(lista__exit, pcb);
                        sem_post(&sem__pl__largo_plazo);
                        break;
                    }
                }
                break;
            case BLOCKED_RES:
                for (int i = 0; i < tam_array_recursos; i++) {
                    if (list_remove_element(recursos_sistema[i] . lista__blocked, pcb)) {
                        recursos_sistema[i] . cantidad++;
                        pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
                        pcb__cambiar(pcb, EXIT);
                        list_add(lista__exit, pcb);
                        sem_post(&sem__pl__largo_plazo);
                        break;
                    }
                }
                break;
            case EXIT:
                break;
        }
    } else {
        log_error(logger, "No se encuentra proceso con identificador %d.", pid);
    }
}


char* cadena_procesos_estado(t_list* lista_estado) {
    int tam = list_size(lista_estado);
    if (tam == 0) return NULL;

    char* string = string_new();

    for (int i = 0; i < tam; i++) {
        string_append(&string, i != 0 ? ", " : "");
        t_pcb* pcb = list_get(lista_estado, i);
        char* pid_s = string_itoa(pcb -> identificador);
        string_append(&string, pid_s);
        free(pid_s);
    }

    return string;
}

void imprimir_estado(t_list* lista_estado, char* estado) {
    if (!list_is_empty(lista_estado)) {
        char* pids = cadena_procesos_estado(lista_estado);
        log_info(logger, "Cola %s - [%s]", estado, pids);
        free(pids);
    }
}

void listas_estado() {
    imprimir_estado(lista__new, "New");
    imprimir_estado(lista__ready[0], "Ready");
    if (lista__ready[1] != NULL) imprimir_estado(lista__ready[1], "Ready Prioridad");
    imprimir_estado(lista__exec, "Exec");

    for (int i = 0; i < list_size(interfaces); i++) {
        t_interfaz* interfaz = list_get(interfaces, i);
        char* str = string_from_format("Blocked (%s)", interfaz -> nombre);
        imprimir_estado(interfaz -> lista__blocked, str);
        free(str);
    }

    for (int i = 0; i < tam_array_recursos; i++) {
        char* str = string_from_format("Blocked (%s)", recursos_sistema[i] . nombre);
        imprimir_estado(recursos_sistema[i] . lista__blocked, str);
        free(str);
    }

    imprimir_estado(lista__exit, "Exit");
}


void proceso__priorizar(t_pcb* pcb) {
    if (pcb -> planificacion_cpu . quantum_restante > 0) {
        pcb__cambiar(pcb, READY_Q);
        list_add(lista__ready[1], pcb);
        imprimir_estado(lista__ready[1], "Ready Prioridad");
    } else {
        pcb__cambiar(pcb, READY);
        pcb -> planificacion_cpu . quantum_restante = (int64_t) quantum;
        list_add(lista__ready[0], pcb);
        imprimir_estado(lista__ready[0], "Ready");
    }
}

void proceso__preparar(t_pcb* pcb) {
    pcb__cambiar(pcb, READY);
    pcb -> planificacion_cpu . quantum_restante = (int64_t) quantum;
    list_add(lista__ready[0], pcb);
    imprimir_estado(lista__ready[0], "Ready");
}


void* proceso__quantum(t_pcb* pcb) {
    int pid_ex = pcb -> identificador;
    int num_ex = pcb -> planificacion_cpu . numero_ejecucion;
    usleep(pcb -> planificacion_cpu . quantum_restante * 1000);

    pthread_mutex_lock(&mx__procesos);
    if (!list_is_empty(lista__exec)) {
        t_pcb* pcb2 = list_get(lista__exec, 0);
        if (pcb2 -> identificador == pid_ex && num_ex == pcb2 -> planificacion_cpu . numero_ejecucion) enviar_interrupcion(interrupcion__timer);
    }
    pthread_mutex_unlock(&mx__procesos);

    return NULL;
}


void proceso__ejecutar(t_pcb* pcb) {
    pcb__cambiar(pcb, EXEC);
    list_add(lista__exec, pcb);

    enviar_contexto(pcb);

    pcb -> planificacion_cpu . tiempo_ejecucion = temporal_create();
}

void proceso__interrumpir(t_pcb* pcb) {
    t_buffer* buffer = buffer_receive(server__cpu_dispatch);
    buffer_unpack(buffer, &(pcb -> registros_cpu . PC), sizeof(uint32_t));
    for (int i = 0; i < 4; i++) buffer_unpack(buffer, &(pcb -> registros_cpu . _8[i]), sizeof(uint8_t));
    for (int i = 0; i < 4; i++) buffer_unpack(buffer, &(pcb -> registros_cpu . _32[i]), sizeof(uint32_t));
    buffer_unpack(buffer, &(pcb -> registros_cpu . SI), sizeof(uint32_t));
    buffer_unpack(buffer, &(pcb -> registros_cpu . DI), sizeof(uint32_t));

    t_interrupcion tipo_interrupcion; buffer_unpack(buffer, &tipo_interrupcion, sizeof(t_interrupcion));

    temporal_stop(pcb -> planificacion_cpu . tiempo_ejecucion);

    switch (tipo_interrupcion) {
        case interrupcion__out_of_memory:
            pcb -> planificacion_cpu . salida = OUT_OF_MEMORY;
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb__cambiar(pcb, EXIT);
            list_add(lista__exit, pcb);
            sem_post(&sem__pl__largo_plazo);
            sem_post(&sem__pl__corto_plazo);
            break;
        case interrupcion__salida:
            pcb -> planificacion_cpu . salida = SUCCESS;
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb__cambiar(pcb, EXIT);
            list_add(lista__exit, pcb);
            sem_post(&sem__pl__largo_plazo);
            sem_post(&sem__pl__corto_plazo);
            break;
        case interrupcion__terminacion:
            pcb -> planificacion_cpu . salida = INTERRUPTED_BY_USER;
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb__cambiar(pcb, EXIT);
            list_add(lista__exit, pcb);
            sem_post(&sem__pl__largo_plazo);
            sem_post(&sem__pl__corto_plazo);
            break;
        case interrupcion__timer:
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            log_info(logger, "PID: %d - Desalojado por fin de Quantum", pcb -> identificador);
            pcb -> planificacion_cpu . numero_ejecucion++;
            desalojar_proceso(pcb);
            sem_post(&sem__pl__corto_plazo);
            break;
        case interrupcion__wait:
            proceso__solicitar_recurso(pcb, buffer);
            break;
        case interrupcion__signal:
            proceso__liberar_recurso(pcb, buffer);
            break;
        case interrupcion__operacion_io:
            proceso__operacion_io(pcb, buffer);
            sem_post(&sem__pl__corto_plazo);
            break;
    }

    buffer_destroy(buffer);
}

void proceso__destruir(t_pcb* pcb) {
    solicitar_liberacion_proceso(pcb -> identificador);

    sem_wait(&sem__eliminacion_proceso[0]);

    log_info(logger, "Finaliza el proceso %d - Motivo: %s", pcb -> identificador, cadena_salida[pcb -> planificacion_cpu . salida]);
    list_remove_element(procesos, pcb);
    pcb__destruir(pcb);

    sem_post(&sem__eliminacion_proceso[1]);
}
