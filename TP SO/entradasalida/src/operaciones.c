#include <__es.h>

void validar_operacion() {
    t_buffer* buffer = buffer_receive(server__kernel);
    t_operacion_io tipo_operacion; buffer_unpack(buffer, &tipo_operacion, sizeof(t_operacion_io));

    bool validacion;

    switch (tipo_interfaz) {
        case io_generica:
            validacion = tipo_operacion == gen_sleep;
            break;
        case io_stdin:
            validacion = tipo_operacion == stdin_read;
            break;
        case io_stdout:
            validacion = tipo_operacion == stdout_write;
            break;
        case io_dialfs:
            validacion = tipo_operacion >= dialfs_create && tipo_operacion <= dialfs_write;
            break;
    }

    buffer_repurpose(buffer, interfaz__validacion__kernel);
    buffer_pack(buffer, &validacion, sizeof(bool));
    buffer_send(buffer, server__kernel);
}


void operacion__gen_sleep(int pid, t_buffer* buffer) {
    int cantidad; buffer_unpack(buffer, &cantidad, sizeof(int));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Operacion: %s - %d", pid, "SLEEP", cantidad * tiempo_unidad_trabajo);
    usleep((useconds_t) (tiempo_unidad_trabajo * cantidad * 1000));
}

bool operacion__stdin_read(int pid, t_buffer* buffer) {
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Operacion: %s - %d, %ld", pid, "STDIN_READ", direccion_fisica, bytes);

    char* text = readline("");

    if (text != NULL) {
        int len = string_length(text);
        void* data = malloc(bytes);
        memset(data, 0, bytes);
        memcpy(data, text, bytes > len ? len : bytes);
        free(text);

        t_buffer* buffer_fs = buffer_create(interfaz__operacion__memoria);
        t_operacion_memoria operacion = mem__write; buffer_pack(buffer_fs, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer_fs, &pid, sizeof(int));
        buffer_pack(buffer_fs, &bytes, sizeof(size_t));
        buffer_pack(buffer_fs, data, bytes);
        buffer_pack(buffer_fs, &direccion_fisica, sizeof(int));

        buffer_send(buffer_fs, server__memoria);
        free(data);

        sem_wait(&sem__memoria[0]);

        buffer_fs = buffer_receive(server__memoria);
        bool resultado; buffer_unpack(buffer_fs, &resultado, sizeof(bool));
        buffer_destroy(buffer_fs);

        sem_post(&sem__memoria[1]);

        return resultado;
    } else {
        return true;
    }
}

bool operacion__stdout_write(int pid, t_buffer* buffer) {
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Operacion: %s - %d, %ld", pid, "STDOUT_WRITE", direccion_fisica, bytes);

    t_buffer* buffer_fs = buffer_create(interfaz__operacion__memoria);
    t_operacion_memoria operacion = mem__read; buffer_pack(buffer_fs, &operacion, sizeof(t_operacion_memoria));
    buffer_pack(buffer_fs, &pid, sizeof(int));
    buffer_pack(buffer_fs, &bytes, sizeof(size_t));
    buffer_pack(buffer_fs, &direccion_fisica, sizeof(int)); // ATT

    buffer_send(buffer_fs, server__memoria);

    sem_wait(&sem__memoria[0]);

    buffer_fs = buffer_receive(server__memoria);
    bool resultado; buffer_unpack(buffer_fs, &resultado, sizeof(bool));
    if (resultado) {
        char* text = buffer_unpack_string(buffer_fs);
        printf("%s\n", text);
        free(text);
    }
    buffer_destroy(buffer_fs);

    sem_post(&sem__memoria[1]);

    return resultado;
}


bool operacion__dialfs_create(int pid, t_buffer* buffer) {
    char* nombre_archivo = buffer_unpack_string(buffer);
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Crear Archivo: %s", pid, nombre_archivo);

    usleep((useconds_t) (tiempo_unidad_trabajo * 1000));

    t_metadata* metadata = metadata__crear(nombre_archivo);

    bool resultado;

    if (!metadata) {
        log_error(logger, "No se pudo crear el archivo \"%s\".", nombre_archivo);
        free(nombre_archivo);
        resultado = false;
    } else {
        bitmap__ocupar(metadata -> bloque_inicial);
        metadata__cerrar(metadata);
        resultado = true;
    }

    return resultado;
}


bool operacion__dialfs_delete(int pid, t_buffer* buffer) {
    char* nombre_archivo = buffer_unpack_string(buffer);
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);
    
    log_info(logger, "PID: %d - Eliminar Archivo: %s", pid, nombre_archivo);
    
    usleep((useconds_t) (tiempo_unidad_trabajo * 1000));

    t_metadata* metadata = metadata__abrir(nombre_archivo);

    bool resultado;

    if (!metadata) {
        log_error(logger, "No existe el archivo \"%s\".", nombre_archivo);
        free(nombre_archivo);
        resultado = false;
    } else {
        int cantidad_bloques_a_desocupar = metadata -> tam != 0 ? ceil(metadata -> tam / block_size) + 1 : 1;
        for (int i = 0; i < cantidad_bloques_a_desocupar; i++) bitmap__desocupar(metadata -> bloque_inicial + i);
        metadata__borrar(metadata);
        resultado = true;
    }

    return resultado;
}

bool operacion__dialfs_truncate(int pid, t_buffer* buffer) {
    char* nombre_archivo = buffer_unpack_string(buffer);
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Truncar Archivo: %s - Tamaño: %ld", pid, nombre_archivo, bytes);

    usleep((useconds_t) (tiempo_unidad_trabajo * 1000));

    t_metadata* metadata = metadata__abrir(nombre_archivo);

    bool resultado;

    if (!metadata) {
        log_error(logger, "No existe el archivo \"%s\".", nombre_archivo);
        free(nombre_archivo);
        resultado = false;
    } else {
        int bloques_i = bloques__cantidad_a_ocupar(metadata -> tam);
        int bloques_f = bloques__cantidad_a_ocupar(bytes);

        if (bloques_f == bloques_i) {
            metadata -> tam = bytes;
            resultado = true;
        } else if (bloques_f < bloques_i) {
            int bloques_a_desocupar = bloques_i - bloques_f;
            for (int i = 0; i < bloques_a_desocupar; i++) bitmap__desocupar(metadata -> bloque_inicial + bloques_f + i);
            metadata -> tam = bytes;
            resultado = true;
        } else {
            int bloques_a_ocupar = bloques_f - bloques_i;

            if (bitmap__disponibilidad_contigua(metadata -> bloque_inicial + bloques_i, bloques_a_ocupar)) {
                for (int i = 0; i < bloques_a_ocupar; i++) bitmap__ocupar(metadata -> bloque_inicial + bloques_i + i);
                metadata -> tam = bytes;
                resultado = true;
            } else if (bitmap__cantidad_libres() >= bloques_a_ocupar) {
                log_info(logger, "PID: %d - Inicio Compactación.", pid);
                compactar(metadata);
                log_info(logger, "PID: %d - Fin Compactación.", pid);
                for (int i = 0; i < bloques_a_ocupar; i++) bitmap__ocupar(metadata -> bloque_inicial + bloques_i + i);
                metadata -> tam = bytes;
                resultado = true;
            } else {
                resultado = false;
            }
        }
    }

    metadata__cerrar(metadata);

    return resultado;
}

bool operacion__dialfs_write(int pid, t_buffer* buffer) {
    char* nombre_archivo = buffer_unpack_string(buffer);
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));
    int puntero; buffer_unpack(buffer, &puntero, sizeof(int));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %ld - Puntero Archivo: %d", pid, nombre_archivo, bytes, puntero);

    usleep((useconds_t) (tiempo_unidad_trabajo * 1000));

    t_metadata* metadata = metadata__abrir(nombre_archivo);

    bool resultado;

    if (metadata -> tam < puntero + bytes) {
        log_error(logger, "\"%s\" no tiene suficiente tamaño para escritura.", nombre_archivo);
        resultado = false;
    } else {
        t_buffer* buffer_fs = buffer_create(interfaz__operacion__memoria);
        t_operacion_memoria operacion = mem__read; buffer_pack(buffer_fs, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer_fs, &pid, sizeof(int));
        buffer_pack(buffer_fs, &bytes, sizeof(size_t));
        buffer_pack(buffer_fs, &direccion_fisica, sizeof(int));

        buffer_send(buffer_fs, server__memoria);

        sem_wait(&sem__memoria[0]);

        buffer_fs = buffer_receive(server__memoria);

        buffer_unpack(buffer_fs, &resultado, sizeof(bool));
        
        if (resultado) {
            bloques__escribir(buffer_unpack_string(buffer_fs), block_size * metadata -> bloque_inicial + puntero, bytes);
        } else {
            log_error(logger, "No se ha leido correctamente de memoria.");
        }

        buffer_destroy(buffer_fs);

        sem_post(&sem__memoria[1]);
    }

    metadata__cerrar(metadata);

    return resultado;
}

bool operacion__dialfs_read(int pid, t_buffer* buffer) {
    char* nombre_archivo = buffer_unpack_string(buffer);
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));
    int puntero; buffer_unpack(buffer, &puntero, sizeof(int));
    buffer_repurpose(buffer, interfaz__recepcion__kernel);
    buffer_send(buffer, server__kernel);
    sem_post(&sem__operaciones[1]);

    log_info(logger, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %ld - Puntero Archivo: %d", pid, nombre_archivo, bytes, puntero);

    usleep((useconds_t) (tiempo_unidad_trabajo * 1000));

    t_metadata* metadata = metadata__abrir(nombre_archivo);

    bool resultado;

    if (metadata -> tam < puntero + bytes) {
        log_error(logger, "\"%s\" no tiene suficiente tamaño para lectura.", nombre_archivo);
        resultado = false;
    } else {
        void* text = bloques__leer(block_size * metadata -> bloque_inicial + puntero, bytes);

        void* data = malloc(bytes);
        memset(data, 0, bytes);
        memcpy(data, text, bytes);
        free(text);

        t_buffer* buffer_fs = buffer_create(interfaz__operacion__memoria);
        t_operacion_memoria operacion = mem__write; buffer_pack(buffer_fs, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer_fs, &pid, sizeof(int));
        buffer_pack(buffer_fs, &bytes, sizeof(size_t));
        buffer_pack(buffer_fs, data, bytes);
        buffer_pack(buffer_fs, &direccion_fisica, sizeof(int));

        buffer_send(buffer_fs, server__memoria);
        free(data);
        
        sem_wait(&sem__memoria[0]);

        buffer_fs = buffer_receive(server__memoria);
        buffer_unpack(buffer_fs, &resultado, sizeof(bool));
        buffer_destroy(buffer_fs);

        sem_post(&sem__memoria[1]);
    }

    metadata__cerrar(metadata);
    
    return resultado;
}



void* ejecutar_operacion() {
    sem_wait(&sem__operaciones[0]);
    t_buffer* buffer = buffer_receive(server__kernel);
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    t_operacion_io tipo_operacion; buffer_unpack(buffer, &tipo_operacion, sizeof(t_operacion_io));

    bool resultado;

    switch (tipo_operacion) {
        case gen_sleep:
            operacion__gen_sleep(pid, buffer);
            resultado = true;
            break;
        case stdin_read:
            resultado = operacion__stdin_read(pid, buffer);
            break;
        case stdout_write:
            resultado = operacion__stdout_write(pid, buffer);
            break;
        case dialfs_create:
            resultado = operacion__dialfs_create(pid, buffer);
            break;
        case dialfs_delete:
            resultado = operacion__dialfs_delete(pid, buffer);
            break;
        case dialfs_truncate:
            resultado = operacion__dialfs_truncate(pid, buffer);
            break;
        case dialfs_write:
            resultado = operacion__dialfs_write(pid, buffer);
            break;
        case dialfs_read:
            resultado = operacion__dialfs_read(pid, buffer);
            break;
    }

    buffer = buffer_create(interfaz__operacion__kernel);
    buffer_pack(buffer, &pid, sizeof(int));
    buffer_pack(buffer, &resultado, sizeof(bool));
    buffer_send(buffer, server__kernel);

    return NULL;
}
