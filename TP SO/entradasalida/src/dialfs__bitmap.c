#include <__es.h>

bool bitmap__crear() {
    char* bitmap_path = string_from_format("%s/%s", path_base_dialfs, "bitmap.dat");
    FILE* bitmap = fopen(bitmap_path, "wb+");
    free(bitmap_path);

    if (bitmap == NULL) {
        log_error(logger, "Hubo un problema abriendo \"bitmap.dat\".");
        return false;
    }

    int zero = 0;

    for (int i = 0; i < block_count / 8; i++) fputc(zero, bitmap);

    fclose(bitmap);

    return true;
}

int bitmap__tam() {
    char* bitmap_path = string_from_format("%s/%s", path_base_dialfs, "bitmap.dat");
    FILE* archivo_bitmap = fopen(bitmap_path, "rb+");
    free(bitmap_path);

    if (archivo_bitmap == NULL) return -1;

    fseek(archivo_bitmap, 0, SEEK_END);

    int tam = ftell(archivo_bitmap);

    fclose(archivo_bitmap);

    return tam;
}

void bitmap__abrir() {
    char* bitmap_path = string_from_format("%s/%s", path_base_dialfs, "bitmap.dat");
    int archivo_bitmap = open(bitmap_path, O_RDWR);
    free(bitmap_path);

    bitmap = malloc(sizeof(t_bitmap));
    bitmap -> bits = bitmap__tam() * 8;
    bitmap -> space = mmap(NULL, bitmap -> bits / 8, PROT_WRITE, MAP_SHARED, archivo_bitmap, 0);
    bitmap -> bitarray = bitarray_create_with_mode(bitmap -> space, bitmap -> bits, LSB_FIRST);

    close(archivo_bitmap);
}

void bitmap__cerrar() {
    munmap(bitmap -> space, bitmap -> bits / 8);
    bitarray_destroy(bitmap -> bitarray);
    free(bitmap);
}


void bitmap__imprimir() {
    printf("BITMAP = ");
    for (int i = 0; i < bitmap -> bits; i++) printf("%d", bitarray_test_bit(bitmap -> bitarray, i));
    printf("\n");
}



int bitmap__buscar_libre() {
    for (int i = 0; i < bitmap -> bits; i++) if (!bitarray_test_bit(bitmap -> bitarray, i)) return i;
    return -1;
}

bool bitmap__disponibilidad_contigua(int index, int count) {
    if (bitmap -> bits < index + count - 1) return false;
    for (int i = 0; i < count; i++) if (bitarray_test_bit(bitmap -> bitarray, index + i)) return false;
    return true;
}

int bitmap__cantidad_libres() {
    int c = 0;
    for (int i = 0; i < bitmap -> bits; i++) if(!bitarray_test_bit(bitmap -> bitarray, i)) c++;
    return c;
}


void bitmap__ocupar(int index) {
    bitarray_set_bit(bitmap -> bitarray, index);
}

void bitmap__desocupar(int index) {
    bitarray_clean_bit(bitmap -> bitarray, index);
}
