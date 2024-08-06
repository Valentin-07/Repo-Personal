#include <__cpu.h>

char* cadena_registro[11] = {"PC", "AX", "BX", "CX", "DX", "EAX", "EBX", "ECX", "EDX", "SI", "DI"};

char* cadena_tipo_instruccion[19] = {"SET", "MOV_IN", "MOV_OUT", "SUM", "SUB", "JNZ", "RESIZE", "COPY_STRING", "WAIT", "SIGNAL", "IO_GEN_SLEEP", "IO_STDIN_READ", "IO_STDOUT_WRITE", "IO_FS_CREATE", "IO_FS_DELETE", "IO_FS_TRUNCATE", "IO_FS_WRITE", "IO_FS_READ", "EXIT"};

void cargar_contexto() {
    t_buffer* buffer = buffer_receive(client__kernel_dispatch);

    buffer_unpack(buffer, &pid, sizeof(int));
    buffer_unpack(buffer, registro[PC], sizeof(uint32_t));
    for (t_indice_registro i = AX; i <= DX; i++) buffer_unpack(buffer, registro[i], sizeof(uint8_t));
    for (t_indice_registro i = EAX; i <= EDX; i++) buffer_unpack(buffer, registro[i], sizeof(uint32_t));
    buffer_unpack(buffer, registro[SI], sizeof(uint32_t));
    buffer_unpack(buffer, registro[DI], sizeof(uint32_t));

    controlador_de_interrupciones . tipo_interrupcion = -1;

    buffer_destroy(buffer);
    modo_ejecucion = usuario;

    sem_post(&sem__ciclo);
}

char* busqueda_instruccion() {
    t_buffer* solicitud_instruccion = buffer_create(cpu__instruccion__memoria);
    buffer_pack(solicitud_instruccion, &pid, sizeof(int));
    buffer_pack(solicitud_instruccion, registro[PC], sizeof(uint32_t));
    buffer_send(solicitud_instruccion, server__memoria);

    log_info(logger, "PID: %d - FETCH - Program Counter: %d", pid, (int) *(uint32_t*)registro[PC]);

    sem_wait(&sem__instruccion[0]);

    t_buffer* buffer = buffer_receive(server__memoria);
    size_t len; buffer_unpack(buffer, &len, sizeof(size_t));
    char* instruccion = malloc(len);
    buffer_unpack(buffer, instruccion, len);
    buffer_destroy(buffer);

    (*(uint32_t*)registro[PC])++;

    sem_post(&sem__instruccion[1]);

    return instruccion;
}

char** decodificacion_instruccion(char* instruccion) {
    char** arr = string_split(instruccion, " ");
    free(instruccion);
    return arr;
}

t_indice_registro apuntar_registro(char* str) {
    for (t_indice_registro reg = PC; reg <= DI; reg++) if (!strcmp(str, cadena_registro[reg])) return reg;
    return -1;
}

t_instruccion tipo_instruccion(char* str) {
    for (t_instruccion i = 0; i < 19; i++) if (strcmp(cadena_tipo_instruccion[i], str) == 0) return i;
    return -1;
}

void ejecucion_instruccion(char** arr) {
    t_instruccion _tipo_instruccion = tipo_instruccion(arr[0]);
    
        switch (_tipo_instruccion) {
            case SET:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_set(apuntar_registro(arr[1]), atoi(arr[2]));
                break;
            case MOV_IN:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_mov_in(apuntar_registro(arr[1]), apuntar_registro(arr[2]));
                break;
            case MOV_OUT:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_mov_out(apuntar_registro(arr[1]), apuntar_registro(arr[2]));
                break;
            case SUM:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_sum(apuntar_registro(arr[1]), apuntar_registro(arr[2]));
                break;
            case SUB:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_sub(apuntar_registro(arr[1]), apuntar_registro(arr[2]));
                break;
            case JNZ:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_jnz(apuntar_registro(arr[1]), atoi(arr[2]));
                break;
            case RESIZE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s", pid, arr[0], arr[1]);
                instruccion_resize(atoi(arr[1]));
                break;
            case COPY_STRING:
                log_info(logger, "PID: %d - Ejecutando: %s - %s", pid, arr[0], arr[1]);
                instruccion_copy_string(atoi(arr[1]));
                break;
            case WAIT:
                log_info(logger, "PID: %d - Ejecutando: %s - %s", pid, arr[0], arr[1]);
                instruccion_wait(arr[1]);
                break;
            case SIGNAL:
                log_info(logger, "PID: %d - Ejecutando: %s - %s", pid, arr[0], arr[1]);
                instruccion_signal(arr[1]);
                break;
            case IO_GEN_SLEEP:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_io_gen_sleep(arr[1], atoi(arr[2]));
                break;
            case IO_STDIN_READ: 
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s, %s", pid, arr[0], arr[1], arr[2], arr[3]);
                instruccion_io_stdin_read(arr[1], apuntar_registro(arr[2]), apuntar_registro(arr[3]));
                break;
            case IO_STDOUT_WRITE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s, %s", pid, arr[0], arr[1], arr[2], arr[3]);
                instruccion_io_stdout_write(arr[1], apuntar_registro(arr[2]), apuntar_registro(arr[3]));
                break;
            case IO_FS_CREATE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_io_fs_create(arr[1], arr[2]);
                break;
            case IO_FS_DELETE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s", pid, arr[0], arr[1], arr[2]);
                instruccion_io_fs_delete(arr[1], arr[2]);
                break;
            case IO_FS_TRUNCATE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s, %s", pid, arr[0], arr[1], arr[2], arr[3]);
                instruccion_io_fs_truncate(arr[1], arr[2], apuntar_registro(arr[3]));
                break;
            case IO_FS_WRITE:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s, %s, %s, %s", pid, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
                instruccion_io_fs_write(arr[1], arr[2], apuntar_registro(arr[3]), apuntar_registro(arr[4]), apuntar_registro(arr[5]));
                break;
            case IO_FS_READ:
                log_info(logger, "PID: %d - Ejecutando: %s - %s, %s, %s, %s, %s", pid, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
                instruccion_io_fs_read(arr[1], arr[2], apuntar_registro(arr[3]), apuntar_registro(arr[4]), apuntar_registro(arr[5]));
                break;
            case EXIT:
                log_info(logger, "PID: %d - Ejecutando: %s", pid, arr[0]);
                instruccion_exit();
                break;
        }

        string_array_destroy(arr);
}

bool check_interrupt() {
    return controlador_de_interrupciones . tipo_interrupcion != (t_interrupcion) -1;
}

void manejar_interrupcion() {
    t_buffer* buffer = buffer_receive(client__kernel_interrupt);

    t_interrupcion _int; buffer_unpack(buffer, &_int, sizeof(t_interrupcion));

    if (modo_ejecucion == kernel || (modo_ejecucion == usuario && (_int != interrupcion__terminacion && controlador_de_interrupciones . tipo_interrupcion != (t_interrupcion) -1))) {
        buffer_destroy(buffer);
    } else {
        controlador_de_interrupciones . tipo_interrupcion = _int;
        buffer_destroy(buffer);
    }
}

void guardar_contexto() {
    t_buffer* buffer = buffer_create(cpu_dispatch__contexto__kernel);
    buffer_pack(buffer, registro[PC], sizeof(uint32_t));
    for (t_indice_registro i = AX; i <= DX; i++) buffer_pack(buffer, registro[i], sizeof(uint8_t));
    for (t_indice_registro i = EAX; i <= EDX; i++) buffer_pack(buffer, registro[i], sizeof(uint32_t));
    buffer_pack(buffer, registro[SI], sizeof(uint32_t));
    buffer_pack(buffer, registro[DI], sizeof(uint32_t));
    
    buffer_pack(buffer, &(controlador_de_interrupciones . tipo_interrupcion), sizeof(t_interrupcion));

    switch (controlador_de_interrupciones . tipo_interrupcion) {
        case interrupcion__out_of_memory:
            break;
        case interrupcion__salida:
            break;
        case interrupcion__terminacion:
            break;
        case interrupcion__timer:
            break;
        case interrupcion__wait:
            buffer_pack_string(buffer, controlador_de_interrupciones . recurso . nombre);
            free(controlador_de_interrupciones . recurso . nombre);
            break;
        case interrupcion__signal:
            buffer_pack_string(buffer, controlador_de_interrupciones . recurso . nombre);
            free(controlador_de_interrupciones . recurso . nombre);
            break;
        case interrupcion__operacion_io:
            buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . nombre);
            buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . tipo_operacion), sizeof(t_operacion_io));
            switch (controlador_de_interrupciones . interfaz . tipo_operacion) {
                case gen_sleep:
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_gen_sleep . unidades_trabajo), sizeof(int));
                    break;
                case stdin_read:
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_stdin_read . direccion_fisica), sizeof(int));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_stdin_read . bytes), sizeof(size_t));
                    break;
                case stdout_write:
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_stdout_write . direccion_fisica), sizeof(int));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_stdout_write . bytes), sizeof(size_t));
                    break;
                case dialfs_create:
                    buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    free(controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    break;
                case dialfs_delete:
                    buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    free(controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    break;
                case dialfs_truncate:
                    buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _truncate . bytes), sizeof(size_t));
                    free(controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    break;
                case dialfs_write:
                    buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _write . bytes), sizeof(size_t));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _write . direccion_fisica), sizeof(int));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _write . puntero), sizeof(int));
                    free(controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    break;
                case dialfs_read:
                    buffer_pack_string(buffer, controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _read . bytes), sizeof(size_t));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _read . direccion_fisica), sizeof(int));
                    buffer_pack(buffer, &(controlador_de_interrupciones . interfaz . _io_dialfs . _read . puntero), sizeof(int));
                    free(controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo);
                    break;
            }
            free(controlador_de_interrupciones . interfaz . nombre);
            break;
    }

    buffer_send(buffer, client__kernel_dispatch);

    controlador_de_interrupciones . tipo_interrupcion = -1;
    modo_ejecucion = kernel;

    pid = -1;
}
