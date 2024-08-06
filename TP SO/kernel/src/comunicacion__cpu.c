#include <__kernel.h>

void enviar_interrupcion(t_interrupcion tipo_int) {
    t_buffer* buffer = buffer_create(kernel__interrupcion__cpu_interrupt);
    buffer_pack(buffer, &tipo_int, sizeof(t_interrupcion));
    buffer_send(buffer, server__cpu_interrupt);
}

void enviar_contexto(t_pcb* pcb) {
    t_buffer* buffer = buffer_create(kernel__contexto__cpu_dispatch);           
    buffer_pack(buffer, &(pcb -> identificador), sizeof(int));
    buffer_pack(buffer, &(pcb -> registros_cpu . PC), sizeof(uint32_t));
    for (int i = 0; i < 4; i++) buffer_pack(buffer, &(pcb -> registros_cpu . _8[i]), sizeof(uint8_t));
    for (int i = 0; i < 4; i++) buffer_pack(buffer, &(pcb -> registros_cpu . _32[i]), sizeof(uint32_t));
    buffer_pack(buffer, &(pcb -> registros_cpu . SI), sizeof(uint32_t));
    buffer_pack(buffer, &(pcb -> registros_cpu . DI), sizeof(uint32_t));
    buffer_send(buffer, server__cpu_dispatch);
}
