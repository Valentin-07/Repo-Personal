#include <__memoria.h>

int main(int argc, char* argv[]) {
    if (!iniciar_modulo(argc, argv[1])) return 1;
    if (!configurar_modulo()) terminar_modulo(0);

    pthread_t th__cpu, th__kernel, th__interfaces;

    pthread_create(&th__cpu, NULL, cx__cpu, NULL);
    pthread_create(&th__kernel, NULL, cx__kernel, NULL);
    pthread_create(&th__interfaces, NULL, cx__interfaces, NULL);

    pthread_join(th__cpu, NULL);
    pthread_join(th__kernel, NULL);
    pthread_detach(th__interfaces);

    return terminar_modulo(1);
}
