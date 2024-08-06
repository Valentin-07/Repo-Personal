#include <__kernel.h>

void solicitar_almacenamiento_proceso(char* path) {
    t_buffer* buffer = buffer_create(kernel__creacion_de_proceso__memoria);
    buffer_pack(buffer, &asignacion_pid, sizeof(int));
    buffer_pack_string(buffer, path);
    buffer_send(buffer, server__memoria);
}

void solicitar_liberacion_proceso(int pid) {
    t_buffer* buffer = buffer_create(kernel__eliminacion_de_proceso__memoria);
    buffer_pack(buffer, &pid, sizeof(int));
    buffer_send(buffer, server__memoria);
}

bool validar_realizacion() {
    t_buffer* buffer = buffer_receive(server__memoria);

    bool validacion; buffer_unpack(buffer, &validacion, sizeof(bool));

    buffer_destroy(buffer);

    return validacion;
}
