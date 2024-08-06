#include <__es.h>

bool bloques__crear() {
    char* block_path = string_from_format("%s/%s", path_base_dialfs, "bloques.dat");
    FILE* bloques = fopen(block_path, "wb+");
    free(block_path);

    if (bloques == NULL) {
        log_error(logger, "Hubo un problema abriendo \"bloques.dat\".");
        return false;
    }

    int zero = 0;

    for (int i = 0; i < block_count * block_size; i++) fputc(zero, bloques);

    fclose(bloques);

    return true;
}

int bloques__tam() {
    char* block_path = string_from_format("%s/%s", path_base_dialfs, "bloques.dat");
    FILE* bloques = fopen(block_path, "rb+");
    free(block_path);

    if (bloques == NULL) return -1;

    fseek(bloques, 0, SEEK_END);

    int tam = ftell(bloques);

    fclose(bloques);

    return tam;
}

void bloques__abrir() {
    char* block_path = string_from_format("%s/%s", path_base_dialfs, "bloques.dat");
    int archivo_bloques = open(block_path, O_RDWR);
    free(block_path);

    bloques = malloc(sizeof(t_bloques));
    bloques -> bytes = bloques__tam();
    bloques -> contenido = mmap(NULL, bloques -> bytes, PROT_WRITE, MAP_SHARED, archivo_bloques, 0);

    close(archivo_bloques);
}

void bloques__cerrar() {
    munmap(bloques -> contenido, bloques -> bytes);
    free(bloques);
}


int bloques__cantidad_a_ocupar(int bytes) {
    if (bytes == 0) return 1;
    double result = (double) bytes / (double) block_size;
    return (int) ceil(result);
}


void bloques__escribir(void* data, int puntero, int tam) {
    memcpy(bloques -> contenido + puntero, data, tam);
    free(data);
}

void* bloques__leer(int puntero, int tam) {
    void* data = malloc(tam);
    memcpy(data, bloques -> contenido + puntero, tam);
    return data;
}
