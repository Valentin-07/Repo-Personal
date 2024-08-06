#include <__es.h>


t_metadata* metadata__crear(char* nombre) {
    int index = bitmap__buscar_libre();
    if (index == -1) {
        log_error(logger, "No hay bloques libres para archivo \"%s\".", nombre);
        return false;
    }

    char* path = string_from_format("%s/%s", path_base_dialfs, nombre);

    if (access(path, F_OK) != -1) {
        log_error(logger, "Metadata \"%s\" ya existe.", nombre);
        free(path);
        return false;
    }
    
    FILE* archivo_metadata = fopen(path, "wb+");

    free(path);

    t_metadata* metadata = malloc(sizeof(t_metadata));
    metadata -> nombre = string_duplicate(nombre); free(nombre);
    metadata -> bloque_inicial = index;
    metadata -> tam = 0;
    
    fprintf(archivo_metadata, "BLOQUE_INICIAL=%d\nTAMANIO_ARCHIVO=%d\n", metadata -> bloque_inicial, metadata -> tam);
    fclose(archivo_metadata);
    
    return metadata;
}

t_metadata* metadata__abrir(char* nombre) {
    if (!nombre) {
        log_error(logger, "Hubo un problema abriendo \"%s\".", nombre);
        return NULL;
    }

    char* path = string_from_format("%s/%s", path_base_dialfs, nombre);
    t_config* conf_metadata = config_create(path);
    free(path);

    if (!conf_metadata) {
        log_error(logger, "Hubo un problema abriendo la configuración de \"%s\".", nombre);
        return NULL;
    }
    
    t_metadata* metadata = malloc(sizeof(t_metadata));
    metadata -> nombre = string_duplicate(nombre); free(nombre);
    metadata -> bloque_inicial = config_get_int_value(conf_metadata, "BLOQUE_INICIAL");
    metadata -> tam = config_get_int_value(conf_metadata, "TAMANIO_ARCHIVO");
    config_destroy(conf_metadata);

    return metadata;
}

void metadata__cerrar(t_metadata* metadata) {
    char* path = string_from_format("%s/%s", path_base_dialfs, metadata -> nombre);
    FILE* archivo_metadata = fopen(path, "w+");
    free(path);
    
    fprintf(archivo_metadata, "BLOQUE_INICIAL=%d\nTAMANIO_ARCHIVO=%d\n", metadata -> bloque_inicial, metadata -> tam);

    fclose(archivo_metadata);

    free(metadata -> nombre);
    free(metadata);
}

bool metadata__borrar(t_metadata* metadata) {
    char* path = string_from_format("%s/%s", path_base_dialfs, metadata -> nombre);

    if (access(path, F_OK) == -1) {
        log_error(logger, "Metadata \"%s\" no existe.", metadata -> nombre);
        free(path);
        return false;
    }

    remove(path);
    
    free(path);

    free(metadata -> nombre);
    free(metadata);

    return true;
}
