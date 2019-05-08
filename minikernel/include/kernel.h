/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"


/**********************************************************
 ********************    STRUCTURES    ********************
 **********************************************************
 */
/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
    int id;                     /* ident. del proceso */
    int estado;                 /* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
    int nSegBlocked;            /* Numero de segundos que el proceso estara bloqueado */
    int startBlockAt;           /* Numero de segundos que el proceso estara bloqueado */
    contexto_t contexto_regs;   /* copia de regs. de UCP */
    void *pila;                 /* dir. inicial de la pila */
    BCPptr siguiente;           /* puntero a otro BCP */
    void *info_mem;             /* descriptor del mapa de memoria */
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct {
    BCP *primero;
    BCP *ultimo;
} lista_BCPs;


/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct {
    int (*fservicio)();
} servicio;


/**********************************************************
 ******************** GLOBAL VARIABLES ********************
 **********************************************************
 */

/*
 * Variable global que identifica el proceso actual
 */

BCP *p_proc_actual = NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];


/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos = {NULL, NULL};


/*
 * Variable global que representa la cola de procesos bloqueados
 */
lista_BCPs lista_blocked = {NULL, NULL};

/*
 * Variable global contador de llamadas al int_reloj
 */
int int_clock_counter = 0;

/**********************************************************
 ********************     ROUTINES     ********************
 **********************************************************
 */

/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();

int sis_terminar_proceso();

int sis_escribir();

int sis_nueva();

int sis_dormir();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS] = {{sis_crear_proceso},
                                        {sis_terminar_proceso},
                                        {sis_escribir},
                                        {sis_nueva},
                                        {sis_dormir}};

#endif /* _KERNEL_H */

