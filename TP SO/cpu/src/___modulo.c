#include <__cpu.h>

int main(int argc, char* argv[]) {
    if (!iniciar_modulo(argc, argv[1])) return 1;
    if (!configurar_modulo()) return terminar_modulo(0);

    pthread_t th__kernel__dispatch, th__kernel__interrupt, th__memoria, th__procesamiento;

    pthread_create(&th__memoria, NULL, cx__memoria, NULL);
    pthread_create(&th__kernel__dispatch, NULL, cx__kernel__dispatch, NULL);
    pthread_create(&th__kernel__interrupt, NULL, cx__kernel__interrupt, NULL);
    pthread_create(&th__procesamiento, NULL, procesamiento, NULL);

    pthread_join(th__memoria, NULL);
    pthread_join(th__kernel__dispatch, NULL);
    pthread_join(th__kernel__interrupt, NULL);
    pthread_join(th__procesamiento, NULL);

    return terminar_modulo(1);
}
