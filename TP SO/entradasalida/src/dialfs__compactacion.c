#include <__es.h>

bool orden_metadata(t_metadata* m1, t_metadata* m2) {
    return m1 -> bloque_inicial < m2 -> bloque_inicial;
}

t_list* obtener_archivos(char* excluido) {
    struct dirent *entry;
    DIR *dir = opendir(path_base_dialfs);

    if (dir == NULL) {
        log_error(logger, "No se pudo abrir el directorio \"%s\".", path_base_dialfs);
        return NULL;
    }
    
    t_list* metadatas = list_create();

    while ((entry = readdir(dir)) != NULL) {
        if (entry -> d_type == DT_REG && strcmp(entry -> d_name, "bitmap.dat") != 0 && strcmp(entry -> d_name, "bloques.dat") && strcmp(entry -> d_name, excluido) != 0) {
            t_metadata* metadata = metadata__abrir(string_duplicate(entry -> d_name));
            if (!metadata) {
                log_error(logger, "Hubo un error al tratar de abrir \"%s\".", entry -> d_name);
            } else {
                list_add_sorted(metadatas, metadata, (bool (*) (void*, void*)) orden_metadata);
            }
        }
    }

    closedir(dir);

    return metadatas;
}

void compactar(t_metadata* metadata_a_truncar) {
    t_list* metadatas = obtener_archivos(metadata_a_truncar -> nombre);

    int archivos_a_compactar = list_size(metadatas);

    void* data_truncado = bloques__leer(block_size * metadata_a_truncar -> bloque_inicial, metadata_a_truncar -> tam);

    int bloques_truncado = bloques__cantidad_a_ocupar(metadata_a_truncar -> tam);
    for (int i = 0; i < bloques_truncado; i++) bitmap__desocupar(metadata_a_truncar -> bloque_inicial + i);

    for (int i = 0; i < archivos_a_compactar; i++) {
        t_metadata* metadata = list_get(metadatas, i);

        void* data = bloques__leer(block_size * metadata -> bloque_inicial, metadata -> tam);

        int cantidad_bloques = bloques__cantidad_a_ocupar(metadata -> tam);
        for (int i = 0; i < cantidad_bloques; i++) bitmap__desocupar(metadata -> bloque_inicial + i);

        int bloque_libre = bitmap__buscar_libre();

        metadata -> bloque_inicial = bloque_libre;

        for (int i = 0; i < cantidad_bloques; i++) bitmap__ocupar(metadata -> bloque_inicial + i);

        bloques__escribir(data, block_size * metadata -> bloque_inicial, metadata -> tam);
    }

    list_destroy_and_destroy_elements(metadatas, (void (*) (void*)) metadata__cerrar);

    usleep((useconds_t) (tiempo_unidad_trabajo * retraso_compactacion * 1000));

    metadata_a_truncar -> bloque_inicial = bitmap__buscar_libre();
    for (int i = 0; i < bloques_truncado; i++) bitmap__ocupar(metadata_a_truncar -> bloque_inicial + i);

    bloques__escribir(data_truncado, block_size * metadata_a_truncar -> bloque_inicial, metadata_a_truncar -> tam);
}
