#include <__es.h>

void* cx__kernel() {
    ac__kernel = true;

    if ((server__kernel = client_connect(ip_kernel, puerto_kernel)) == -1) {
        log_error(logger, "Kernel indisponible.");
        ac__kernel = false;
    } else {
        t_buffer* buffer = buffer_create(interfaz__handshake__kernel);
        size_t name_length = (size_t) (string_length(nombre_interfaz) + 1);
        buffer_pack(buffer, &name_length, sizeof(size_t));
        buffer_pack(buffer, nombre_interfaz, name_length);
        buffer_send(buffer, server__kernel);
        if (buffer_scan(server__kernel) != kernel__handshake__interfaz) {
            log_error(logger, "Kernel invalidada.");
            client_disconnect(server__kernel);
            ac__kernel = false;
        }
    }

    if (ac__kernel) log_info(logger, "Conectado a Kernel.");

    while (ac__kernel) {
        pthread_t th_operacion;
        
        switch ((int) buffer_scan(server__kernel)) {
            case -1:
                log_warning(logger, "Kernel se desconecto.");
                ac__kernel = false;
                break;
            default:
                log_warning(logger, "Kernel envio operación desconocida.");
                break;
            case kernel__validacion__interfaz:
                validar_operacion();
                break;
            case kernel__operacion__interfaz:
                pthread_create(&th_operacion, NULL, ejecutar_operacion, NULL);
                sem_post(&sem__operaciones[0]);
                sem_wait(&sem__operaciones[1]);
                pthread_detach(th_operacion);
                break;
            case terminar:
                buffer_send(buffer_create(terminar), server__kernel);
                ac__kernel = false;
                break;
        }
    }

    return NULL;
}

void* cx__memoria() {
    ac__memoria = true;

    if ((server__memoria = client_connect(ip_memoria, puerto_memoria)) == -1) {
        log_error(logger, "Memoria indisponible.");
        ac__memoria = false;
    } else {
        t_buffer* buffer = buffer_create(interfaz__handshake__memoria);
        size_t name_length = (size_t) (string_length(nombre_interfaz) + 1);
        buffer_pack(buffer, &name_length, sizeof(size_t));
        buffer_pack(buffer, nombre_interfaz, name_length);
        buffer_send(buffer, server__memoria);
        if (buffer_scan(server__memoria) != memoria__handshake__interfaz) {
            log_error(logger, "Memoria invalidada.");
            client_disconnect(server__memoria);
            ac__memoria = false;
        }
    }

    if (ac__memoria) log_info(logger, "Conectado a Memoria.");

    while (ac__memoria) {
        switch ((int) buffer_scan(server__memoria)) {
            case -1:
                log_warning(logger, "Memoria se desconecto.");
                ac__memoria = false;
                break;
            default:
                log_warning(logger, "Memoria envio operación desconocida.");
                break;
            case memoria__operacion__interfaz:
                sem_post(&sem__memoria[0]);
                sem_wait(&sem__memoria[1]);
                break;
            case terminar:
                buffer_send(buffer_create(terminar), server__memoria);
                ac__memoria = false;
                break;
        }
    }

    return NULL;
}
