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

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
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
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
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
static void espera_int(){
	int nivel;

	//printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	//ROUND ROBIN
	ticksPorRodaja = TICKS_POR_RODAJA;
	procesoAExpulsar = NULL;
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	//printk("llego aqui\n");

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

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
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	int n_interrupcion = fijar_nivel_int(NIVEL_2);//1 o 2?
	char car;
	car = leer_puerto(DIR_TERMINAL);
	

	if(char_escritos < TAM_BUF_TERM){
		printk("-> TRATANDO INT. DE TERMINAL %c\n", car);
		buffer_char[ind_escribir] = car;
		ind_escribir = (ind_escribir +1 ) % TAM_BUF_TERM;
		char_escritos++;
		if(lista_espera_char.primero != NULL){
		//	printk("Habia un proceso esperando a la escritura");
			//si hay procesos esperando a que se escriba en el terminal
			BCPptr proceso = lista_espera_char.primero;
			//elimina al proceso de la lista de procesos en espera de escritura
			eliminar_primero(&lista_espera_char);
			proceso->estado=LISTO;
			//añade el proceso a  la lista de procesos listos
			insertar_ultimo(&lista_listos, proceso);
		}
	} else {
		printk("El buffer esta lleno\n");
	}
  //modificar para la lectura de caracteres
	fijar_nivel_int(n_interrupcion);
    return;
}

static void roundRobin(){
	ticksPorRodaja --;
	if(ticksPorRodaja <= 0){
		procesoAExpulsar = p_proc_actual;
		activar_int_SW();
	}

}
/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	//printk("-> TRATANDO INT. DE RELOJ\n");
	cuentaAtrasBloqueados();
	roundRobin();
    return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	int n_interrupcion = fijar_nivel_int(NIVEL_1);
	//printk("-> TRATANDO INT. SW\n");
	if (p_proc_actual == procesoAExpulsar)
	{
		BCPptr actual = p_proc_actual;
		int n_interrupcion = fijar_nivel_int(NIVEL_3);
		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_listos, actual);
		p_proc_actual = planificador();
		fijar_nivel_int(n_interrupcion);
		cambio_contexto(&(actual->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	fijar_nivel_int(n_interrupcion);
	
	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		// modificado
		for(int i = 0; i< NUM_MUT_PROC; i++){
			p_proc->descriptores[i] = -1;
		}
		p_proc->n_descriptores = 0;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

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
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}

int obtener_id_pr(){
	return p_proc_actual->id;
}

int dormir(unsigned int segundos){
	//leo el parametro de los registros
	unsigned int seg_registro = (unsigned int)leer_registro(1);

	//guardo el nivel de interrupcion
	int n_interrupcion = fijar_nivel_int(NIVEL_3);
	BCPptr actual = p_proc_actual;

	//actualizo la estructura de datos
	actual->estado = BLOQUEADO;
	actual->segundos_dormir = seg_registro * TICK;
	

	//cambio de lista
	eliminar_elem(&lista_listos, actual);
	insertar_ultimo(&lista_bloqueados, actual);

	//el planificador devuelve el proceso a ejecutar
	p_proc_actual = planificador();

	//restauro el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);

	cambio_contexto(&(actual->contexto_regs), &(p_proc_actual->contexto_regs));

	printk("Proceso %d termina de dormir.\n", p_proc_actual->id);
	return 0;
}

void cuentaAtrasBloqueados(){
	//recorro la lista y actualizo los tiempos
	BCPptr aux = lista_bloqueados.primero;
	while(aux != NULL){
		BCPptr siguiente = aux->siguiente;
		aux->segundos_dormir--;
		if(aux->segundos_dormir <=0){
			//si ha terminado lo cambio de lista
			aux->estado = LISTO;
			eliminar_elem(&lista_bloqueados, aux);
			insertar_ultimo(&lista_listos, aux);
		}
		aux = siguiente;
	}
	return;
}

int buscarMutexPorNombre(char* nombre){
	int i = 0;
	for(i = 0;i < NUM_MUT;i++){
		if(strcmp(lista_mutex[i].nombre, nombre) == 0){
			return i;
		}
	}
	return -1;
}

int buscarHuecoListaMutex(){
	int i = 0;
	for(i = 0;i < NUM_MUT;i++){
		if(lista_mutex[i].libre_ocupado == LIBRE) return i;
	}
	return -1;
}


int crear_mutex(char* nombre, int tipo){
	printk("Comienza crear mutex.\n");

	char* nom = (char*)leer_registro(1);
	int t = (int) leer_registro(2);

	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	printk("Comprobando validez del nombre.\n");
	if(strlen(nom) > (MAX_NOM_MUT-1)){
		//el ultimo caracter se elimina
		nom[MAX_NOM_MUT] = '\0';
	}
	
	mutex m;

	if(buscarMutexPorNombre(nom) == -1){
		printk("El nombre no existe, se puede crear.\n");
		int pos = buscarHuecoListaMutex();
		if(pos >= 0){
			printk("Creando mutex.\n");
			//inicializo el mutex
			strcpy(m.nombre, nom);
			m.libre_ocupado = OCUPADO;
			m.recursivo = t;
			m.n_procesos_esperando = 0;
			m.proceso_usando = NULL;
			m.bloqueos = 0;

			//guardo el mutex
			lista_mutex[pos] = m;
			total_mutex++;
			
			int desc = abrir_mutex(nom);
			fijar_nivel_int(n_interrupcion);
			return desc;
		} else {
			printk("No quedan mutex en el sistema. El proceso pasa a estar bloqueado.\n");
			
			BCPptr aux = p_proc_actual;

			//cambio el estado a bloqueado
			aux->estado = BLOQUEADO;
			eliminar_elem(&lista_listos, aux);
			//inserto en la lista de esprea para mutex
			insertar_ultimo(&lista_espera_mutex, aux);

			//cambio al siguiente de la lista
			p_proc_actual = planificador();

			fijar_nivel_int(n_interrupcion);

			cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));

			return -1;
		} 
	} else {
		printk("Ese nombre ya esta en uso.\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}
	return 0;
}

int abrir_mutex(char* nombre){
	printk("Comenzando abrir mutex.\n");
	char* nom = (char*) leer_registro(1);

	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	if(p_proc_actual->n_descriptores < NUM_MUT_PROC){
		//si quedan descriptores disponibles
		int desc = buscarMutexPorNombre(nom);
		if(desc >= 0){
			printk("Mutex encontrado. Asignando...\n");
			p_proc_actual->descriptores[p_proc_actual->n_descriptores] = desc;
			p_proc_actual->n_descriptores++;
			fijar_nivel_int(n_interrupcion);
			return desc;
		} else {
			printk("No existe el mutex especificado.\n");
			fijar_nivel_int(n_interrupcion);
			return -1;
		}
	} else {
		printk("No quedan descriptores disponibles para este proceso.\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}
}

//TO-DO: revisar lock y unlock

int lock(unsigned int mutexid){
	printk("Comenzando lock\n");
	unsigned int id = (unsigned int) leer_registro(1);
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	int encontrado = -1;
	int i = 0;
	for(i = 0;i < NUM_MUT_PROC;i++){
		if(p_proc_actual->descriptores[i] == id) encontrado = 0;
	}


	if(encontrado == -1){
		printk("El mutex no existe.\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}

	while(lista_mutex[id].proceso_usando != NULL && lista_mutex[id].proceso_usando != p_proc_actual){
		// si no lo uso yo, me bloqueo
		p_proc_actual->estado = BLOQUEADO;
		int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_mutex[id].procesos_esperando,p_proc_actual);
		lista_mutex[id].n_procesos_esperando++;
		fijar_nivel_int(n_interrupcion2);
		
		BCPptr aux = p_proc_actual;
		p_proc_actual = planificador();

		cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	lista_mutex[id].proceso_usando = p_proc_actual;
	
	if(lista_mutex[id].bloqueos == 1 && lista_mutex[id].recursivo == NO_RECURSIVO){
		printk("El mutex no es recursivo.\n");
		return -2;
	}
	lista_mutex[id].bloqueos++;

	printk("Lock terminado.\n");
	fijar_nivel_int(n_interrupcion);
	return 0;	
}

int unlock(unsigned int mutexid){
	printk("Comienza unlock.\n");
	unsigned int id = leer_registro(1);
	int n_interrupcion = fijar_nivel_int(NIVEL_1);

	int i = 0;
	int encontrado = -1;
	for(i = 0;i < NUM_MUT_PROC;i++){
		if(p_proc_actual->descriptores[i] == id){
			encontrado = 0;
		}
	}

	if(encontrado == -1){ // el mutex no existe
	 	printk("El mutex no se encuentra en la lista de descriptores\n");
	 	fijar_nivel_int(n_interrupcion);
	 	return -1;
	}
	if(lista_mutex[id].proceso_usando != p_proc_actual) {
		printk("El mutex no lo estoy usando yo.\n");
	 	fijar_nivel_int(n_interrupcion);
		return -2;
	}

	lista_mutex[id].bloqueos--;
	if(lista_mutex[id].bloqueos != 0){
		printk("El mutex es recursivo.\nUnlock terminado.\n");
	 	fijar_nivel_int(n_interrupcion);
		return 0;
	}
	lista_mutex[id].proceso_usando = NULL;

	if(lista_mutex[id].procesos_esperando.primero == NULL){
		printk("No hay procesos bloqueando el mutex.\n");
	 	fijar_nivel_int(n_interrupcion);
		return -3;
	}

	printk("El proceso usando pasa a ser el siguiente en espera.\n");
	BCPptr aux = lista_mutex[id].procesos_esperando.primero;

	aux->estado = LISTO;

	int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
	eliminar_primero(&lista_mutex[id].procesos_esperando);
	insertar_ultimo(&lista_listos, aux);
	fijar_nivel_int(n_interrupcion2);

	lista_mutex[id].proceso_usando = aux;	
	
	printk("Unlock termina.\n");
	fijar_nivel_int(n_interrupcion);
	return 0;
}

int liberarTodosLosProcesosBloqueadosMutex(mutex* m){
	printk("Liberando procesos bloqueados en el mutex %s.\n", m->nombre);
	BCPptr actual = m->procesos_esperando.primero;
	BCPptr siguiente = NULL;
	while(m->procesos_esperando.primero != NULL){
		siguiente = actual->siguiente;
		int n_interrupcion = fijar_nivel_int(NIVEL_3);
		eliminar_primero(&m->procesos_esperando);
		insertar_ultimo(&lista_listos, siguiente);
		actual->estado = LISTO;
		fijar_nivel_int(n_interrupcion);
		actual = siguiente;
	}
	// redundante, pero viene bien para deteccion de errores fuera del scope de esta funcion
	return m->procesos_esperando.primero == NULL ?  1 :  -1;
}

int cerrar_mutex(unsigned int mutexid){
	printk("Comienza cerrar mutex\n");
	unsigned int id = (unsigned int) leer_registro(1);
	int n_interrupcion = fijar_nivel_int(NIVEL_1);
	int i = 0;
	int encontrado = -1;
	for (i = 0; i < NUM_MUT_PROC; i++){
		if(p_proc_actual->descriptores[i] == id) encontrado = 1;
	}
	if(encontrado == -1){
		printk("El descriptor no existe en el proceso actual.\n");
		fijar_nivel_int(n_interrupcion);
		return -1;
	}
	mutex* m = &lista_mutex[id];
	
	// cierre implicito (se aplica a la lista de mutex solamente)
	if(m->proceso_usando == p_proc_actual){
		m->proceso_usando = NULL;
		m->n_procesos_esperando = 0;
	}
	p_proc_actual->n_descriptores--;
	p_proc_actual->descriptores[i] = -1;
	m->libre_ocupado = LIBRE;
	total_mutex--;

	// si es recursivo, libero todos los procesos bloqueados en el
	if(m->recursivo == RECURSIVO){
		if(liberarTodosLosProcesosBloqueadosMutex(m) == -1){
			printk("Error al liberar todos los procesos bloqueados por el mutex. Resolviendo...\n");
			return -2;
		}
	}
	m->bloqueos = 0;
	if(lista_espera_mutex.primero != NULL){
		printk("Hay procesos en espera para crear un mutex.\n");
		BCPptr proceso = lista_espera_mutex.primero;
		int n_interrupcion2 = fijar_nivel_int(NIVEL_3);
		eliminar_primero(&lista_espera_mutex);
		insertar_ultimo(&lista_listos, proceso);
		proceso->estado = LISTO;
		fijar_nivel_int(n_interrupcion2);
	}
	printk("Mutex cerrado.\n");
	fijar_nivel_int(n_interrupcion);
	return 0;
}
int leer_caracter(){
	
	int n_interrupcion = fijar_nivel_int(NIVEL_1);
	char leido= -1;//para comprobar si lee del buffer 
	do{
		if(char_escritos > 0){
			//tratar cada caracter escrito
			leido= buffer_char[ind_leer];
			ind_leer= (ind_leer +1) % TAM_BUF_TERM;
			char_escritos--;
		}else{
			//bloquea el proceso si no hay ningun caracter introducido
			BCPptr proceso = p_proc_actual;
			proceso->estado= BLOQUEADO;
			int interrup = fijar_nivel_int(NIVEL_3);
			//eliminar de la lista de procesos listos
			eliminar_elem(&lista_listos, proceso);
			//añadir a la lista de espera de introducir char
			insertar_ultimo(&lista_espera_char, proceso);
			//cambio al siguiente de la lista
			p_proc_actual = planificador();

			fijar_nivel_int(interrup);

			cambio_contexto(&(proceso->contexto_regs),&(p_proc_actual->contexto_regs));
		}
	} while (leido < 0);
	fijar_nivel_int(n_interrupcion);
	return leido;
}
/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
	
	int i = 0;
	for(i = 0;i < NUM_MUT;i++) lista_mutex[i].libre_ocupado = LIBRE;

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
	

