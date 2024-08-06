#include <__memoria.h>

bool espacio__ocupar_marco(int marco) {
    if (!bitarray_test_bit(bitarray_marcos, marco)) {
        bitarray_set_bit(bitarray_marcos, marco);
        return true;
    } else return false;
}

void espacio__desocupar_marco(int marco) {
    bitarray_clean_bit(bitarray_marcos, marco);
}


void espacio__escribir(int pid, void* dato, int direccion_fisica, size_t bytes) {
    memcpy(espacio_usuario + direccion_fisica, dato, bytes);
    log_info(logger, "PID: %d - Accion: %s - Direccion fisica: %d - Tamaño: %ld", pid, "ESCRIBIR", direccion_fisica, bytes);
}

void espacio__leer(int pid, void* dato, int direccion_fisica, size_t bytes) {
    memcpy(dato, espacio_usuario + direccion_fisica, bytes);
    log_info(logger, "PID: %d - Accion: %s - Direccion fisica: %d - Tamaño: %ld", pid, "LEER", direccion_fisica, bytes);
}
