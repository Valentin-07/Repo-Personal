#include <__memoria.h>

void* cx__cpu() {
    ac__cpu = true;
    
    if ((local__memoria = server_start(ip_escucha, puerto_escucha)) == -1) {
        log_error(logger, "No se puede preparar escucha para CPU.");
        ac__cpu = false;
    } else {
        client__cpu = server_await(local__memoria);
        if (buffer_scan(client__cpu) != cpu__handshake__memoria) {
            log_error(logger, "CPU no validada.");
            client_disconnect(client__cpu);
            ac__cpu = false;
        }
    }

    if (ac__cpu) {
        t_buffer* buffer = buffer_create(memoria__handshake__cpu);
        buffer_pack(buffer, &tam_pagina, sizeof(int));
        buffer_send(buffer, client__cpu);
        log_info(logger, "Aceptado a CPU");
    }
    
    sem_post(&sem__interfaces);
    
    sem_post(&sem__sec_hilos[0]);
    
    while (ac__cpu) {
        switch ((int) buffer_scan(client__cpu)) {
            case -1:
                log_error(logger, "CPU se desconecto.");
                ac__cpu = false;
                break;
            default:
                log_warning(logger, "CPU envio operación desconocida.");
                break;
            case terminar:
                buffer_send(buffer_create(terminar), client__cpu);
                ac__cpu = false;
                break;
            case cpu__instruccion__memoria:
                enviar_instruccion();
                break;
            case cpu__traduccion__memoria:
                obtener_marco();
                break;
            case cpu__operacion__memoria:
                operacion_cpu();
                break;
        }
    }

    return NULL;
}

void* cx__kernel() {
    ac__kernel = true;

    sem_wait(&sem__sec_hilos[0]);
    
    if (local__memoria == -1) {
        log_error(logger, "No se puede preparar escucha para Kernel.");
        ac__kernel = false;
    } else {
        client__kernel = server_await(local__memoria);
        if (buffer_scan(client__kernel) != kernel__handshake__memoria) {
            log_error(logger, "Kernel no validada.");
            client_disconnect(client__kernel);
            ac__kernel = false;
        }
    }
    
    if (ac__kernel) {
        t_buffer* buffer = buffer_create(memoria__handshake__kernel);
        buffer_pack_string(buffer, path_instrucciones);
        buffer_send(buffer, client__kernel);
        log_info(logger, "Aceptado a Kernel");
        sem_post(&sem__interfaces);
    }
    
    
    while (ac__kernel) {
        switch ((int) buffer_scan(client__kernel)) {
            case -1:
                log_error(logger, "Kernel se desconecto.");
                ac__kernel = false;
                break;
            default:
                log_warning(logger, "Kernel envio operación desconocida.");
                break;
            case terminar:
                buffer_send(buffer_create(terminar), client__kernel);
                ac__kernel = false;
                break;
            case kernel__creacion_de_proceso__memoria:
                almacenar_proceso();
                break;
            case kernel__eliminacion_de_proceso__memoria:
                liberar_proceso();
                break;
        }
    }

    return NULL;
}

void* cx__interfaz(t_interfaz* interfaz) {
    bool actividad__interfaz = true;

    for (int i = 0; i < 2; i++) sem_init(&(interfaz -> semaforo[i]), 0, 0);

    while (actividad__interfaz) {
        int operacion = buffer_scan(interfaz -> socket);
        
        switch (operacion) {
            case -1:
                log_warning(logger, "%s se desconecto.", interfaz -> nombre);
                list_remove_element(interfaces, interfaz);
                actividad__interfaz = false;
                break;
            default:
                log_warning(logger, "Operación desconocída de %s. (%d)", interfaz -> nombre, operacion);
                break;
            case interfaz__operacion__memoria:
                operacion_interfaz(interfaz);
                break;
            case terminar:
                actividad__interfaz = false;
                sem_post(&interfaz -> semaforo[0]);
                sem_wait(&interfaz -> semaforo[1]);
                break;
        }
    }

    for (int i = 0; i < 2; i++) sem_destroy(&(interfaz -> semaforo[i]));

    free(interfaz -> nombre);
    free(interfaz);

    return NULL;
}

void* cx__interfaces() {
    for (int i = 0; i < 2; i++) sem_wait(&sem__interfaces);

    ac__interfaces = true;

    if (local__memoria == -1) {
        log_error(logger, "No se puede preparar para Interfaces de Entrada / Salida.");
        ac__interfaces = false;
    }

    if (ac__interfaces) {
        log_info(logger, "Preparado para Interfaces de Entrada / Salida.");
        interfaces = list_create();
    }
    
    
    while (ac__interfaces) {
        pthread_t th__interfaz;

        int client__interfaz = server_await(local__memoria);
        
        if (buffer_scan(client__interfaz) != interfaz__handshake__memoria) {
            log_error(logger, "Interfaz no validada.");
            client_disconnect(client__interfaz);
            continue;
        }

        t_interfaz* interfaz = interfaz__conectar(client__interfaz);

        list_add(interfaces, interfaz);

        log_info(logger, "Aceptado a %s.", interfaz -> nombre);
        buffer_send(buffer_create(memoria__handshake__interfaz), client__interfaz);

        pthread_create(&th__interfaz, NULL, (void* (*) (void*)) cx__interfaz, (void*) interfaz);
        pthread_detach(th__interfaz);
    }

    return NULL;
}
