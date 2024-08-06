#include <__cpu.h>

int instruccion_set(t_indice_registro reg, int val) {
    if (reg >= AX && reg <= DX) {
        *(uint8_t*)registro[reg] = (uint8_t) val;
        return 1;
    } else if (reg >= PC && reg <= DI) {
        *(uint32_t*)registro[reg] = (uint32_t) val;
        return 1;
    } else return 0;
}

int instruccion_mov_in(t_indice_registro datos, t_indice_registro direccion) {
    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica != -1) {
        size_t tam_bytes = (datos >= AX && datos <= DX) ? sizeof(uint8_t) : sizeof(uint32_t);
        t_buffer* buffer = buffer_create(cpu__operacion__memoria);
        t_operacion_memoria operacion = mem__read; buffer_pack(buffer, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer, &pid, sizeof(int));
        buffer_pack(buffer, &tam_bytes, sizeof(size_t));
        buffer_pack(buffer, &direccion_fisica, sizeof(int));
        buffer_send(buffer, server__memoria);

        sem_wait(&sem__memoria[0]);

        buffer = buffer_receive(server__memoria);

        bool resultado; buffer_unpack(buffer, &resultado, sizeof(bool));
        if (resultado) {
            buffer_unpack(buffer, registro[datos], tam_bytes);
            log_info(logger, "PID: %d - Acción: %s - Dirección Física: %d - Valor: %d", pid, "LEER", direccion_fisica, (int) (tam_bytes == sizeof(uint8_t) ? *(uint8_t*)registro[datos] : *(uint32_t*)registro[datos]));
        }
        
        buffer_destroy(buffer);
        
        sem_post(&sem__memoria[1]);

        return resultado;
    }
    return 0;
}

int instruccion_mov_out(t_indice_registro direccion, t_indice_registro datos) {
    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica != -1) {
        size_t tam_bytes = (datos >= AX && datos <= DX) ? sizeof(uint8_t) : sizeof(uint32_t);
        t_buffer* buffer = buffer_create(cpu__operacion__memoria);
        t_operacion_memoria operacion = mem__write; buffer_pack(buffer, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer, &pid, sizeof(int));
        buffer_pack(buffer, &tam_bytes, sizeof(size_t));
        buffer_pack(buffer, registro[datos], tam_bytes);
        buffer_pack(buffer, &direccion_fisica, sizeof(int));
        buffer_send(buffer, server__memoria);

        sem_wait(&sem__memoria[0]);

        buffer = buffer_receive(server__memoria);

        bool resultado; buffer_unpack(buffer, &resultado, sizeof(bool));
        if (resultado) {
            void* valor = malloc(tam_bytes); buffer_unpack(buffer, valor, tam_bytes);
            log_info(logger, "PID: %d - Acción: %s - Dirección Física: %d - Valor: %d", pid, "ESCRIBIR", direccion_fisica, (int) (tam_bytes == sizeof(uint8_t) ? *(uint8_t*)valor : *(uint32_t*)valor));
            free(valor);
        }
        
        buffer_destroy(buffer);
        
        sem_post(&sem__memoria[1]);

        return resultado;
    }
    return 0;
}

int instruccion_sum(t_indice_registro reg_1, t_indice_registro reg_2) {
    if ((reg_1 >= PC && reg_1 <= DI) && (reg_2 >= PC && reg_2 <= DI)) {
        int val = (reg_2 >= AX && reg_2 <= DX) ? (int) *(uint8_t*)registro[reg_2] : (int) *(uint32_t*)registro[reg_2];
        if (reg_1 >= AX && reg_1 <= DX) *(uint8_t*)registro[reg_1] += (uint8_t) val; else *(uint32_t*)registro[reg_1] += (uint32_t) val;
        return 1;
    } else return 0;
}

int instruccion_sub(t_indice_registro reg_1, t_indice_registro reg_2) {
    if ((reg_1 >= PC && reg_1 <= DI) && (reg_2 >= PC && reg_2 <= DI)) {
        int val = (reg_2 >= AX && reg_2 <= DX) ? (int) *(uint8_t*)registro[reg_2] : (int) *(uint32_t*)registro[reg_2];
        if (reg_1 >= AX && reg_1 <= DX) *(uint8_t*)registro[reg_1] -= (uint8_t) val; else *(uint32_t*)registro[reg_1] += (uint32_t) val;
        return 1;
    } else return 0;
}

int instruccion_jnz(t_indice_registro reg, int _PC) {
    if (reg >= AX && reg <= DX) {
        if ((*(uint8_t*)registro[reg]) != 0) *(uint32_t*)registro[PC] = (uint32_t) _PC;
        return 1;
    } else if (reg >= PC && reg <= DI) {
        if ((*(uint32_t*)registro[reg]) != 0) *(uint32_t*)registro[PC] = (uint32_t) _PC;
        return 1;
    } else return 0;
}

int instruccion_resize(int size) {
    if (size < 0) return 0; else {
        t_buffer* buffer = buffer_create(cpu__operacion__memoria);
        t_operacion_memoria operacion = mem__resize; buffer_pack(buffer, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer, &pid, sizeof(int));
        buffer_pack(buffer, &size, sizeof(int));
        buffer_send(buffer, server__memoria);

        sem_wait(&sem__memoria[0]);

        buffer = buffer_receive(server__memoria);
        bool resultado; buffer_unpack(buffer, &resultado, sizeof(bool));
        buffer_destroy(buffer);

        if (!resultado) controlador_de_interrupciones . tipo_interrupcion = interrupcion__out_of_memory;

        sem_post(&sem__memoria[1]);

        return 1;
    }
}

int instruccion_copy_string(int size) {
    int direccion_fisica_s = mmu__calculo(pid, (int) *(uint32_t*)registro[SI]);
    int direccion_fisica_d = mmu__calculo(pid, (int) *(uint32_t*)registro[DI]);
    if (direccion_fisica_s != -1 && direccion_fisica_d != -1) {
        t_buffer* buffer = buffer_create(cpu__operacion__memoria);
        t_operacion_memoria operacion = mem__copy_string; buffer_pack(buffer, &operacion, sizeof(t_operacion_memoria));
        buffer_pack(buffer, &pid, sizeof(int));
        buffer_pack(buffer, &size, sizeof(int));
        buffer_pack(buffer, &direccion_fisica_s, sizeof(int));
        buffer_pack(buffer, &direccion_fisica_d, sizeof(int));
        buffer_send(buffer, server__memoria);

        sem_wait(&sem__memoria[0]);

        buffer = buffer_receive(server__memoria);
        bool resultado; buffer_unpack(buffer, &resultado, sizeof(bool));
        buffer_destroy(buffer);
        
        sem_post(&sem__memoria[1]);

        return resultado;
    }
    return 0;
}

int instruccion_wait(char* nom) {
    if (nom == NULL) return 0;
    controlador_de_interrupciones . tipo_interrupcion = interrupcion__wait;
    controlador_de_interrupciones . recurso . nombre = string_duplicate(nom);
    return 1;
}

int instruccion_signal(char* nom) {
    if (nom == NULL) return 0;
    controlador_de_interrupciones . tipo_interrupcion = interrupcion__signal;
    controlador_de_interrupciones . recurso . nombre = string_duplicate(nom);
    return 1;
}

int instruccion_io_gen_sleep(char* nom, int uni) {
    if (nom == NULL || uni <= 0) return 0;
    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = gen_sleep;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nom);
    controlador_de_interrupciones . interfaz . _io_gen_sleep . unidades_trabajo = uni;
    return 1;
}

int instruccion_io_stdin_read(char* nom, t_indice_registro direccion, t_indice_registro tam) {
    if (nom == NULL) return 0;

    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica == -1) return 0;
    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = stdin_read;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nom);
    controlador_de_interrupciones . interfaz . _io_stdin_read . direccion_fisica = direccion_fisica;
    controlador_de_interrupciones . interfaz . _io_stdin_read . bytes = (size_t) (tam >= AX && tam <= DX) ? (size_t) *(uint8_t*)registro[tam] : (size_t) *(uint32_t*)registro[tam];

    return 1;
}

int instruccion_io_stdout_write(char* nom, t_indice_registro direccion, t_indice_registro tam) {
    if (nom == NULL) return 0;

    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica == -1) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = stdout_write;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nom);
    controlador_de_interrupciones . interfaz . _io_stdout_write . direccion_fisica = direccion_fisica;
    controlador_de_interrupciones . interfaz . _io_stdout_write . bytes = (size_t) (tam >= AX && tam <= DX) ? (size_t) *(uint8_t*)registro[tam] : (size_t) *(uint32_t*)registro[tam];

    return 1;
}

int instruccion_io_fs_create(char* nombre_interfaz, char* nombre_archivo) {
    if (nombre_interfaz == NULL || nombre_archivo == NULL) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = dialfs_create;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nombre_interfaz);
    controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo = string_duplicate(nombre_archivo);

    return 1;
}

int instruccion_io_fs_delete(char* nombre_interfaz, char* nombre_archivo) {
    if (nombre_interfaz == NULL || nombre_archivo == NULL) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = dialfs_delete;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nombre_interfaz);
    controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo = string_duplicate(nombre_archivo);

    return 1;
}

int instruccion_io_fs_truncate(char* nombre_interfaz, char* nombre_archivo, t_indice_registro tam) {
    if (nombre_interfaz == NULL || nombre_archivo == NULL) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = dialfs_truncate;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nombre_interfaz);
    controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo = string_duplicate(nombre_archivo);
    controlador_de_interrupciones . interfaz . _io_dialfs . _truncate . bytes = (size_t) (tam >= AX && tam <= DX) ? (size_t) *(uint8_t*)registro[tam] : (size_t) *(uint32_t*)registro[tam];

    return 1;
}

int instruccion_io_fs_write(char* nombre_interfaz, char* nombre_archivo, t_indice_registro direccion, t_indice_registro tam, t_indice_registro puntero) {
    if (nombre_interfaz == NULL || nombre_archivo == NULL) return 0;

    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica == -1) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = dialfs_write;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nombre_interfaz);
    controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo = string_duplicate(nombre_archivo);
    controlador_de_interrupciones . interfaz . _io_dialfs . _write . bytes = (size_t) (tam >= AX && tam <= DX) ? (size_t) *(uint8_t*)registro[tam] : (size_t) *(uint32_t*)registro[tam];
    controlador_de_interrupciones . interfaz . _io_dialfs . _write . direccion_fisica = direccion_fisica;
    controlador_de_interrupciones . interfaz . _io_dialfs . _write . puntero = (int) (puntero >= AX && puntero <= DX) ? (int) *(uint8_t*)registro[puntero] : (int) *(uint32_t*)registro[puntero];

    return 1;
}

int instruccion_io_fs_read(char* nombre_interfaz, char* nombre_archivo, t_indice_registro direccion, t_indice_registro tam, t_indice_registro puntero) {
    if (nombre_interfaz == NULL || nombre_archivo == NULL) return 0;

    int direccion_fisica = mmu__calculo(pid, (direccion >= AX && direccion <= DX) ? (int) *(uint8_t*)registro[direccion] : (int) *(uint32_t*)registro[direccion]);
    if (direccion_fisica == -1) return 0;

    controlador_de_interrupciones . tipo_interrupcion = interrupcion__operacion_io;
    controlador_de_interrupciones . interfaz . tipo_operacion = dialfs_read;
    controlador_de_interrupciones . interfaz . nombre = string_duplicate(nombre_interfaz);
    controlador_de_interrupciones . interfaz . _io_dialfs . nombre_archivo = string_duplicate(nombre_archivo);
    controlador_de_interrupciones . interfaz . _io_dialfs . _read . bytes = (size_t) (tam >= AX && tam <= DX) ? (size_t) *(uint8_t*)registro[tam] : (size_t) *(uint32_t*)registro[tam];
    controlador_de_interrupciones . interfaz . _io_dialfs . _read . direccion_fisica = direccion_fisica;
    controlador_de_interrupciones . interfaz . _io_dialfs . _read . puntero = (int) (puntero >= AX && puntero <= DX) ? (int) *(uint8_t*)registro[puntero] : (int) *(uint32_t*)registro[puntero];

    return 1;
}

void instruccion_exit() {
    controlador_de_interrupciones . tipo_interrupcion = interrupcion__salida;
}
