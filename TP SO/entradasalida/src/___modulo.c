#include <__es.h>

int main(int argc, char* argv[]) {
    if (!iniciar_modulo(argc, argv[1], argv[2])) return 1;
    if (!configurar_modulo()) return terminar_modulo(0);

    pthread_t th__kernel, th__memoria;

    pthread_create(&th__kernel, NULL, cx__kernel, NULL);
    pthread_create(&th__memoria, NULL, cx__memoria, NULL);

    pthread_join(th__kernel, NULL);
    pthread_join(th__memoria, NULL);

    return terminar_modulo(1);
}
