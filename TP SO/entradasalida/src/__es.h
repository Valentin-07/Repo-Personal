#ifndef ES_H_
#define ES_H_

#include <utils/bufferlib.h>
#include <utils/operaciones_io.h>
#include <utils/operaciones_memoria.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>

#define required_arg_count 3

extern t_log* logger;
extern t_config* config;
extern char* nombre_interfaz;

int iniciar_modulo(int, char*, char*);
int configurar_modulo();
int terminar_modulo(int);


// Estructuras.

typedef enum {io_generica, io_stdin, io_stdout, io_dialfs} t_tipo_interfaz;

typedef struct {
    char* space;
    t_bitarray* bitarray;
    int bits;
} t_bitmap;

typedef struct {
    char* contenido;
    int bytes;
} t_bloques;

typedef struct {
    char* nombre;
    int bloque_inicial;
    int tam;
} t_metadata;


// Variables de configuración.

extern t_tipo_interfaz  tipo_interfaz;
extern int              tiempo_unidad_trabajo;
extern char*            ip_kernel;
extern char*            puerto_kernel;
extern char*            ip_memoria;
extern char*            puerto_memoria;
extern char*            path_base_dialfs;
extern int              block_size;
extern int              block_count;
extern int              retraso_compactacion;



// Conexiones.

extern int server__kernel;
extern int server__memoria;

extern bool ac__kernel, ac__memoria;

void* cx__kernel();
void* cx__memoria();



// DialFS - Archivos.

t_metadata* metadata__crear(char*);
t_metadata* metadata__abrir(char*);
void metadata__cerrar(t_metadata*);
bool metadata__borrar(t_metadata*);


// DialFS - Bitmap.

extern t_bitmap* bitmap;

bool bitmap__crear();
int bitmap__tam();
void bitmap__abrir();
void bitmap__cerrar();

void bitmap__imprimir();


int bitmap__buscar_libre();
bool bitmap__disponibilidad_contigua(int, int);
int bitmap__cantidad_libres();
void bitmap__ocupar(int);
void bitmap__desocupar(int);


// DialFS - Bloques.

extern t_bloques* bloques;

bool bloques__crear();
int bloques__tam();
void bloques__abrir();
void bloques__cerrar();

int bloques__cantidad_a_ocupar(int);

void bloques__escribir(void*, int, int);
void* bloques__leer(int, int);


// DialFS - Compactación.

void compactar(t_metadata*);


// Operaciones.

extern sem_t sem__operaciones[2];

extern sem_t sem__memoria[2];

void validar_operacion();
void* ejecutar_operacion();


#endif