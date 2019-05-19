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
 ********************    DEFINITION    ********************
 **********************************************************
 */

#define NO_RECURSIVO 0
#define RECURSIVO 1


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
    int intSistema;            /* interrupciones en modo sistema */
    int intUsuario;            /* interrupciones en modo usuario */
    int nMutex;                /* Contador del numero de mutex */
    int mutexBlock;            /* Flag bloqueado por mutex */
    int readBlock;             /* Flag bloqueado por lectura de caracter*/
    int mutexList[NUM_MUT_PROC];
    int mutex_id;

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

/*
 * Definicion de la estructura que almacena informacion de cuantas veces se esta ejecutando en usuario y sistema
 */
struct tiempos_ejec {
    int usuario;
    int sistema;
};

typedef struct mutex_t *mutex_ptr;

typedef struct mutex_t {
    int index;
    char nombre[MAX_NOM_MUT];
    int tipo;
    int proceso_bloqueado;
    int num_procesos;
    mutex_ptr siguiente;

} mutex;

typedef struct {
    mutex *primero;
    mutex *ultimo;
} lista_Mutex;

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

/*
 * Variable global flag para evitar panico cuando se accede erroneamente
 */
int memAccess;

/*
 * Lista global con los mutex que hay disponibles
 */
lista_Mutex lista_mutex = {NULL, NULL};

/*
 * Variable global que lleva el contador del numero de mutex en el sistema
 */

int cont_mutex = 0;

/*
 * Variable global generadora de indices de mutex
 */

int cont_mutex_index = 0;

/*
  * Variable global numero de caracteres en buffer
  */
int size_buffer = 0;


/*
 * Buffer de caracteres procesados del terminal
 */
char buffer[TAM_BUF_TERM];

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

int sis_tiempos_proceso();

int sis_crear_mutex();

int sis_abrir_mutex();

int sis_lock();

int sis_unlock();

int sis_cerrar_mutex();

int sis_leer_caracter();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS] = {{sis_crear_proceso},
                                        {sis_terminar_proceso},
                                        {sis_escribir},
                                        {sis_nueva},
                                        {sis_dormir},
                                        {sis_tiempos_proceso},
                                        {sis_crear_mutex},
                                        {sis_abrir_mutex},
                                        {sis_lock},
                                        {sis_unlock},
                                        {sis_cerrar_mutex},
                                        {sis_leer_caracter}
};

#endif /* _KERNEL_H */

