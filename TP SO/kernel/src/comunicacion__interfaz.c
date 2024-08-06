#include <__kernel.h>

bool validar_operacion(t_interfaz* interfaz, t_operacion_io tipo_operacion) {
    bool validacion;

    t_buffer* buffer = buffer_create(kernel__validacion__interfaz);
    buffer_pack(buffer, &tipo_operacion, sizeof(t_operacion_io));
    buffer_send(buffer, interfaz -> socket);

    sem_wait(&(interfaz -> semaforo_validacion[0]));

    buffer = buffer_receive(interfaz -> socket);
    buffer_unpack(buffer, &validacion, sizeof(bool));
    buffer_destroy(buffer);

    sem_post(&(interfaz -> semaforo_validacion[1]));

    return validacion;
}
