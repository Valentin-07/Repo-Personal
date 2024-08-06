#include <__cpu.h>

int tlb__index_reemplazo() {
    int target;
    int first = -1;
    for (int i = 0; i < cantidad_entradas_tlb; i++) {
        if (first == -1) {
            target = 0;
            first = tlb . bloque[i] . utilizacion[algoritmo_tlb];
        } else if (tlb . bloque[i] . utilizacion[algoritmo_tlb] < first) {
            target = i;
            first = tlb . bloque[i] . utilizacion[algoritmo_tlb];
        }
    }
    return target;
}

int tlb__actualizar(int numero_pagina, int numero_marco) {
    static int _valor = 0;
    int index = tlb . tam < cantidad_entradas_tlb ? tlb . tam++ : tlb__index_reemplazo();
    tlb . bloque[index] . pid = pid;
    tlb . bloque[index] . pagina = numero_pagina;
    tlb . bloque[index] . marco = numero_marco;
    tlb . bloque[index] . utilizacion[0] = _valor++;
    return index;
}


int tlb__apuntar(int numero_pagina) {
    int index = -1;
    for (int i = 0; i < tlb . tam; i++) {
        if (tlb . bloque[i] . pid == pid && tlb . bloque[i] . pagina == numero_pagina) {
            index = i;
            break;
        }
    }
    return index;
}

int tlb__acceder_marco(int index) {
    static int _valor = 0;
    int numero_marco = tlb . bloque[index] . marco;
    tlb . bloque[index] . utilizacion[1] = _valor++;
    return numero_marco;
}

void tlb__remover_marco(int index) {
    for (int i = index; i < tlb . tam - 1; i++) tlb . bloque[index] = tlb . bloque[index + 1];
}
