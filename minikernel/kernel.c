/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include <stdbool.h>
#include "kernel.h"    /* Contiene defs. usadas por este modulo */
#include "string.h"
#include <stdlib.h>


/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */


bool verificaCondiciones(const char *nombre);

bool nombreMutexRepetido(const char *nombre);


void anadirProcesoAListaBloqueados(BCP *proc);


void insertar_mutex(lista_Mutex *lista, mutex *pMutex);


void crearMutex(char *nombre, int tipo);

int getDescriptor();

int getMutexId(const char *nombre);

void eliminar_mutex(lista_Mutex *lista, int mutex_id);

void eliminar_primero_mutex(lista_Mutex *lista);

mutex *getMutex(lista_Mutex *lista, int mutex_id);

void eliminarProcesoListaBloqueados(BCP *proceso_bloqueado);

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc() {
    int i;

    for (i = 0; i < MAX_PROC; i++)
        tabla_procs[i].estado = NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre() {
    int i;

    for (i = 0; i < MAX_PROC; i++)
        if (tabla_procs[i].estado == NO_USADA)
            return i;
    return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP *proc) {
    if (lista->primero == NULL)
        lista->primero = proc;
    else
        lista->ultimo->siguiente = proc;
    lista->ultimo = proc;
    proc->siguiente = NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista) {

    if (lista->ultimo == lista->primero)
        lista->ultimo = NULL;
    lista->primero = lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP *proc) {
    BCP *paux = lista->primero;

    if (paux == proc)
        eliminar_primero(lista);
    else {
        for (; ((paux) && (paux->siguiente != proc));
               paux = paux->siguiente);
        if (paux) {
            if (lista->ultimo == paux->siguiente)
                lista->ultimo = paux;
            paux->siguiente = paux->siguiente->siguiente;
        }
    }
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int() {
    int nivel;

    //printk("-> NO HAY LISTOS. ESPERA INT\n");

    /* Baja al m�nimo el nivel de interrupci�n mientras espera */
    nivel = fijar_nivel_int(NIVEL_1);
    halt();
    fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP *planificador() {
    while (lista_listos.primero == NULL)
        espera_int();        /* No hay nada que hacer */
    return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso() {

    printk("\n\n-> LIBERAR PROCESO\n");

    int i;
    for (i = 0; i < NUM_MUT_PROC; i++) {
        printf("******************** BUSCAMOS MUTEX DE ESTE PROCESO %d\n", p_proc_actual->id);
        int mutex_id;
        if ((mutex_id = p_proc_actual->mutexList[i]) == -1)continue;
        printf("******************** MUTEX ID ENCONTRADO %d\n", mutex_id);
        printf("******************** BUSCAMOS MUTEX EN LA LISTA\n");
        mutex *mutex1 = getMutex(&lista_mutex, mutex_id);
        printf("******************** MUTEX ENCONTRADO %s\n", mutex1->nombre);
        if (mutex1->index != -1 && mutex1->proceso_bloqueado == p_proc_actual->id) {
            printf("******************** DESBLOQUEAMOS MUTEX %d\n", mutex_id);
            mutex1->proceso_bloqueado = -1;
            mutex1->num_procesos--;
            printf("******************** TIENE %d PROCESOS MAS\n", mutex1->num_procesos);
            if (mutex1->num_procesos > 0)continue;
            eliminar_mutex(&lista_mutex, mutex1->index);
            strcpy(mutex1->nombre, "");
            cont_mutex--;
        }
        printf("******************** BUSCAMOS PROCESOS BLOQUEADOS\n");
        BCP *proceso_bloqueado = lista_blocked.primero;
        bool encontrado = false;
        while (proceso_bloqueado != NULL && !encontrado) {
            printf("******************** BUSCAMOS PROCESO BLOQUEADO POR MUTEX\n");
            if (proceso_bloqueado->estado == BLOQUEADO
                && proceso_bloqueado->mutex_id == mutex_id) {
                printf("******************** PROCESO ENCONTRADO: %d\n", proceso_bloqueado->id);
                proceso_bloqueado->estado = LISTO;
                proceso_bloqueado->mutexBlock = 0;
                eliminarProcesoListaBloqueados(proceso_bloqueado);
                encontrado = true;
            }
            proceso_bloqueado = proceso_bloqueado->siguiente;
        }


        proceso_bloqueado = lista_blocked.primero;
        while (proceso_bloqueado != NULL) {
            if (proceso_bloqueado->estado == BLOQUEADO
                && proceso_bloqueado->mutexBlock == 1) {
                proceso_bloqueado->estado = LISTO;
                proceso_bloqueado->mutexBlock = 0;
                eliminarProcesoListaBloqueados(proceso_bloqueado);
                break;
            }
            proceso_bloqueado = proceso_bloqueado->siguiente;
        }

    }
    printf("******************** PROCESADOS MUTEX DE ESTE PROCESO %d\n", p_proc_actual->id);

    BCP *p_proc_anterior;

    liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

    p_proc_actual->estado = TERMINADO;
    eliminar_primero(&lista_listos); /* proc. fuera de listos */

    /* Realizar cambio de contexto */
    p_proc_anterior = p_proc_actual;
    p_proc_actual = planificador();

    printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
           p_proc_anterior->id, p_proc_actual->id);

    liberar_pila(p_proc_anterior->pila);
    cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit() {

    if (!viene_de_modo_usuario())
        panico("excepcion aritmetica cuando estaba dentro del kernel");


    printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
    liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem() {

    if (!viene_de_modo_usuario() && memAccess == 0)
        panico("excepcion de memoria cuando estaba dentro del kernel");


    printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
    liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal() {
    char car;

    car = leer_puerto(DIR_TERMINAL);
    printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

    if (size_buffer >= TAM_BUF_TERM) {
        printf("TAMANO DEL BUFFER EXCEDIDO %d\n", size_buffer);
        return;
    }

    printf("METEMOS EN BUFFER\n");
    buffer[size_buffer] = car;
    size_buffer++;

    BCP *proc_blocked = lista_blocked.primero;
    bool desbloqueado = false;


    if (proc_blocked != NULL && proc_blocked->readBlock == 1) {
        printf("PRIMERO BLOQUEADO POR LECTURA, DESBLOQUEAMOS\n");
        desbloqueado = true;
        proc_blocked->estado = LISTO;
        proc_blocked->readBlock = 0;

        int int_level = fijar_nivel_int(NIVEL_3);
        eliminar_elem(&lista_blocked, proc_blocked);
        insertar_ultimo(&lista_listos, proc_blocked);
        fijar_nivel_int(int_level);

    }

    while (!desbloqueado && proc_blocked != lista_blocked.ultimo) {
        printf("SEGUIMOS DESBLOQUEANDO\n");
        proc_blocked = proc_blocked->siguiente;
        if (proc_blocked->readBlock == 1) {

            desbloqueado = true;
            proc_blocked->estado = LISTO;
            proc_blocked->readBlock = 0;

            printf("CAMBIO DE CONTEXTO\n");
            int int_level = fijar_nivel_int(NIVEL_3);
            eliminar_elem(&lista_blocked, proc_blocked);
            insertar_ultimo(&lista_listos, proc_blocked);
            fijar_nivel_int(int_level);

        }
    }


    return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj() {

    //printk("\n\n-> TRATANDO INT. DE RELOJ\n");
    int_clock_counter++;
    if (lista_listos.primero != NULL) {
        if (viene_de_modo_usuario())p_proc_actual->intUsuario++;
        else p_proc_actual->intSistema++;
    }

    BCP *first_blocked = lista_blocked.primero;
    while (first_blocked != NULL) {
        //printf("******************** HAY PROCESO BLOCKED (%d)\n", first_blocked->id);
        int seg_left =
                (first_blocked->nSegBlocked * TICK) - (int_clock_counter - first_blocked->startBlockAt);
        BCPptr next_blocked = first_blocked->siguiente;
        // printf("******************** LE QUEDAN (%d)\n", seg_left);
        if (seg_left <= 0 &&
            first_blocked->mutexBlock != 1) {
            //  printf("******************** DESBLOQUEAMOS \n");
            first_blocked->estado = LISTO;
            eliminarProcesoListaBloqueados(first_blocked);
            // printf("******************** DONE \n");

        }
        //  printf("******************** PASAMOS SIGUIENTE\n");

        first_blocked = next_blocked;
    }
    // printf("******************** NO HAY PROCESO BLOCKED\n\n");
    return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis() {
    int nserv, res;

    nserv = leer_registro(0);
    if (nserv < NSERVICIOS)
        res = (tabla_servicios[nserv].fservicio)();
    else
        res = -1;        /* servicio no existente */
    escribir_registro(0, res);
    return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw() {

    printk("-> TRATANDO INT. SW\n");

    return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog) {
    void *imagen, *pc_inicial;
    int error = 0;
    int proc;
    BCP *p_proc;

    proc = buscar_BCP_libre();
    if (proc == -1)
        return -1;    /* no hay entrada libre */

    /* A rellenar el BCP ... */
    p_proc = &(tabla_procs[proc]);

    /* crea la imagen de memoria leyendo ejecutable */
    imagen = crear_imagen(prog, &pc_inicial);
    if (imagen) {
        p_proc->info_mem = imagen;
        p_proc->pila = crear_pila(TAM_PILA);
        fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
                           pc_inicial,
                           &(p_proc->contexto_regs));
        p_proc->id = proc;
        p_proc->estado = LISTO;
        p_proc->nMutex = 0;
        p_proc->mutexBlock = 0;
        p_proc->readBlock = 0;
        p_proc->mutex_id = -1;
        int i;
        for (i = 0; i < NUM_MUT_PROC; i++) {
            p_proc->mutexList[i] = -1;
        }
        /* lo inserta al final de cola de listos */
        insertar_ultimo(&lista_listos, p_proc);
        error = 0;
    } else
        error = -1; /* fallo al crear imagen */

    return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso() {
    char *prog;
    int res;

    printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
    prog = (char *) leer_registro(1);
    res = crear_tarea(prog);
    return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir() {
    char *texto;
    unsigned int longi;

    texto = (char *) leer_registro(1);
    longi = (unsigned int) leer_registro(2);

    escribir_ker(texto, longi);
    return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso() {

    printk("-> FIN PROCESO %d\n", p_proc_actual->id);

    liberar_proceso();

    return 0; /* no deber�a llegar aqui */
}

int sis_nueva() {
    return p_proc_actual->id;
}

int sis_dormir() {

    unsigned int seg = (unsigned int) leer_registro(1);
    //printf("******************** Dormir: (%d) segundos \n", seg);

    p_proc_actual->estado = BLOQUEADO;
    p_proc_actual->nSegBlocked = seg;
    p_proc_actual->startBlockAt = int_clock_counter;

    anadirProcesoAListaBloqueados(p_proc_actual);

    BCP *p_proc_blocked = p_proc_actual;
    p_proc_actual = planificador();
    cambio_contexto(&(p_proc_blocked->contexto_regs), &(p_proc_actual->contexto_regs));


    return 0;

}

int sis_tiempos_proceso() {
    struct tiempos_ejec *t_ejec = (struct tiempos_ejec *) leer_registro(1);
    if (t_ejec == NULL)
        return int_clock_counter;

    int int_level = fijar_nivel_int(NIVEL_3);
    memAccess = 1;
    fijar_nivel_int(int_level);

    t_ejec->usuario = p_proc_actual->intUsuario;
    t_ejec->sistema = p_proc_actual->intSistema;


    return int_clock_counter;
}

int sis_crear_mutex() {

    char *nombre = (char *) leer_registro(1);
    int tipo = (int) leer_registro(2);

    printf("\n\n******************** PROCEDEMOS A CREAR EL MUTEX %s DEL TIPO %d\n", nombre, tipo);

    if (!verificaCondiciones(nombre)) {
        printf("******************** ERROR: NO SE HA VERIFICADO LAS CONDICIONES NECESARIAS\n");
        return -1;
    }

    printf("******************** HAY %d mutex en el sistema\n", cont_mutex);

    while (cont_mutex >= NUM_MUT) {
        // printf("******************** NUMERO MAXIMO DE MUTEX EN EL SISTEMA EXCEDIDO\n");

        p_proc_actual->estado = BLOQUEADO;
        p_proc_actual->mutexBlock = 1;
        anadirProcesoAListaBloqueados(p_proc_actual);

        printf("******************** CAMBIAMOS CONTEXTO DEL PROC %d\n", p_proc_actual->id);
        BCP *p_proc_blocked = p_proc_actual;
        p_proc_actual = planificador();
        cambio_contexto(&(p_proc_blocked->contexto_regs), &(p_proc_actual->contexto_regs));

        printf("******************** VOLVEMOS A VERIFICAR NOMBRE %s DEL PROC %d\n", nombre, p_proc_actual->id);
        if (!verificaCondiciones(nombre))return -1;
    }
    printf("******************** NUMERO MAXIMO DE MUTEX EN EL SISTEMA OK\n");

    crearMutex(nombre, tipo);
    int descriptor = getDescriptor();
    printf("******************** ASIGNAMOS DESCRIPTOR %d al proceso %d\n", descriptor, p_proc_actual->id);
    p_proc_actual->mutexList[descriptor] = cont_mutex_index++;
    cont_mutex++;

    printf("******************** FIN CREAR MUTEX\n\n");
    return p_proc_actual->nMutex++;


}

int sis_abrir_mutex() {

    char *nombre = (char *) leer_registro(1);
    printf("\n\n------------- EMPEZAMOS A ABRIR MUTEX %s\n", nombre);
    if (p_proc_actual->nMutex >= NUM_MUT_PROC)return -1;
    printf("------------- buscamos id del mutex\n");
    int mutex_id = getMutexId(nombre);
    if (mutex_id == -1) {
        printf("------------- ERROR: id no encontrado\n");
        return -1;
    }
    printf("------------- encontrado id: %d\n", mutex_id);
    int descriptor = getDescriptor();
    printf("------------- asignamos descriptor %d a proceso %d\n", descriptor, p_proc_actual->id);
    p_proc_actual->mutexList[descriptor] = mutex_id;
    mutex *mutex1 = getMutex(&lista_mutex, mutex_id);
    mutex1->num_procesos++;
    printf("******************** FIN ABRIR MUTEX\n\n");
    return p_proc_actual->nMutex++;
}

int sis_lock() {
    unsigned int descriptor = (unsigned int) leer_registro(1);
    int mutex_id;
    printf("\n\n-------> EMPEZAMOS A BLOQUEAR MUTEX %d\n", descriptor);
    if (descriptor > NUM_MUT_PROC - 1
        || (mutex_id = p_proc_actual->mutexList[descriptor]) == -1)
        return -1;
    printf("-------> OBTENEMOS EL MUTEX CON INDEX %d\n", mutex_id);
    mutex *mutex1 = getMutex(&lista_mutex, mutex_id);

    while (mutex1->index != -1 && mutex1->proceso_bloqueado != p_proc_actual->id
           && mutex1->proceso_bloqueado != -1) {
        // printf("-------> EL MUTEX %d ESTA CERRADO, BLOQUEAMOS PROCESO %d\n", p_proc_actual->id);
        p_proc_actual->estado = BLOQUEADO;
        p_proc_actual->mutex_id = mutex_id;
        // printf("-------> BLOQUEAMOS\n");
        anadirProcesoAListaBloqueados(p_proc_actual);
        // printf("-------> CAMBIO DE CONTEXTO\n");
        BCP *p_proc_blocked = p_proc_actual;
        p_proc_actual = planificador();
        cambio_contexto(&(p_proc_blocked->contexto_regs), &(p_proc_actual->contexto_regs));
    }
    printf("-------> EL MUTEX %d NO ESTA CERRADO\n", mutex_id);

    printf("-------> COMPROBAMOS CONDICIONES\n");
    if (mutex1->index != -1 && mutex1->tipo == NO_RECURSIVO && mutex1->num_procesos == 1) {
        printf("-------> ERROR: MUTEX NO RECURSIVO CON NUMERO DE PROCESOS %d\n", mutex1->num_procesos);
        return -1;
    }
    printf("Mutex %d tiene asignado %d", mutex1->index, mutex1->proceso_bloqueado);
    printf("asignamos mutex %d a proceso %d", mutex_id, p_proc_actual->id);
    mutex1->proceso_bloqueado = p_proc_actual->id;
    mutex1->num_procesos++;
    return 0;
}


int sis_unlock() {

    unsigned int descriptor = (unsigned int) leer_registro(1);
    int mutex_id;
    printf("\n\n-------> EMPEZAMOS A DESBLOQUEAR MUTEX %d\n", descriptor);
    if (descriptor > NUM_MUT_PROC - 1
        || (mutex_id = p_proc_actual->mutexList[descriptor]) == -1)
        return -1;
    printf("-------> OBTENEMOS EL MUTEX CON INDEX %d\n", mutex_id);
    mutex *mutex1 = getMutex(&lista_mutex, mutex_id);
    if (mutex1->index == -1 || mutex1->proceso_bloqueado != p_proc_actual->id)
        return -1;
    mutex1->num_procesos--;
    if (mutex1->num_procesos != 0) {
        printf("-------> MUTEX RECURSIVO BLOQUEADO POR %d PROCESOS MAS\n", mutex1->num_procesos);
        return 0;
    }
    printf("-------> MUTEX DESBLOQUEADO\n");
    mutex1->proceso_bloqueado = -1;

    BCP *proceso_bloqueado = lista_blocked.primero;
    bool encontrado = false;

    while (proceso_bloqueado != NULL && !encontrado) {
        BCP *proceso_siguiente = proceso_bloqueado->siguiente;
        if (proceso_bloqueado->estado == BLOQUEADO
            && proceso_bloqueado->mutex_id == mutex_id) {
            proceso_bloqueado->estado = LISTO;
            proceso_bloqueado->mutex_id = -1;
            proceso_bloqueado->mutexBlock = 0;

            eliminarProcesoListaBloqueados(proceso_bloqueado);

            encontrado = true;
        }
        proceso_bloqueado = proceso_siguiente;
    }
    return 0;
}


int sis_cerrar_mutex() {

    unsigned int descriptor = (unsigned int) leer_registro(1);
    int mutex_id;
    printf("\n\n::::::::::::EMPEZAMOS A CERRAR MUTEX %d\n", descriptor);
    if (descriptor > NUM_MUT_PROC - 1
        || (mutex_id = p_proc_actual->mutexList[descriptor]) == -1)
        return -1;
    printf("\n\n::::::::::::PROCEDEMOS A ELEMINAR MUTEX %d PROCESO %d\n", mutex_id, p_proc_actual->id);
    mutex *mutex1 = getMutex(&lista_mutex, mutex_id);
    if (mutex1->index == -1)
        return -1;
    mutex1->num_procesos--;
    p_proc_actual->nMutex--;
    p_proc_actual->mutexList[descriptor] = -1;

    printf("::::::::::::BUSCAMOS PROCESOS BLOQUEADOS\n");
    BCP *proceso_bloqueado = lista_blocked.primero;
    bool encontrado = false;
    while (proceso_bloqueado != NULL && !encontrado) {
        printf("::::::::::::BUSCAMOS PROCESO BLOQUEADO POR MUTEX\n");
        BCP *proceso_siguiente = proceso_bloqueado->siguiente;
        if (proceso_bloqueado->estado == BLOQUEADO
            && proceso_bloqueado->mutex_id == mutex_id) {
            printf("::::::::::::PROCESO ENCONTRADO: %d\n", proceso_bloqueado->id);
            proceso_bloqueado->estado = LISTO;
            proceso_bloqueado->mutex_id = -1;
            eliminarProcesoListaBloqueados(proceso_bloqueado);
            encontrado = true;
        }
        proceso_bloqueado = proceso_siguiente;
    }
    if (mutex1->num_procesos > 0)
        return 0;
    printf("::::::::::::NO HAY MAS PROCESOS\n");
    eliminar_mutex(&lista_mutex, mutex_id);
    strcpy(mutex1->nombre, "");
    cont_mutex--;


    proceso_bloqueado = lista_blocked.primero;
    while (proceso_bloqueado != NULL) {
        BCP *proceso_siguiente = proceso_bloqueado->siguiente;
        if (proceso_bloqueado->estado == BLOQUEADO
            && proceso_bloqueado->mutexBlock == 1) {
            proceso_bloqueado->estado = LISTO;
            proceso_bloqueado->mutexBlock = 0;
            eliminarProcesoListaBloqueados(proceso_bloqueado);
            break;
        }
        proceso_bloqueado = proceso_siguiente;
    }

    printf("\n\n::::::::::::MUTEX ELIMINADO %d\n", descriptor);
    return 0;
}


int sis_leer_caracter() {

    printf("\n\nCOMIENZA LEER CARACTER\n");
    int int_level = fijar_nivel_int(NIVEL_2);

    while (size_buffer == 0) {
        //printf("BUFFER VACIO\n");
        p_proc_actual->estado = BLOQUEADO;
        p_proc_actual->readBlock = 1;

        int int_level_2 = fijar_nivel_int(NIVEL_3);
        eliminar_elem(&lista_listos, p_proc_actual);
        insertar_ultimo(&lista_blocked, p_proc_actual);
        fijar_nivel_int(int_level_2);

        BCP *p_proc_blocked = p_proc_actual;
        p_proc_actual = planificador();
        cambio_contexto(&(p_proc_blocked->contexto_regs), &(p_proc_actual->contexto_regs));
    }

    printf("BUFFER NO VACIO\n");
    char primer_car = buffer[0];
    printf("LEIDO PRIMER CARACTER\n");
    size_buffer--;
    int i;
    for (i = 0; i < size_buffer; i++) {
        buffer[i] = buffer[i + 1];
    }
    fijar_nivel_int(int_level);

    return primer_car;
}

/******************************************
 * ****************************************
 * ******** Funciones auxiliares **********
 * ****************************************
 * ****************************************
 */

void anadirProcesoAListaBloqueados(BCP *proc) {
    int int_level = fijar_nivel_int(NIVEL_3);
    eliminar_elem(&lista_listos, proc);
    insertar_ultimo(&lista_blocked, proc);
    fijar_nivel_int(int_level);
}

void eliminarProcesoListaBloqueados(BCP *proceso_bloqueado) {
    int int_level = fijar_nivel_int(NIVEL_3);
    eliminar_elem(&lista_blocked, proceso_bloqueado);
    insertar_ultimo(&lista_listos, proceso_bloqueado);
    fijar_nivel_int(int_level);
}

int getMutexId(const char *nombre) {
    mutex *mutex_primero = lista_mutex.primero;
    int mutex_id = -1;
    while (mutex_primero != NULL) {
        if (strcmp(mutex_primero->nombre, nombre) == 0) {
            mutex_id = mutex_primero->index;
        }
        mutex *mutex_next = mutex_primero->siguiente;
        mutex_primero = mutex_next;
    }
    return mutex_id;
}

mutex *getMutex(lista_Mutex *lista, int mutex_id) {
    mutex *mutex_it = lista->primero;
    mutex *mutex_it_return = malloc(sizeof(mutex));
    while (mutex_it != NULL) {
        if (mutex_it->index == mutex_id) {
            mutex_it_return = mutex_it;
            return mutex_it_return;
        }
        mutex_it = mutex_it->siguiente;
    }
    mutex_it_return->index = -1;
    strcpy(mutex_it_return->nombre, "Failed");
    return mutex_it_return;

}

int getDescriptor() {
    int i;
    for (i = 0; i < NUM_MUT_PROC; ++i) {
        if (p_proc_actual->mutexList[i] == -1) {
            return i;
        }
    }
    return -1;
}

bool verificaCondiciones(const char *nombre) {

    return strlen(nombre) <= MAX_NOM_MUT
           && p_proc_actual->nMutex <= NUM_MUT_PROC
           && !nombreMutexRepetido(nombre);
}

bool nombreMutexRepetido(const char *nombre) {
    mutex *mutex_primero = lista_mutex.primero;
    bool repetido = false;
    while (mutex_primero != NULL && !repetido) {
        // printf("******************** NOMBRE %s......\n", mutex_primero->nombre);
        if (strcmp(mutex_primero->nombre, nombre) == 0) {
            repetido = true;
        }
        mutex *mutex_next = mutex_primero->siguiente;
        mutex_primero = mutex_next;
    }
    //printf("******************** ........ EL NOMBRE ESTA REPETIDO  %s\n", repetido ? "true" : "false");
    return repetido;
}

void insertar_mutex(lista_Mutex *lista, mutex *pMutex) {
    printf("\t INSERTAMOS MUTEX -> %d\n", pMutex->index);
    if (lista->primero == NULL)
        lista->primero = pMutex;
    else
        lista->ultimo->siguiente = pMutex;
    lista->ultimo = pMutex;
    pMutex->siguiente = NULL;
}

void eliminar_mutex(lista_Mutex *lista, int mutex_id) {
    printf("\t ELIMINAMOS MUTEX -> %d\n", mutex_id);
    mutex *mutex_it = lista->primero;

    if (mutex_it->index == mutex_id)
        eliminar_primero_mutex(lista);
    else {
        for (; ((mutex_it) && (mutex_it->siguiente->index != mutex_id));
               mutex_it = mutex_it->siguiente);
        if (mutex_it) {
            if (lista->ultimo == mutex_it->siguiente)
                lista->ultimo = mutex_it;
            mutex_it->siguiente = mutex_it->siguiente->siguiente;
        }
    }
}

void eliminar_primero_mutex(lista_Mutex *lista) {

    if (lista->ultimo == lista->primero)
        lista->ultimo = NULL;
    lista->primero = lista->primero->siguiente;
}

void crearMutex(char *nombre, int tipo) {
    printf("******************** CREAMOS MUTEX\n");
    mutex *nuevo_mutex = malloc(sizeof(mutex));
    nuevo_mutex->index = cont_mutex_index;
    printf("******************** init mutex con index %d\n", nuevo_mutex->index);
    printf("******************** init mutex con nombre %s\n", nombre);
    strcpy(nuevo_mutex->nombre, nombre);
    printf("******************** copiado nombre %s\n", nuevo_mutex->nombre);
    nuevo_mutex->tipo = tipo;
    printf("******************** del tipo %d\n", tipo);
    nuevo_mutex->num_procesos = 0;
    printf("********************  incrementado numero de procesos %d\n", nuevo_mutex->num_procesos);
    nuevo_mutex->proceso_bloqueado = p_proc_actual->id;
    printf("******************** INSERTAMOS MUTEX EN LISTA\n");
    insertar_mutex(&lista_mutex, nuevo_mutex);
}

/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main() {
    /* se llega con las interrupciones prohibidas */

    instal_man_int(EXC_ARITM, exc_arit);
    instal_man_int(EXC_MEM, exc_mem);
    instal_man_int(INT_RELOJ, int_reloj);
    instal_man_int(INT_TERMINAL, int_terminal);
    instal_man_int(LLAM_SIS, tratar_llamsis);
    instal_man_int(INT_SW, int_sw);

    iniciar_cont_int();        /* inicia cont. interr. */
    iniciar_cont_reloj(TICK);    /* fija frecuencia del reloj */
    iniciar_cont_teclado();        /* inici cont. teclado */

    iniciar_tabla_proc();        /* inicia BCPs de tabla de procesos */

    /* crea proceso inicial */
    if (crear_tarea((void *) "init") < 0)
        panico("no encontrado el proceso inicial");

    /* activa proceso inicial */
    p_proc_actual = planificador();
    cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    panico("S.O. reactivado inesperadamente");
    return 0;
}
