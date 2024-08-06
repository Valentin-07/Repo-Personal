#include <__cpu.h>

void solicitar_marco(int numero_pagina) {
    t_buffer* buffer = buffer_create(cpu__traduccion__memoria);
    buffer_pack(buffer, &pid, sizeof(int));
    buffer_pack(buffer, &numero_pagina, sizeof(int));
    buffer_send(buffer, server__memoria);
}

int recibir_marco() {
    int numero_marco;
    t_buffer* buffer = buffer_receive(server__memoria);
    buffer_unpack(buffer, &numero_marco, sizeof(int));
    buffer_destroy(buffer);
    return numero_marco;
}
