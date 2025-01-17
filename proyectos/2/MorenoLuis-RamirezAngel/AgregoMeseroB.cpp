#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "sprites.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#define maxClientes 10 //El numero m�ximo de clientes que vamos a simular

char pantalla [47][28];
int numClientes = 0; //Para llevar la cuenta de los clientes creados
char flagMeseroA = 'n'; //Bandera para saber si llamamos o no al mesero
char flagMeseroB = 'n';
char flagCocineroA ='n'; //Bandera para saber el cocinero esta ocupado o no
short int mesas[6] = {0,0,0,0,0,0} ; //Bandera para saber que mesa esta ocupada(1) o libre (0)
short int genteFormada=0; //Clientes formados fuera
short int genteSentada=0; //Clientes en el restaurante
short int pedidoA = 0; //Que mesa le pide algo al mesero A
short int pedidoB = 0; //que mesa le pide algo al mesero B

std::mutex puerta; //S�lo un cliente puede entrar por la puerta
std::mutex caja; //S�lo un cliente puede estar pagando en la caja

std::mutex meseroAtiendeA; //El mesero atiende a un cliente o cocinero a la vez
std::mutex meseroAtiendeB; //y hay dos meseros

std::mutex cocineroA;//Hay 3 cocineros
std::mutex cocineroB;
std::mutex cocineroC;

std::condition_variable afuera; //Afuera habr� una fila de m�x 5 personas esperando 
std::mutex semAfuera; 			// para lograrlo usaremos un sem�foro

std::condition_variable restaurant; //M�ximo hay 6 personas dentro del restaurante
std::mutex semRestaurante; 			//para lograrlo usaremos un sem�foro

std::condition_variable meseroVenA; //Para que el mesero A pueda esperar mientras llegan pedidos
std::mutex meseroEsperaA;

std::condition_variable meseroVenB; //Para que el mesero B pueda esperar mientras llegan pedidos
std::mutex meseroEsperaB;

std::condition_variable meseroEncarga; //Para estar al pendiente de los cocineros, cuando se desocupen el mesero encarga un pedido 
std::mutex noEncarges;

std::condition_variable cociLibreA; //Para que el cocinero A pueda esperar mientras llegan pedidos
std::mutex cociEsperaA;

struct cliente
{
	short int numMesa; //En que mesa se sienta
};

struct mesero
{
	short int colaPedidos [3]={0,0,0}; //Guarda la mesa que le hizo el pedido en el orden que fueron pidiendo
	short int entrega; //A que mesa va a entregar la comida
};

struct cocinero
{
	short int pedido; //Sabe que mesa le hizo el pedido que cocina
};

short int eligeMesa(void)
{	
	if(mesas[0]==0) //Si la 0 esta desocupada
	{
		mesas[0]=1; //Marcala ocupada
		return 0;     //Regresa el numero de mesa
	}
	
	if(mesas[1]==0)
	{
		mesas[1]=1;
		return 1;
	}
	
	if(mesas[2]==0)
	{
		mesas[2]=1;
		return 2;
	}
	
	if(mesas[3]==0)
	{
		mesas[3]=1;
		return 3;
	}
	
	if(mesas[4]==0)
	{
		mesas[4]=1;
		return 4;
	}
	
	if(mesas[5]==0)
	{
		mesas[5]=1;
		return 5;
	}
	
	return 9; //Mi codigo de error
}

void entraAlRestaurante(struct cliente* paquito)
{
	puerta.lock();//S�lo una persona entra por la puerta a la vez
	std::unique_lock<std::mutex> espera(semRestaurante);
	while(genteSentada>5)	//Cuando no haya mesas vac�as en el restaurante
	{
		restaurant.wait(espera);	//Dejo a los dem�s esperando afuera
	}
	genteSentada++; //Si entra, hay un cliente m�s en el restaurante
	
	paquito->numMesa = eligeMesa(); //El cliente elige una mesa
	
	printf("\n\tEstoy en mesa %d",paquito->numMesa); //Va a la mesa
	puerta.unlock(); //Se aleja de la puerta
	
	genteFormada--; //Hay una persona menos en la fila
	afuera.notify_all(); //Notifico que ya puede formarse otra persona
}

void pideComida(struct cliente *paquito)
{
	if (paquito->numMesa < 3)
	{
		meseroAtiendeA.lock(); //Ocupo a mi mesero
		printf("\nww Pide %d",paquito->numMesa);
		pedidoA = paquito->numMesa;
		flagMeseroA = 't'; //Pongo al mesero en modo "toma pedido"
		meseroVenA.notify_all(); //Despierto a mi mesero
		Sleep(100);//En lo que anota mi pedido
		meseroAtiendeA.unlock();//suelto a mi mesero
	}else
	{
		meseroAtiendeB.lock(); //Lo mismo pero con el mesero B
		printf("\nww Pide %d",paquito->numMesa);
		pedidoB = paquito->numMesa;
		flagMeseroB = 't'; 
		meseroVenB.notify_all(); 
		Sleep(100);
		meseroAtiendeB.unlock();
	}
	
}

void saleDelRestaurante(struct cliente * paquito)
{
	genteSentada--; //Hay un cliente menos dentro
	mesas[paquito->numMesa]=0; //La mesa del cliente ahora esta libre
	printf("\n\t\tsali mesa %d",paquito->numMesa);
	restaurant.notify_all(); //Le aviso a quien quer�a entrar
	
}

void nvoCliente(void)
{
	short int nombreCliente;
	struct cliente *paquito;
	
	nombreCliente = numClientes;
	
	paquito = (struct cliente*) malloc (sizeof(struct cliente));
	
	printf("\tHola =D");
	//Sleep(1000);
	
	entraAlRestaurante(paquito);
	pideComida(paquito);
	//comeComida(paquito);
	//pagaComida(paquito);
	Sleep(6000);
	saleDelRestaurante(paquito);
	
	free(paquito);
	
	if (nombreCliente == maxClientes) //Si mi ultimo cliente salio del restaurante, despierto a mi mesero para que salga de su funci�n
	{
		flagMeseroA='m';
		meseroVenA.notify_all();//desbloqueo a mi mesero para que termine
	}
}

void formaAlCliente (void)
{
	std::unique_lock<std::mutex> espera(semAfuera);
	while(genteFormada>4)	//Cuando hay 5 personas en la fila
	{
		afuera.wait(espera);	//Dejan de llegar clientes porque la fila es muy larga
	}
	genteFormada++; //Si no hab�a 5, te formas
}

void funcionMadre (void)
{
	int tiempo;
	
	srand( time(NULL) );//Uso el tiempo para randomizar mis numeros
	do
	{
		formaAlCliente(); //Ves si hay menos de 5 clientes en la fila
	
		std::thread cliente(nvoCliente); //Lanzo el hilo de mi cliente
		numClientes++;
		
		if(cliente.joinable()) //Si el hilo a�n le pertenece a este proceso...
    	{
        	cliente.detach(); //No voy a esperar a que el hilo termine su ejecuci�n, as� que lo hago independiente
    	}
		
		//tiempo = rand() %1500;
		tiempo=0;
		printf("\n==Cliente %d",numClientes);
		printf("\n tiempo: %d %d",tiempo, rand());
		Sleep(500+tiempo); //Espero antes de crear un hilo
		
	}while(numClientes<maxClientes); //Repite hasta crear el m�ximo de clientes simulados
	
	printf("\n**Se acabaron los clientes");
}

void meseroAEncarga(struct mesero * mA) //Funci�n Auxiliar de meseroUno, esta al pendiente de los cocineros
{										//si uno esta disponible, va y le hace un pedido

	std::unique_lock<std::mutex> descansa(noEncarges);
		while(flagCocineroA != 'n')	//Mientras el cocinero NO este desocupado
		{
			meseroEncarga.wait(descansa);	//El mesero espera para realizar un pedido
		}
	
	meseroAtiendeA.lock();//Bloqueo a mi mesero porque esta en uso
	
	if(flagCocineroA == 'n') //Si el primer cocinero esta desocupado
	{
		cocineroA.lock(); //Ocupo a mi cocinero 1
		flagCocineroA='p';
		pedidoA = mA->colaPedidos[0]; //Mando el pedido a mi cocinero
		cociLibreA.notify_all(); //Le digo a mi cocinero que tiene un pedido
		cocineroA.unlock(); //lo libero
		
		mA->colaPedidos[0]=mA->colaPedidos[1]; //Recorro mi cola de pedidos
		mA->colaPedidos[1]=mA->colaPedidos[2];
		mA->colaPedidos[2]=0;
	}
	
	meseroAtiendeA.unlock(); //Dejo de usar a mi mesero
}

void meseroUno (void) //Atiende las primeras 3 mesas
{
	struct mesero mA;
	printf("\n-- Naci, mesero 1");
	do
	{
		std::unique_lock<std::mutex> descansa(meseroEsperaA);
		while(flagMeseroA=='n')	//Si nadie llama al mesero...
		{
			meseroVenA.wait(descansa);	//El mesero espera
		}
		
		//De lo contrario ...
		
		switch(flagMeseroA)
		{
			case 't': //En caso de que haya llamado el cliente tomo su pedido
					{
						if(mA.colaPedidos[0]==0) //Si no hay pedidos en la cola
						{
							printf("\n++1er pedido mesa %d",pedidoA);
							mA.colaPedidos[0] = pedidoA; //Guarda el numero de mesa que le hizo un pedido
						}else
						{
							if(mA.colaPedidos[1]==0)//Si hab�a un pedido en la cola
							{
								printf("\n++2do pedido mesa %d",pedidoA);
								mA.colaPedidos[1] = pedidoA;
							}else //Si hab�a dos pedidos en la cola
							{
								printf("\n++3er pedido mesa %d",pedidoA);
								mA.colaPedidos[2] = pedidoA;
							}
						}
						
						std::thread encargalo (meseroAEncarga, &mA); //Hago un hilo para que mi mesero este listo
																	//para encargar los pedidos a los cocineros
						if (encargalo.joinable())
						{
							encargalo.detach();	//y suelto ese hilo para que sea independiente
						}
						flagMeseroA = 'n'; //me desocupo
					}
						break;
						
			case 'r': //Si llamo el cocinero, recojo el pedido
						mA.entrega = pedidoA;
						printf("\n\n==Entrego Pedido %d\n",mA.entrega);
						flagMeseroA = 'n'; //me desocupo
						break;	
			default:
						printf("\nAdios");
						printf("\n--%d --%d --%d",mesas[0],mesas[1],mesas[2]);
						Sleep(500);
		} 
		
	}while( (mesas[0]+mesas[1]+mesas[2])!= 0 || numClientes< maxClientes);//Repite mientras mis mesas no esten vac�as	y no hayamos cerrado
	
}

void meseroBEncarga(struct mesero * mB) //Funci�n Auxiliar de meseroDos, esta al pendiente de los cocineros
{										//si uno esta disponible, va y le hace un pedido
	
	std::unique_lock<std::mutex> descansa(noEncarges);
		while(flagCocineroA != 'n')	//Mientras el cocinero NO este desocupado
		{
			meseroEncarga.wait(descansa);	//El mesero espera para realizar un pedido
		}
	
	meseroAtiendeB.lock();//Bloqueo a mi mesero porque esta en uso
	
	if(flagCocineroA == 'n') //Si el primer cocinero esta desocupado
	{
		cocineroA.lock(); //Ocupo a mi cocinero 1
		flagCocineroA='d'; //Le digo que le habla el meseroDos
		pedidoB = mB->colaPedidos[0]; //Mando el pedido a mi cocinero
		cociLibreA.notify_all(); //Le digo a mi cocinero que tiene un pedido
		cocineroA.unlock(); //lo libero
		
		mB->colaPedidos[0]=mB->colaPedidos[1]; //Recorro mi cola de pedidos
		mB->colaPedidos[1]=mB->colaPedidos[2];
		mB->colaPedidos[2]=0;
	}
	
	meseroAtiendeB.unlock(); //Dejo de usar a mi mesero
}

void meseroDos (void) //Atiende las �ltimas 3 mesas
{					//Misma funci�n que meseroUno(), pero con las variables creadas para meseroB
	
	struct mesero mB; 
	printf("\n-- Naci, mesero 2");
	do
	{
		std::unique_lock<std::mutex> descansa(meseroEsperaB);
		while(flagMeseroB=='n')	//Si nadie llama al mesero...
		{
			meseroVenB.wait(descansa);	//El mesero espera
		}
		
		//De lo contrario ...
		
		switch(flagMeseroB)
		{
			case 't': //En caso de que haya llamado el cliente tomo su pedido
					{
						if(mB.colaPedidos[0]==0) //Si no hay pedidos en la cola
						{
							printf("\n~~1er pedido mesa %d",pedidoB);
							mB.colaPedidos[0] = pedidoB; //Guarda el numero de mesa que le hizo un pedido
						}else
						{
							if(mB.colaPedidos[1]==0)//Si hab�a un pedido en la cola
							{
								printf("\n~~2do pedido mesa %d",pedidoB);
								mB.colaPedidos[1] = pedidoB;
							}else //Si hab�a dos pedidos en la cola
							{
								printf("\n~~3er pedido mesa %d",pedidoB);
								mB.colaPedidos[2] = pedidoB;
							}
						}
						
						std::thread encargalo (meseroBEncarga, &mB); //Hago un hilo para que mi mesero este listo
																	//para encargar los pedidos a los cocineros
						if (encargalo.joinable())
						{
							encargalo.detach();	//y suelto ese hilo para que sea independiente
						}
						flagMeseroB = 'n'; //me desocupo
					}
						break;
						
			case 'r': //Si llamo el cocinero, recojo el pedido
						mB.entrega = pedidoB;
						printf("\n\n==Entrego Pedido %d\n",mB.entrega);
						flagMeseroB = 'n'; //me desocupo
						break;	
			default:
						printf("\nAdios2");
						printf("\n--%d --%d --%d",mesas[3],mesas[4],mesas[5]);
						Sleep(500);
		} 
		
	}while( (mesas[3]+mesas[4]+mesas[5])!= 0 || numClientes< maxClientes);//Repite mientras mis mesas no esten vac�as	y no hayamos cerrado
	
}


void cocinero (void)
{
	struct cocinero structCocinero;
	do
	{
		std::unique_lock<std::mutex> descansa(cociEsperaA);
		while( flagCocineroA=='n'  )	//Si el cocinero esta desocupado
		{
			cociLibreA.wait(descansa);	//El cocinero espera
		}
		
	
		
		switch(flagCocineroA)
		{
			case 'p'://Si se le hizo un pedido meseroA
						structCocinero.pedido = pedidoA;
		
						printf("\nhago pedido %d",structCocinero.pedido);
						Sleep(1000); //Prepara la comida
				
						meseroAtiendeA.lock();
						flagMeseroA = 'r'; //mesero recoge el pedido
						pedidoA = structCocinero.pedido;
						meseroVenA.notify_all(); //Llamo al mesero
						meseroAtiendeA.unlock();//libero al mesero
				
						flagCocineroA='n'; //me desocupo =)
						meseroEncarga.notify_all(); //Y se lo digo a los meseros
						break;
			case 'd'://Si se lo hizo meseroB
						structCocinero.pedido = pedidoB;
		
						printf("\nhago pedido %d",structCocinero.pedido);
						Sleep(1000); //Prepara la comida
				
						meseroAtiendeB.lock();
						flagMeseroB = 'r'; //mesero recoge el pedido
						pedidoB = structCocinero.pedido;
						meseroVenB.notify_all(); //Llamo al mesero
						meseroAtiendeB.unlock();//libero al mesero
				
						flagCocineroA='n'; //me desocupo =)
						meseroEncarga.notify_all(); //Y se lo digo a los meseros
						break;
			default:
				printf("\n**Von voyage");
				printf("\nPresiona ENTER para terminar...");
		}
		
		
	}while( (mesas[0]+mesas[1]+mesas[2]+mesas[3]+mesas[4]+mesas[5])!= 0 || numClientes< maxClientes);//Repite mientras mis mesas no esten vac�as y no hayamos cerrado
	
}

void prinx (void)
{
	while (numClientes<maxClientes)
	{
	//	printf("\n\tHay %d clientes afuera",genteFormada);
		Sleep(1000);
	}
	
}


int main(void)
{	
	
	
	printf("\n,, LAnzo meseroA");
	std::thread meseA (meseroUno);
	printf("\n,, LAnzo meseroB");
	std::thread meseB (meseroDos);
	printf("\n,, LAnzo Cocinero");
	std::thread cociA (cocinero);
	printf("\n,, LAnzo Madre");
	std::thread madre (funcionMadre);
	
//	std::thread imprime (prinx);
//	imprime.detach();
	madre.join(); //espero a que termine mi hilo creador de hilos
	meseA.join();//espero a que termine mi hilo mesero
	
	flagMeseroB='m';//Le digo a mi meseroB que es hora de terminar
	meseroVenB.notify_all();//desbloqueo a mi mesero para que termine
	meseB.join();//Espero a que termine
	
	flagCocineroA='m';//Le digo a mi cocinero que es hora de acabar
	cociLibreA.notify_all();//Y lo despierto
	cociA.join();//Espero a que mi hilo cocinero termine
	
	return 0;
}
