#include <__cpu.h>

int mmu__calculo(int pid, int direccion_logica) {
    int numero_pagina = (int) (direccion_logica / tam_pagina);
    int offset = direccion_logica - numero_pagina * tam_pagina;
    int direccion_fisica = -1, numero_marco;

    if (cantidad_entradas_tlb != 0) {
        int index = tlb__apuntar(numero_pagina);

        if (index != -1) {
            log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pid, numero_pagina);
            numero_marco = tlb__acceder_marco(index);
            direccion_fisica = numero_marco * tam_pagina + offset;
        } else {
            log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pid, numero_pagina);

            solicitar_marco(numero_pagina);

            sem_wait(&sem__marco[0]);

            numero_marco = recibir_marco();
            
            sem_post(&sem__marco[1]);

            if (numero_marco != -1) {
                index = tlb__actualizar(numero_pagina, numero_marco);
                numero_marco = tlb__acceder_marco(index);
                direccion_fisica = numero_marco * tam_pagina + offset;
                log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numero_pagina, numero_marco);
            }
        }
    } else {
        solicitar_marco(numero_pagina);

        sem_wait(&sem__marco[0]);

        numero_marco = recibir_marco();
            
        sem_post(&sem__marco[1]);

        if (numero_marco != -1) {
            direccion_fisica = numero_marco * tam_pagina + offset;
            log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numero_pagina, numero_marco);
        }
    }

    

    return direccion_fisica;
}
