/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 12

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID_PR 3
#define DORMIR 4
#define TIEMPOS_PROCESO 5
#define CREAR_MUTEX 6
#define ABRIR_MUTEX 7
#define LOCK 8
#define UNLOCK 9
#define CERRAR_MUTEX 10
#define LEER_CARACTER 11


#endif /* _LLAMSIS_H */

