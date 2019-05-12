/*
 *  usuario/include/servicios.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef SERVICIOS_H
#define SERVICIOS_H

/* Evita el uso del printf de la bilioteca est�ndar */
#define printf escribirf

struct tiempos_ejec {
    int usuario;
    int sistema;
};

/* Funcion de biblioteca */
int escribirf(const char *formato, ...);

/* Llamadas al sistema proporcionadas */
int crear_proceso(char *prog);

int terminar_proceso();

int escribir(char *texto, unsigned int longi);

int obtener_id_pr();

int dormir(unsigned int segundos);

int tiempos_proceso(struct tiempos_ejec *t_ejec);

#endif /* SERVICIOS_H */

