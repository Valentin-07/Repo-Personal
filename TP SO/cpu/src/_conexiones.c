#include <__cpu.h>

void* cx__memoria() {
    ac__memoria = true;

    if ((server__memoria = client_connect(ip_memoria, puerto_memoria)) == -1) {
        log_error(logger, "Memoria indisponible.");
        ac__memoria = false;
    } else {
        buffer_send(buffer_create(cpu__handshake__memoria), server__memoria);
        if (buffer_scan(server__memoria) != memoria__handshake__cpu) {
            log_error(logger, "Memoria invalidada.");
            client_disconnect(server__memoria);
            ac__memoria = false;
        }
    }

    if (ac__memoria) {
        t_buffer* buffer = buffer_receive(server__memoria);
        buffer_unpack(buffer, &tam_pagina, sizeof(int));
        buffer_destroy(buffer);
        log_info(logger, "Conectado a Memoria.");
    }

    while (ac__memoria) {
        switch ((int) buffer_scan(server__memoria)) {
            case -1:
                log_warning(logger, "Memoria se desconecto.");
                ac__memoria = false;
                break;
            default:
                log_warning(logger, "Memoria envio operación desconocida.");
                break;
            case memoria__instruccion__cpu:
                sem_post(&sem__instruccion[0]);
                sem_wait(&sem__instruccion[1]);
                break;
            case memoria__traduccion__cpu:
                sem_post(&sem__marco[0]);
                sem_wait(&sem__marco[1]);
                break;
            case memoria__operacion__cpu:
                sem_post(&sem__memoria[0]);
                sem_wait(&sem__memoria[1]);
                break;
            case terminar:
                ac__memoria = false;
                break;
        }
    }

    return NULL;
}

void* cx__kernel__dispatch() {
    ac__kernel__dispatch = true;

    if ((local__cpu_dispatch = server_start(ip_escucha, puerto_escucha_dispatch)) == -1) {
        log_error(logger, "No se puede preparar Dispatch.");
        ac__kernel__dispatch = false;
    } else {
        client__kernel_dispatch = server_await(local__cpu_dispatch);
        if (buffer_scan(client__kernel_dispatch) != kernel__handshake__cpu_dispatch) {
            log_error(logger, "Kernel no validada para Dispatch.");
            client_disconnect(client__kernel_dispatch);
            ac__kernel__dispatch = false;
        }
    }

    if (ac__kernel__dispatch) log_info(logger, "Aceptado a Kernel para Dispatch");
    buffer_send(buffer_create(cpu_dispatch__handshake__kernel), client__kernel_dispatch);

    while (ac__kernel__dispatch) {
        switch ((int) buffer_scan(client__kernel_dispatch)) {
            case -1:
                log_warning(logger, "Kernel se desconecto de Dispatch.");
                ac__kernel__dispatch = false;
                break;
            default:
                log_warning(logger, "Memoria envio operación desconocida a Dispatch.");
                break;
            case kernel__contexto__cpu_dispatch:
                pthread_mutex_lock(&mx__contexto);
                cargar_contexto();
                pthread_mutex_unlock(&mx__contexto);
                break;
            case terminar:
                ac__kernel__dispatch = false;
                ac_procesamiento = false;
                buffer_send(buffer_create(terminar), client__kernel_dispatch);
                buffer_send(buffer_create(terminar), server__memoria);
                sem_post(&sem__ciclo);
                break;
        }
    }

    return NULL;
}

void* cx__kernel__interrupt() {
    ac__kernel__interrupt = true;

    if ((local__cpu_interrupt = server_start(ip_escucha, puerto_escucha_interrupt)) == -1) {
        log_error(logger, "No se puede preparar Interrupt.");
        ac__kernel__interrupt = false;
    } else {
        client__kernel_interrupt = server_await(local__cpu_interrupt);
        if (buffer_scan(client__kernel_interrupt) != kernel__handshake__cpu_interrupt) {
            log_error(logger, "Kernel no validada para Interrupt.");
            client_disconnect(client__kernel_interrupt);
            ac__kernel__interrupt = false;
        }
    }

    if (ac__kernel__interrupt) log_info(logger, "Aceptado a Kernel para Interrupt");
    buffer_send(buffer_create(cpu_interrupt__handshake__kernel), client__kernel_interrupt);

    while (ac__kernel__interrupt) {
        switch ((int) buffer_scan(client__kernel_interrupt)) {
            case -1:
                log_warning(logger, "Kernel se desconecto de Interrupt.");
                ac__kernel__interrupt = false;
                break;
            default:
                log_warning(logger, "Memoria envio operación desconocida a Interrupt.");
                break;
            case kernel__interrupcion__cpu_interrupt:
                pthread_mutex_lock(&mx__contexto);
                manejar_interrupcion();
                pthread_mutex_unlock(&mx__contexto);
                break;
            case terminar:
                ac__kernel__interrupt = false;
                buffer_send(buffer_create(terminar), client__kernel_interrupt);
                break;
        }
    }

    return NULL;
}
