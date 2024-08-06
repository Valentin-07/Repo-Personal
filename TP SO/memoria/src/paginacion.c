#include <__memoria.h>


int acceso_tabla_pagina(t_imagen_proceso* imagen_proceso, int numero_pagina) {
    return numero_pagina < imagen_proceso -> tabla_paginas . cantidad_paginas ? imagen_proceso -> tabla_paginas . marco[numero_pagina] : -1;
}

void obtener_marco() {
    t_buffer* buffer = buffer_receive(client__cpu);

    int identificador; buffer_unpack(buffer, &identificador, sizeof(int));
    int numero_pagina; buffer_unpack(buffer, &numero_pagina, sizeof(int));

    usleep((useconds_t) (retardo_respuesta * 1000));

    bool _pid(t_imagen_proceso* _imagen_proceso) {
        return _imagen_proceso -> pid == identificador;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    int marco = acceso_tabla_pagina(imagen_proceso, numero_pagina);
    if (marco != -1) log_info(logger, "PID: %d - Pagina: %d - Marco: %d", imagen_proceso -> pid, numero_pagina, marco);
    else log_error(logger, "Proceso %d no encontrado o pagina %d no incluida en tabla.", identificador, numero_pagina);

    buffer_repurpose(buffer, memoria__traduccion__cpu);
    buffer_pack(buffer, &marco, sizeof(int));
    buffer_send(buffer, client__cpu);
}


void redimensionar(t_buffer* buffer) {
    int identificador; buffer_unpack(buffer, &identificador, sizeof(int));
    int tamanio; buffer_unpack(buffer, &tamanio, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == identificador;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    bool resultado = imagen_proceso__ajustar(imagen_proceso, tamanio);

    buffer_repurpose(buffer, memoria__operacion__cpu);
    buffer_pack(buffer, &resultado, sizeof(bool));
    buffer_send(buffer, client__cpu);
}


void cpu_escribir(t_buffer* buffer) {
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    void* valor = malloc(bytes); buffer_unpack(buffer, valor, bytes);
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    bool resultado = imagen_proceso__escribir(imagen_proceso, valor, direccion_fisica, bytes);

    buffer_repurpose(buffer, memoria__operacion__cpu);
    buffer_pack(buffer, &resultado, sizeof(bool));
    if (resultado) buffer_pack(buffer, valor, bytes);
    buffer_send(buffer, client__cpu);
    free(valor);
}

void cpu_leer(t_buffer* buffer) {
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    void* valor = malloc(bytes);
    memset(valor, 0, bytes);
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    bool resultado = imagen_proceso__leer(imagen_proceso, valor, direccion_fisica, bytes);

    buffer_repurpose(buffer, memoria__operacion__cpu);
    buffer_pack(buffer, &resultado, sizeof(bool));
    if (resultado) buffer_pack(buffer, valor, bytes);
    buffer_send(buffer, client__cpu);
    free(valor);
}

void cpu_copiar(t_buffer* buffer) {
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    int size; buffer_unpack(buffer, &size, sizeof(int));
    void* valor = malloc(size);
    memset(valor, 0, size);
    int direccion_fisica_s; buffer_unpack(buffer, &direccion_fisica_s, sizeof(int));
    int direccion_fisica_d; buffer_unpack(buffer, &direccion_fisica_d, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    bool resultado = imagen_proceso__leer(imagen_proceso, valor, direccion_fisica_s, size);
    if (resultado) resultado = imagen_proceso__escribir(imagen_proceso, valor, direccion_fisica_d, size);

    buffer_repurpose(buffer, memoria__operacion__cpu);
    buffer_pack(buffer, &resultado, sizeof(bool));
    buffer_send(buffer, client__cpu);
    free(valor);
}


void io_escribir(t_interfaz* interfaz, t_buffer* buffer) {
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    void* data = malloc(bytes); buffer_unpack(buffer, data, bytes);
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    bool resultado = imagen_proceso__escribir(imagen_proceso, data, direccion_fisica, bytes);

    buffer_repurpose(buffer, memoria__operacion__interfaz);
    buffer_pack(buffer, &resultado, sizeof(bool));
    buffer_send(buffer, interfaz -> socket);
    free(data);
}

void io_leer(t_interfaz* interfaz, t_buffer* buffer) {
    int pid; buffer_unpack(buffer, &pid, sizeof(int));
    size_t bytes; buffer_unpack(buffer, &bytes, sizeof(size_t));
    int direccion_fisica; buffer_unpack(buffer, &direccion_fisica, sizeof(int));

    bool _pid (t_imagen_proceso* imagen_proceso) {
        return imagen_proceso -> pid == pid;
    }

    t_imagen_proceso* imagen_proceso = list_find(imagenes_procesos, (bool (*) (void*)) _pid);

    void* data = malloc(bytes); // LOSS HERE
    bool resultado = imagen_proceso__leer(imagen_proceso, data, direccion_fisica, bytes);
    
    buffer_repurpose(buffer, memoria__operacion__interfaz);
    buffer_pack(buffer, &resultado, sizeof(bool));
    if (resultado) {
        char* text = malloc(bytes + 1);
        strncpy(text, data, bytes);
        text[bytes] = '\0';
        free(data);
        buffer_pack_string(buffer, text);
        free(text);
    }
    buffer_send(buffer, interfaz -> socket);
}
