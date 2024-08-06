#include <__kernel.h>

int main(int argc, char* argv[]) {
    if (!iniciar_modulo(argc, argv[1])) return 1;
    if (!configurar_modulo()) return terminar_modulo(0);

    pthread_t th__cpu_dispatch, th__cpu_interrupt, th__memoria, th__interfaces, th__pl__largo_plazo, th__pl__corto_plazo, th__consola;

    pthread_create(&th__cpu_dispatch, NULL, cx__cpu_dispatch, NULL);
    pthread_create(&th__cpu_interrupt, NULL, cx__cpu_interrupt, NULL);
    pthread_create(&th__memoria, NULL, cx__memoria, NULL);
    pthread_create(&th__interfaces, NULL, cx__interfaces, NULL);
    pthread_create(&th__pl__largo_plazo, NULL, pl__largo_plazo, NULL);
    pthread_create(&th__pl__corto_plazo, NULL, pl__corto_plazo, NULL);
    pthread_create(&th__consola, NULL, consola, NULL);

    pthread_join(th__cpu_dispatch, NULL);
    pthread_join(th__cpu_interrupt, NULL);
    pthread_join(th__memoria, NULL);
    pthread_detach(th__interfaces);
    pthread_join(th__pl__largo_plazo, NULL);
    pthread_join(th__pl__corto_plazo, NULL);
    pthread_join(th__consola, NULL);

    return terminar_modulo(1);
}
