#include <__memoria.h>

void operacion_interfaz(t_interfaz* interfaz) {
    t_buffer* buffer = buffer_receive(interfaz -> socket);

    t_operacion_memoria operacion; buffer_unpack(buffer, &operacion, sizeof(t_operacion_memoria));

    usleep((useconds_t) (retardo_respuesta * 1000));

    switch (operacion) {
        case mem__write:
            io_escribir(interfaz, buffer);
            break;
        case mem__read:
            io_leer(interfaz, buffer);
            break;
        default: break;
    }
}
