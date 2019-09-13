/*Tarea 1: Programa para resolver un problema de sincronizaci�n usando sem�foros y mutex
creado por Luis Moreno y Angel Ram�rez Cese�a */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <thread>
#include <mutex>
#include <condition_variable>

unsigned short int numAlumno=0; //Para llevar la cuenta de los alumnos creados
unsigned int numSillas; //Cuantos alumnos pueden entrar al cub�culo del profesor a la vez
unsigned int numAlumni; //Cuantos alumnos tendremos en la simulaci�n
unsigned int sillasVacias; //Nos va a servir al final del programa para saber cuando acabar la ejecuci�n

std::mutex preguntando; //Para que s�lo un alumno pueda preguntar a la vez

std::condition_variable pasillo; //Para que duerman los alumnos que se quedaron afuera
std::mutex semaforo; //Para proteger la secci�n del sem�foro


struct alumno //Todo alumno tiene...
{
	unsigned short int identificador; //Un nombre
	unsigned short int numPreguntas; //Varias preguntas buscando respuesta
};


struct alumno * crearAlumno (void) //Reservo la memor�a para mi estructura alumno e inicializo sus valores
{
	numAlumno++;
	unsigned short int numIdentificador = numAlumno; //Mi alumno es el n�mero...
	
	struct alumno * ptrAl;
	ptrAl = (struct alumno *) malloc ( 1*sizeof(struct alumno) ); //Creo mi estructura alumno
	
	ptrAl->identificador = numIdentificador; //Le doy nombre
	
	ptrAl->numPreguntas = rand() % 15; //y un n�mero de preguntas aleatorio menor a 15
	
	return ptrAl; //Envio la estructura creada como resultado
	
}

void entrarAlSalon (void) //Aqu� aplico mi sem�foro para dejar entrar a cierto n�mero de personas
{
	std::unique_lock<std::mutex> alto(semaforo);
	while(numSillas<=0)	//Cuando no haya sillas vac�as en el sal�n
	{
		pasillo.wait(alto);	//Dejo a los dem�s esperando en el pasillo
	}
	numSillas--;	//Si alguien entra, tenemos una silla menos
}

void alumnoVivo (void)	//Mi alumno esta vivo y quiere hacer unas preguntas
{
	struct alumno *ptrAl;
	
	ptrAl = crearAlumno(); //Mando crear a mi alumno
	
	printf("\n\nHola, soy %d y tengo %d preguntas =D\n",ptrAl->identificador,ptrAl->numPreguntas); //Se presenta
	
	entrarAlSalon(); //Ve si puede entrar al sal�n
	printf("%c %i entr%c al sal%cn \n",175,ptrAl->identificador,162,162); //Entre al sal�n
	
	while(ptrAl->numPreguntas>0) //Mientras a�n tenga preguntas que hacer...
	{
		preguntando.lock(); //pido la palabra
		printf("%c %i hizo una pregunta\n",177,ptrAl->identificador); //Hago mi pregunta y me responden
		Sleep(500);
		(ptrAl->numPreguntas)--; //Tengo una duda menos
		preguntando.unlock(); //Cedo la palabra
		Sleep(500); //Espero un poco antes de volver a preguntar
	}
	
	numSillas++; //Si ya acab� de preguntar me levanto de mi silla...
	
	printf("%c %i sali%c del sal%cn \n\n",245,ptrAl->identificador,162,162);
	
	pasillo.notify_all(); //...y le aviso a los de afuera
	
	
	
	free(ptrAl); //Libero la memoria de mi estructura alumno
	
}


void funcionMadre (void) //Creo alumnos cada cierto tiempo
{
	unsigned short int numHilo=0;
	//int i;
	do
	{
	
		std::thread hilo(alumnoVivo); //Lanzo el hilo de mi alumno
		numHilo++; //Ya cree un hilo
		
		if(hilo.joinable()) //Si el hilo a�n le pertenece a este proceso...
    	{
        	hilo.detach(); //No voy a esperar a que el hilo termine su ejecuci�n, as� que lo hago independiente
    	}
		
		
		Sleep(1000); //Espero 1 segundo entre la creaci�n de un nuevo hilo
	}while(numHilo<numAlumni); //Repito todo hasta que cree los alumnos necesarios en la simulaci�n
	
	
}


int main (void)
{
	unsigned int i;
	printf("Bienvenido al problema de los alumnos y el asesor =D\n\n");
	printf("%cCuantas sillas hay en el sal%cn? ",168,162);
	scanf("%u",&numSillas);
	sillasVacias = numSillas;
	printf("\n%cY cuantos alumnos recibiremos hoy? ",168);
	scanf("%u",&numAlumni);
	
	
	std::thread madre (funcionMadre); //Lanzo mi funci�n creadora de hilos en un nuevo hilo
	
	madre.join();	//Espero a que termine de ejecutarse mi funci�n madre
	
	while(numSillas != sillasVacias) //Mantengo la ejecuci�n mientras a�n haya alumnos en el sal�n:
	{								 //numSillas va a ir aumentando conforme vayan saliendo los alumnos del sal�n
		Sleep(1000);				 // y su valor debe ser igual al de sillasVacias cuando no quede nadie dentro.
		
		//printf("\n== numSillas %u / numVacias %u\n",numSillas,sillasVacias);
	}
	
	printf("\n\nSe acabo =D\n\nPresiona ENTER para continuar...");
	
	fflush(stdin);
	getchar();
	
	return 0;
}
