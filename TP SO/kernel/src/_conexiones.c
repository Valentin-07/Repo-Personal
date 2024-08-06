#include <__kernel.h>

void* cx__cpu_dispatch() {
    ac__cpu_dispatch = true;

    if ((server__cpu_dispatch = client_connect(ip_cpu, puerto_cpu_dispatch)) == -1) {
        log_error(logger, "CPU (Dispatch) no disponible.");
        ac__cpu_dispatch = false;
    } else {
        buffer_send(buffer_create(kernel__handshake__cpu_dispatch), server__cpu_dispatch);
        if (buffer_scan(server__cpu_dispatch) != cpu_dispatch__handshake__kernel) {
            log_error(logger, "CPU (Dispatch) invalidada.");
            client_disconnect(server__cpu_dispatch);
            ac__cpu_dispatch = false;
        }
    }

    if (ac__cpu_dispatch) log_info(logger, "Conectado a CPU (Dispatch).");
    
    sem_post(&sem__interfaces);
    sem_post(&sem__consola);
    sem_post(&sem__pl__largo_plazo);
    sem_post(&sem__pl__corto_plazo);

    while (ac__cpu_dispatch) {
        switch ((int) buffer_scan(server__cpu_dispatch)) {
            case -1:
                log_warning(logger, "CPU (Dispatch) se desconecto.");
                ac__cpu_dispatch = false;
                break;
            default:
                log_warning(logger, "CPU (Dispatch) envio operación desconocida.");
                break;
            case cpu_dispatch__contexto__kernel:
                if (!planificacion_activa) {
                    interrupcion_pausada = true;
                    sem_wait(&sem__interrupcion);
                }
                pthread_mutex_lock(&mx__procesos);
                proceso__interrumpir(list_remove(lista__exec, 0));
                interrupcion_pausada = false;
                pthread_mutex_unlock(&mx__procesos);
                break;
            case terminar:
                ac__cpu_dispatch = false;
                break;
        }
    }

    return NULL;
}

void* cx__cpu_interrupt() {
    ac__cpu_interrupt = true;

    if ((server__cpu_interrupt = client_connect(ip_cpu, puerto_cpu_interrupt)) == -1) {
        log_error(logger, "CPU (Interrupt) no disponible.");
        ac__cpu_interrupt = false;
    } else {
        buffer_send(buffer_create(kernel__handshake__cpu_interrupt), server__cpu_interrupt);
        if (buffer_scan(server__cpu_interrupt) != cpu_interrupt__handshake__kernel) {
            log_error(logger, "CPU (Interrupt) invalidada.");
            client_disconnect(server__cpu_interrupt);
            ac__cpu_interrupt = false;
        }
    }

    if (ac__cpu_interrupt) log_info(logger, "Conectado a CPU (Interrupt).");
    
    sem_post(&sem__interfaces);
    sem_post(&sem__consola);
    sem_post(&sem__pl__largo_plazo);
    sem_post(&sem__pl__corto_plazo);

    while (ac__cpu_interrupt) {
        switch ((int) buffer_scan(server__cpu_interrupt)) {
            case -1:
                log_warning(logger, "CPU (Interrupt) se desconecto.");
                ac__cpu_interrupt = false;
                break;
            default:
                log_warning(logger, "CPU (Interrupt) envio operación desconocida.");
                break;
            case terminar:
                ac__cpu_interrupt = false;
                break;
        }
    }

    return NULL;
}

void* cx__memoria() {
    ac__memoria = true;

    if ((server__memoria = client_connect(ip_memoria, puerto_memoria)) == -1) {
        log_error(logger, "Memoria no disponible.");
        ac__memoria = false;
    } else {
        buffer_send(buffer_create(kernel__handshake__memoria), server__memoria);
        if (buffer_scan(server__memoria) != memoria__handshake__kernel) {
            log_error(logger, "Memoria invalidada.");
            client_disconnect(server__memoria);
            ac__memoria = false;
        }
    }

    if (ac__memoria) {
        t_buffer* buffer = buffer_receive(server__memoria);
        path_scripts = buffer_unpack_string(buffer);
        buffer_destroy(buffer);
        log_info(logger, "Conectado a Memoria.");
    }

    sem_post(&sem__interfaces);
    sem_post(&sem__consola);
    sem_post(&sem__pl__largo_plazo);
    sem_post(&sem__pl__corto_plazo);

    while (ac__memoria) {
        switch ((int) buffer_scan(server__memoria)) {
            case -1:
                log_warning(logger, "Memoria se desconecto.");
                ac__memoria = false;
                break;
            default:
                log_warning(logger, "Memoria envio operación desconocida.");
                break;
            case memoria__creacion_de_proceso__kernel:
                sem_post(&sem__creacion_proceso[0]);
                sem_wait(&sem__creacion_proceso[1]);
                break;
            case memoria__eliminacion_de_proceso__kernel:
                sem_post(&sem__eliminacion_proceso[0]);
                sem_wait(&sem__eliminacion_proceso[1]);
                break;
            case terminar:
                ac__memoria = false;
                break;
        }
    }

    return NULL;
}

void* cx__interfaz(t_interfaz* interfaz) {
    bool actividad__interfaz = true;

    for (int i = 0; i < 2; i++) sem_init(&(interfaz -> semaforo_validacion[i]), 0, 0);
    interfaz -> lista__blocked = list_create();

    while (actividad__interfaz) {
        int operacion = buffer_scan(interfaz -> socket);
        
        switch (operacion) {
            case -1:
                log_warning(logger, "%s se desconecto.", interfaz -> nombre);
                actividad__interfaz = false;
                break;
            default:
                log_warning(logger, "Operación desconocída de %s. (%d)", interfaz -> nombre, operacion);
                break;
            case interfaz__validacion__kernel:
                sem_post(&(interfaz -> semaforo_validacion[0]));
                sem_wait(&(interfaz -> semaforo_validacion[1]));
                break;
            case interfaz__operacion__kernel:
                if (!planificacion_activa) {
                    interfaz -> operacion_terminada = true;
                    sem_wait(&(interfaz -> semaforo_operacion));
                }
                pthread_mutex_lock(&mx__procesos);
                interfaz__completar_operacion(interfaz);
                interfaz -> operacion_terminada = false;
                pthread_mutex_unlock(&mx__procesos);
                break;
            case interfaz__recepcion__kernel:
                sem_post(&(interfaz -> semaforo_recepcion));
                break;
            case terminar:
                actividad__interfaz = false;
                break;
        }
    }

    for (int i = 0; i < 2; i++) sem_destroy(&(interfaz -> semaforo_validacion[i]));
    sem_destroy(&(interfaz -> semaforo_operacion));
    sem_destroy(&(interfaz -> semaforo_recepcion));
    list_destroy(interfaz -> lista__blocked);

    free(interfaz -> nombre);
    free(interfaz);

    return NULL;
}

void* cx__interfaces() {
    for (int i = 0; i < 3; i++) sem_wait(&sem__interfaces);

    ac__interfaces = true;

    if ((local__kernel = server_start(ip_escucha, puerto_escucha)) == -1) {
        log_error(logger, "No se puede preparar para Interfaces de Entrada / Salida.");
        ac__interfaces = false;
    }

    if (ac__interfaces) {
        log_info(logger, "Preparado para Interfaces de Entrada / Salida.");
        interfaces = list_create();
    }

    sem_post(&sem__consola);
    sem_post(&sem__pl__largo_plazo);
    sem_post(&sem__pl__corto_plazo);
    
    while (ac__interfaces) {
        pthread_t th__interfaz;

        int client__interfaz = server_await(local__kernel);
        
        if (buffer_scan(client__interfaz) != interfaz__handshake__kernel) {
            log_error(logger, "Interfaz no validada.");
            client_disconnect(client__interfaz);
            continue;
        }

        t_interfaz* interfaz = interfaz__conectar(client__interfaz);

        list_add(interfaces, interfaz);

        log_info(logger, "Aceptado a %s.", interfaz -> nombre);
        buffer_send(buffer_create(kernel__handshake__interfaz), client__interfaz);

        pthread_create(&th__interfaz, NULL, (void* (*) (void*)) cx__interfaz, (void*) interfaz);
        pthread_detach(th__interfaz);
    }

    return NULL;
}
