#include <__cpu.h>

void* procesamiento() {
    ac_procesamiento = true;
    modo_ejecucion = kernel;
    controlador_de_interrupciones . tipo_interrupcion = -1;

    for (t_indice_registro i = PC; i <= DI; i++) registro[i] = malloc((i >= AX && i <= DX) ? sizeof(uint8_t) : sizeof(uint32_t));

    tlb . tam = 0;
    tlb . bloque = malloc(sizeof(t_bloque_tlb) * cantidad_entradas_tlb);

    while (ac_procesamiento) {

        sem_wait(&sem__ciclo);

        if (modo_ejecucion == kernel) continue;

        char* instruccion = busqueda_instruccion();

        char** arr = decodificacion_instruccion(instruccion);

        ejecucion_instruccion(arr);

        pthread_mutex_lock(&mx__contexto);
        
        if (check_interrupt()) guardar_contexto();

        pthread_mutex_unlock(&mx__contexto);

        if (modo_ejecucion == usuario) sem_post(&sem__ciclo);
    }

    free(tlb . bloque);
    for (t_indice_registro i = PC; i <= DI; i++) free(registro[i]);

    return NULL;
}
