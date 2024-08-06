#include <__memoria.h>

void operacion_cpu() {
    t_buffer* buffer = buffer_receive(client__cpu);

    t_operacion_memoria operacion; buffer_unpack(buffer, &operacion, sizeof(t_operacion_memoria));

    usleep((useconds_t) (retardo_respuesta * 1000));

    switch (operacion) {
        case mem__resize:
            redimensionar(buffer);
            break;
        case mem__write:
            cpu_escribir(buffer);
            break;
        case mem__read:
            cpu_leer(buffer);
            break;
        case mem__copy_string:
            cpu_copiar(buffer);
            break;
    }
}