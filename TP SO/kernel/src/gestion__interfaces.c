#include <__kernel.h>

t_interfaz* interfaz__conectar(int socket) {
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));

    t_buffer* buffer = buffer_receive(socket);
    size_t len; buffer_unpack(buffer, &len, sizeof(size_t));
    interfaz -> nombre = malloc(len);
    buffer_unpack(buffer, interfaz -> nombre, len);
    interfaz -> socket = socket;
    buffer_destroy(buffer);
    for (int i = 0; i < 2; i++) sem_init(&(interfaz -> semaforo_validacion[i]), 0, 0);
    sem_init(&(interfaz -> semaforo_operacion), 0, 0);
    sem_init(&(interfaz -> semaforo_recepcion), 0, 0);

    return interfaz;
}

void interfaz__desconectar(t_interfaz* interfaz) {
    buffer_send(buffer_create(terminar), interfaz -> socket);
}


t_interfaz* buscar_interfaz(char* nombre) {
    bool _nombre(t_interfaz* _tci) {
        return !strcmp(_tci -> nombre, nombre);
    }

    return list_find(interfaces, (bool (*) (void*)) _nombre);
}




void proceso__operacion_io(t_pcb* pcb, t_buffer* buffer) {
    void generar_io_gen_sleep() {
        int cantidad; buffer_unpack(buffer, &cantidad, sizeof(int)); buffer_pack(pcb -> buffer_es, &cantidad, sizeof(int));
    }

    void generar_io_stdin_read() {
        int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int)); buffer_pack(pcb -> buffer_es, &direccion_fisica, sizeof(int));
        size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t)); buffer_pack(pcb -> buffer_es, &bytes, sizeof(size_t));
    }

    void generar_io_stdout_write() {
        int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int)); buffer_pack(pcb -> buffer_es, &direccion_fisica, sizeof(int));
        size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t)); buffer_pack(pcb -> buffer_es, &bytes, sizeof(size_t));
    }

    void generar_io_dialfs_create() {
        char* nombre_archivo = buffer_unpack_string(buffer);
        buffer_pack_string(pcb -> buffer_es, nombre_archivo);
        free(nombre_archivo);
    }

    void generar_io_dialfs_delete() {
        char* nombre_archivo = buffer_unpack_string(buffer);
        buffer_pack_string(pcb -> buffer_es, nombre_archivo);
        free(nombre_archivo);
    }

    void generar_io_dialfs_truncate() {
        char* nombre_archivo = buffer_unpack_string(buffer);
        buffer_pack_string(pcb -> buffer_es, nombre_archivo);
        free(nombre_archivo);
        size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t)); buffer_pack(pcb -> buffer_es, &bytes, sizeof(size_t));
    }

    void generar_io_dialfs_write() {
        char* nombre_archivo = buffer_unpack_string(buffer);
        buffer_pack_string(pcb -> buffer_es, nombre_archivo);
        free(nombre_archivo);
        size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t)); buffer_pack(pcb -> buffer_es, &bytes, sizeof(size_t));
        int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int)); buffer_pack(pcb -> buffer_es, &direccion_fisica, sizeof(int));
        int puntero; buffer_unpack(buffer, &puntero, sizeof(int)); buffer_pack(pcb -> buffer_es, &puntero, sizeof(int));
    }

    void generar_io_dialfs_read() {
        char* nombre_archivo = buffer_unpack_string(buffer);
        buffer_pack_string(pcb -> buffer_es, nombre_archivo);
        free(nombre_archivo);
        size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t)); buffer_pack(pcb -> buffer_es, &bytes, sizeof(size_t));
        int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int)); buffer_pack(pcb -> buffer_es, &direccion_fisica, sizeof(int));
        int puntero; buffer_unpack(buffer, &puntero, sizeof(int)); buffer_pack(pcb -> buffer_es, &puntero, sizeof(int));
    }

    char* nombre_interfaz = buffer_unpack_string(buffer);

    t_interfaz* interfaz = buscar_interfaz(nombre_interfaz);

    free(nombre_interfaz);
    
    if (!interfaz) {
        temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
        pcb -> planificacion_cpu . salida = INVALID_INTERFACE;
        pcb__cambiar(pcb, EXIT);
        list_add(lista__exit, pcb);
        sem_post(&sem__pl__largo_plazo);
    } else {
        t_operacion_io tipo_operacion; buffer_unpack(buffer, &tipo_operacion, sizeof(t_operacion_io));
        if (!validar_operacion(interfaz, tipo_operacion)) {
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb -> planificacion_cpu . salida = INVALID_INTERFACE;
            pcb__cambiar(pcb, EXIT);
            list_add(lista__exit, pcb);
            sem_post(&sem__pl__largo_plazo);
        } else {
            pcb -> buffer_es = buffer_create(kernel__operacion__interfaz);
            buffer_pack(pcb -> buffer_es, &(pcb -> identificador), sizeof(t_operacion_io));
            buffer_pack(pcb -> buffer_es, &tipo_operacion, sizeof(t_operacion_io));
            switch (tipo_operacion) {
                case gen_sleep:
                    generar_io_gen_sleep();
                    break;
                case stdin_read:
                    generar_io_stdin_read();
                    break;
                case stdout_write:
                    generar_io_stdout_write();
                    break;
                case dialfs_create:
                    generar_io_dialfs_create();
                    break;
                case dialfs_delete:
                    generar_io_dialfs_delete();
                    break;
                case dialfs_truncate:
                    generar_io_dialfs_truncate();
                    break;
                case dialfs_write:
                    generar_io_dialfs_write();
                    break;
                case dialfs_read:
                    generar_io_dialfs_read();
                    break;
            }

            pcb -> planificacion_cpu . quantum_restante -= temporal_gettime(pcb -> planificacion_cpu . tiempo_ejecucion);
            temporal_destroy(pcb -> planificacion_cpu . tiempo_ejecucion);
            pcb -> planificacion_cpu . numero_ejecucion++;

            pcb__cambiar(pcb, BLOCKED_IO);
            list_add(interfaz -> lista__blocked, pcb);

            if (list_size(interfaz -> lista__blocked) == 1) {
                buffer_send(pcb -> buffer_es, interfaz -> socket);
                sem_wait(&(interfaz -> semaforo_recepcion));
            }
            
            log_info(logger, "PID: %d - Bloqueado por: %s", pcb -> identificador, interfaz -> nombre);
        }
    }
}


void interfaz__completar_operacion(t_interfaz* interfaz) {
    t_buffer* buffer = buffer_receive(interfaz -> socket);
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    bool resultado; buffer_unpack(buffer, &resultado, sizeof(resultado));
    buffer_destroy(buffer);

    bool _pid(t_pcb* _pcb) {
        return _pcb -> identificador == pid;
    }

    t_pcb* pcb = list_remove_by_condition(interfaz -> lista__blocked, (bool (*) (void*)) _pid);
    
    if (pcb != NULL) {
        if (resultado) {
            desbloquear_proceso(pcb);
            sem_post(&sem__pl__corto_plazo);
        } else {
            pcb -> planificacion_cpu . salida = OUT_OF_MEMORY;
            pcb__cambiar(pcb, EXIT);
            list_add(lista__exit, pcb);
            sem_post(&sem__pl__largo_plazo);
        }
    }

    if (!list_is_empty(interfaz -> lista__blocked)) {
        t_pcb* pcb = list_get(interfaz -> lista__blocked, 0);
        buffer_send(pcb -> buffer_es, interfaz -> socket);
    }
}
