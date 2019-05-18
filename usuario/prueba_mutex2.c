/*
 * usuario/prueba_mutex.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 * Programa de usuario que realiza una prueba de los mutex
 *
 */

#include "servicios.h"

int main() {
    int desc1, desc2;

    printf("prueba_mutex 2 comienza\n");

    if ((desc1 = crear_mutex("m1", NO_RECURSIVO)) < 0)
        printf("error creando m1. NO DEBE APARECER 1\n");

    printf("\t\n----------Probamos a bloquear un mutex usando un descriptor erroneo");
    if (lock(desc1 + 1) < 0)
        printf("error en lock de mutex. DEBE APARECER 2\n");

    if ((desc2 = crear_mutex("m2", RECURSIVO)) < 0)
        printf("error creando m2. NO DEBE APARECER 3\n");
    printf("hacemos lock sobre mutex 1\n");
    if (lock(desc1) < 0)
        printf("error en lock de mutex. NO DEBE APARECER 4\n");

    printf("\t\n----------Segundo lock sobre semaforo no recursivo -> ERROR");
    if (lock(desc1) < 0)
        printf("segundo lock en mutex no recursivo. DEBE APARECER 5\n");

    if (lock(desc2) < 0)
        printf("error en lock de mutex. NO DEBE APARECER 6\n");

    /* segundo lock sobre sem�foro recursivo -> correcto */
    if (lock(desc2) < 0)
        printf("error en lock de mutex. NO DEBE APARECER 7\n");

    if (crear_proceso("mutex1") < 0)
        printf("Error creando mutex1 8\n");

    if (crear_proceso("mutex2") < 0)
        printf("Error creando mutex2 9\n");

    printf("prueba_mutex duerme 1 seg.: ejecutar�n los procesos mutex1 y mutex2 que se bloquear�n en lock de mutex\n");
    dormir(1);

    /* No debe despertar a nadie */
    if (unlock(desc2) < 0)
        printf("error en unlock de mutex. NO DEBE APARECER 10\n");

    printf("prueba_mutex duerme 1 seg.: no ejecutar� ning�n proceso ya que los mutex est�n bloqueados\n");
    dormir(1);

    /* Debe despertar al proceso mutex1 */
    if (unlock(desc2) < 0)
        printf("error en unlock de mutex. NO DEBE APARECER 11\n");

    printf("prueba_mutex duerme 1 seg.: debe ejecutar mutex1 ya que se ha liberado el mutex m2\n");
    dormir(1);

    printf("prueba_mutex termina: debe ejecutar mutex2 ya que el cierre impl�cito de m1 debe despertarlo y mutex1 est� dormido\n");

    /* cierre impl�cito de sem�foros: debe despertar a mutex2 */
    return 0;
}
