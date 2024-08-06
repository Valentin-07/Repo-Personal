#include <__memoria.h>

t_interfaz* interfaz__conectar(int socket) {
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));

    t_buffer* buffer = buffer_receive(socket);
    size_t len; buffer_unpack(buffer, &len, sizeof(size_t));
    interfaz -> nombre = malloc(len);
    buffer_unpack(buffer, interfaz -> nombre, len);
    interfaz -> socket = socket;
    buffer_destroy(buffer);
    for (int i = 0; i < 2; i++) sem_init(&(interfaz -> semaforo[i]), 0, 0);

    return interfaz;
}

void interfaz__desconectar(t_interfaz* interfaz) {
    buffer_send(buffer_create(terminar), interfaz -> socket);
    sem_wait(&interfaz -> semaforo[0]);
    sem_post(&interfaz -> semaforo[1]);
}
