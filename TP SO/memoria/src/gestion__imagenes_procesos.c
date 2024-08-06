#include <__memoria.h>

t_list* extraer_instrucciones(char* path) {
    FILE* f = fopen(path, "r");
    if (f == NULL) return NULL;

    t_list* instrucciones = list_create();

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, f)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }

        char *instruccion = string_duplicate(line);

        list_add(instrucciones, instruccion);
    }

    free(line);

    fclose(f);

    return instrucciones;
}

t_imagen_proceso* imagen_proceso__crear(int pid, char* filename) {
    t_list* instrucciones = extraer_instrucciones(filename);

    if (instrucciones == NULL) {
        log_error(logger, "No se pudo encontrar el archivo de pseudocodigo. (%s)", filename);
        return NULL;
    } else {
        t_imagen_proceso* imagen_proceso = malloc(sizeof(t_imagen_proceso));
        imagen_proceso -> pid = pid;
        imagen_proceso -> instrucciones = instrucciones;
        imagen_proceso -> tam = 0;
        imagen_proceso -> tabla_paginas . cantidad_paginas = 0;
        imagen_proceso -> tabla_paginas . marco = NULL;
        return imagen_proceso;
    }
}

void imagen_proceso__destruir(t_imagen_proceso* imagen_proceso) {
    list_destroy_and_destroy_elements(imagen_proceso -> instrucciones, free);
    for (int i = 0; i < imagen_proceso -> tabla_paginas . cantidad_paginas; i++) espacio__desocupar_marco(imagen_proceso -> tabla_paginas . marco[i]);
    free(imagen_proceso -> tabla_paginas . marco);
    free(imagen_proceso);
}


void almacenar_proceso() {
    t_buffer* buffer = buffer_receive(client__kernel);

    int identificador; buffer_unpack(buffer, &identificador, sizeof(int));
    char* arch = buffer_unpack_string(buffer);
    
    char* path = string_from_format("%s%s", path_instrucciones, arch);
    
    t_imagen_proceso* imagen_proceso = imagen_proceso__crear(identificador, path);
    
    free(arch);
    
    bool valid = imagen_proceso != NULL;

    if (valid) {
        list_add(imagenes_procesos, imagen_proceso);

        log_info(logger, "PID: %d - Instrucciones almacenadas: %d (%s)", imagen_proceso -> pid, list_size(imagen_proceso -> instrucciones), path);
        log_info(logger, "PID: %d - Tamaño: %d", imagen_proceso -> pid, imagen_proceso -> tabla_paginas . cantidad_paginas);
    } else {
        log_error(logger, "No se pudo almacenar instrucciones para proceso %d.", identificador);
    }
    
    free(path);

    buffer_repurpose(buffer, memoria__creacion_de_proceso__kernel);
    buffer_pack(buffer, &valid, sizeof(bool));
    buffer_send(buffer, client__kernel);

}

void liberar_proceso() {
    t_buffer* buffer = buffer_receive(client__kernel);

    int identificador; buffer_unpack(buffer, &identificador, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == identificador;
    }

    t_imagen_proceso* imagen_proceso = list_remove_by_condition(imagenes_procesos, (bool (*) (void*)) _pid);

    log_info(logger, "PID: %d - Instrucciones liberada: %d", imagen_proceso -> pid, list_size(imagen_proceso -> instrucciones));
    log_info(logger, "PID: %d - Tamaño: %d", imagen_proceso -> pid, imagen_proceso -> tabla_paginas . cantidad_paginas);
    imagen_proceso__destruir(imagen_proceso);
    
    buffer_repurpose(buffer, memoria__eliminacion_de_proceso__kernel);
    buffer_send(buffer, client__kernel);
}


void enviar_instruccion() {
    t_buffer* buffer = buffer_receive(client__cpu);

    int pid;buffer_unpack(buffer, &pid, sizeof(int));
    uint32_t pc; buffer_unpack(buffer, &pc, sizeof(uint32_t));

    usleep((useconds_t) (retardo_respuesta * 1000));

     bool _pid(t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);
    
    char* instruccion_esp = list_get(imagen_proceso -> instrucciones, (int) pc);

    buffer_repurpose(buffer, memoria__instruccion__cpu);
    buffer_pack_string(buffer, instruccion_esp);
    buffer_send(buffer, client__cpu);
}


bool imagen_proceso__ajustar(t_imagen_proceso* imagen_proceso, int tam_nuevo) {
    int tam_actual = imagen_proceso -> tam;
    int cantidad_paginas_actual = imagen_proceso -> tabla_paginas . cantidad_paginas;

    int cantidad_paginas_nuevo = tam_nuevo % tam_pagina == 0 ? tam_nuevo / tam_pagina : ceil(tam_nuevo / tam_pagina) + 1;

    bool resultado;

    if (tam_nuevo > tam_actual) {
        log_info(logger, "PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d", imagen_proceso -> pid, tam_actual, tam_nuevo);

        int cantidad_paginas = 0;
        int cantidad_paginas_necesitadas = cantidad_paginas_nuevo - cantidad_paginas_actual;
        int* paginas_nuevas = malloc((size_t) (cantidad_paginas_necesitadas * sizeof(int)));
        
        for (int i = 0; cantidad_paginas < cantidad_paginas_necesitadas && i < cantidad_bits; i++) {
            if (espacio__ocupar_marco(i)) {
                paginas_nuevas[cantidad_paginas++] = i;
            }
        }

        if (cantidad_paginas != cantidad_paginas_necesitadas) {
            for (int i = 0; i < cantidad_paginas; i++) {
                espacio__desocupar_marco(paginas_nuevas[i]);
            }
            resultado = false;
        } else {
            imagen_proceso -> tabla_paginas . marco = tam_actual == 0 ?
                malloc((size_t) (cantidad_paginas_nuevo * sizeof(int))) :
                realloc(imagen_proceso -> tabla_paginas . marco, (size_t) ((cantidad_paginas_nuevo + cantidad_paginas_actual) * sizeof(int)));
            
            int j = 0;
            for (int i = cantidad_paginas_actual; i < cantidad_paginas_nuevo; i++) {
                imagen_proceso -> tabla_paginas . marco[i] = paginas_nuevas[j++];
            }

            imagen_proceso -> tam = tam_nuevo;
            imagen_proceso -> tabla_paginas . cantidad_paginas += cantidad_paginas_necesitadas;

            resultado = true;
        }

        free(paginas_nuevas);

    } else if (tam_nuevo < tam_actual) {
        log_info(logger, "PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d", imagen_proceso -> pid, tam_actual, tam_nuevo);

        int cantidad_paginas_a_liberar = cantidad_paginas_actual - cantidad_paginas_nuevo;

        while (cantidad_paginas_a_liberar > 0) {
            imagen_proceso -> tabla_paginas . cantidad_paginas--;
            espacio__desocupar_marco(imagen_proceso -> tabla_paginas . marco[imagen_proceso -> tabla_paginas . cantidad_paginas]);
            cantidad_paginas_a_liberar--;
        }

        imagen_proceso -> tam = tam_nuevo;
        imagen_proceso -> tabla_paginas . marco = realloc(imagen_proceso -> tabla_paginas . marco, (size_t) (imagen_proceso -> tabla_paginas . cantidad_paginas * sizeof(int)));

        resultado = true;
    } else {
        resultado = true;
    }

    return resultado;
}


bool imagen_proceso__escribir(t_imagen_proceso* imagen_proceso, void* dato, int direccion_fisica, size_t bytes) {
    int df = direccion_fisica, b = (int) bytes;
    int numero_marco = floor(df / tam_pagina);
    int d = df - numero_marco * tam_pagina;
    bool validar = b + d <= imagen_proceso -> tam;

    int numero_pagina = -1;
    for (int i = 0; i < imagen_proceso -> tabla_paginas . cantidad_paginas; i++) {
        if (imagen_proceso -> tabla_paginas . marco[i] == numero_marco) {
            numero_pagina = i;
            break;
        }
    }

    if (validar && numero_pagina != -1) {
        int be = b > tam_pagina - d ? tam_pagina - d : b;
        espacio__escribir(imagen_proceso -> pid, dato, df, be);
        b -= be;

        while (b > 0) {
            be = b > tam_pagina ? tam_pagina : b;

            numero_pagina++;
            numero_marco = imagen_proceso -> tabla_paginas . marco[numero_pagina];
            df = numero_marco * tam_pagina;

            espacio__escribir(imagen_proceso -> pid, (bytes - b + dato), df, be);
            b -= be;
        }

        return true;
    }

    return false;
}

bool imagen_proceso__leer(t_imagen_proceso* imagen_proceso, void* dato, int direccion_fisica, size_t bytes) {
    int df = direccion_fisica;
    int numero_marco = floor(df / tam_pagina);
    int d = df - numero_marco * tam_pagina;
    int nb = bytes + d - imagen_proceso -> tam;

    int b = nb > 0 ? bytes - nb : bytes;

    int numero_pagina = -1;
    for (int i = 0; i < imagen_proceso -> tabla_paginas . cantidad_paginas; i++) {
        if (imagen_proceso -> tabla_paginas . marco[i] == numero_marco) {
            numero_pagina = i;
            break;
        }
    }

    if (numero_pagina != -1) {
        int be = b > tam_pagina - d ? tam_pagina - d : b;
        espacio__leer(imagen_proceso -> pid, dato, df, be);
        b -= be;

        while (b > 0) {
            be = b > tam_pagina ? tam_pagina : b;

            numero_pagina++;
            numero_marco = imagen_proceso -> tabla_paginas . marco[numero_pagina];
            df = numero_marco * tam_pagina;
            
            espacio__leer(imagen_proceso -> pid, (bytes - b + dato), df, be);
            b -= be;
        }

        return true;
    }

    return false;
}
