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
#include <string.h>


#define NO_RECURSIVO 0
#define RECURSIVO 1
#define LIBRE 1
#define OCUPADO 0
/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;
int char_escritos;
int escribir;
int leer;

typedef struct BCP_t {
    int id;				/* ident. del proceso */
    int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
	unsigned int segundos_dormir; /*segundos que debe dormir el proceso si fuera necesario*/
    contexto_t contexto_regs;	/* copia de regs. de UCP */
    void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
	int descriptores[NUM_MUT_PROC];// descriptores de mutex en uso
	int n_descriptores;
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;

typedef struct{
	char nombre[MAX_NOM_MUT];
	int libre_ocupado; // LIBRE|OCUPADO
	int recursivo; //RECURSIVO|NO RECURSIVO
	int n_procesos_esperando; // numero de procesos esperando asociado a procesos_esperando
	lista_BCPs procesos_esperando; // lista de procesos esperando
	BCPptr proceso_usando;// proceso que está actualmente usando
	int bloqueos; // total de bloqueos
} mutex;
/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};
lista_BCPs lista_bloqueados={NULL, NULL};
lista_BCPs lista_espera_mutex={NULL, NULL};


mutex lista_mutex[NUM_MUT];
int total_mutex = 0;
/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

void cuentaAtrasBloqueados();
int quedanMutexDisponibles();
int buscarMutexPorNombre();
int asignarDescriptorAProceso();
/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr();
int dormir(unsigned int segundos);
int crear_mutex(char* nombre, int tipo);
int abrir_mutex(char*nombre);
int lock(unsigned int mutexid);
int unlock(unsigned int mutexid);
int cerrar_mutex(unsigned int mutexid);
int leer_caracter();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr},
					{dormir},
					{crear_mutex},
					{abrir_mutex},
					{lock},
					{unlock},
					{cerrar_mutex}
					{leer_caracter}};

#endif /* _KERNEL_H */

