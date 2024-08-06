#include <__kernel.h>

void* pl__largo_plazo() {
    for (int i = 0; i < 4; i++) sem_wait(&sem__pl__largo_plazo);

    ac__pl__largo_plazo = ac__cpu_dispatch && ac__cpu_interrupt && ac__memoria && ac__interfaces;
    planificacion_activa = false;

    lista__new = list_create();
    lista__exit = list_create();

    while (ac__pl__largo_plazo) {
        sem_wait(&sem__pl__largo_plazo);

        if (!planificacion_activa) continue;

        pthread_mutex_lock(&mx__procesos);
        while (!list_is_empty(lista__exit)) proceso__destruir(list_remove(lista__exit, 0));
        pthread_mutex_unlock(&mx__procesos);

        pthread_mutex_lock(&mx__procesos);
        while (!list_is_empty(lista__new) && nivel_multiprogramacion < grado_multiprogramacion) aceptar_proceso(list_remove(lista__new, 0));
        sem_post(&sem__pl__corto_plazo);
        pthread_mutex_unlock(&mx__procesos);
    }

    list_destroy(lista__new);
    list_destroy(lista__exit);
    
    return NULL;
}


void first_in_first_out() {
    lista__ready[0] = list_create();
    lista__exec = list_create();

    while (ac__pl__corto_plazo) {
        sem_wait(&sem__pl__corto_plazo);

        if (!planificacion_activa) continue;

        pthread_mutex_lock(&mx__procesos);
        if (!list_is_empty(lista__ready[0]) && list_is_empty(lista__exec)) proceso__ejecutar(list_remove(lista__ready[0], 0));
        pthread_mutex_unlock(&mx__procesos);
    }

    list_destroy(lista__ready[0]);
    list_destroy(lista__exec);
}

void round_robin() {
    lista__ready[0] = list_create();
    lista__exec = list_create();

    while (ac__pl__corto_plazo) {
        sem_wait(&sem__pl__corto_plazo);

        if (!planificacion_activa) continue;

        pthread_mutex_lock(&mx__procesos);
        if (!list_is_empty(lista__ready[0]) && list_is_empty(lista__exec)) {
            pthread_t hilo_timer;
            t_pcb* pcb = list_remove(lista__ready[0], 0);
            pthread_create(&hilo_timer, NULL, (void *(*) (void *)) proceso__quantum, pcb);
            pthread_detach(hilo_timer);
            proceso__ejecutar(pcb);
        }
        pthread_mutex_unlock(&mx__procesos);
    }

    list_destroy(lista__ready[0]);
    list_destroy(lista__exec);
}

void virtual_round_robin() {
    lista__ready[0] = list_create();
    lista__ready[1] = list_create();
    lista__exec = list_create();

    while (ac__pl__corto_plazo) {
        sem_wait(&sem__pl__corto_plazo);

        if (!planificacion_activa) continue;

        pthread_mutex_lock(&mx__procesos);
        if ((!list_is_empty(lista__ready[0]) || !list_is_empty(lista__ready[1])) && list_is_empty(lista__exec)) {
            pthread_t hilo_timer;
            t_pcb* pcb = !list_is_empty(lista__ready[1]) ? list_remove(lista__ready[1], 0) : list_remove(lista__ready[0], 0);
            pthread_create(&hilo_timer, NULL, (void *(*) (void *)) proceso__quantum, pcb);
            pthread_detach(hilo_timer);
            proceso__ejecutar(pcb);
        }
        pthread_mutex_unlock(&mx__procesos);
    }

    list_destroy(lista__ready[0]);
    list_destroy(lista__ready[1]);
    list_destroy(lista__exec);
}


void* pl__corto_plazo() {
    for (int i = 0; i < 4; i++) sem_wait(&sem__pl__corto_plazo);

    ac__pl__corto_plazo = ac__cpu_dispatch && ac__cpu_interrupt && ac__memoria && ac__interfaces;

    planificador();
    
    return NULL;
}
