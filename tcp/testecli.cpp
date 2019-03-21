#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#define	SOCKET	int
	#define INVALID_SOCKET  ((SOCKET)~0)
#endif

// Usadas para calcular o tempo
struct timespec startCount, stopCount;
// usada para calcular o numero de pacotes enviados
unsigned int i;
// Mutex para garantir exclusao mutua na hora do calculo
// do tempo para nao alterar i e nem stopCount no momento do calculo.
pthread_mutex_t blockSending;
// Arquivo para salvar as medias por segundo.
FILE *logFile;
// Funcao executada na thread usada para fazer os calculos e
// monitorar o tempo em paralelo
// Retirando tempo de dentro do laco de execucao do envio de pacotes
// Tornanco assim um pouco mais precisas as medidas
void *getRate(void *arg);


int main(int argc, char* argv[])
{
  SOCKET s;
  struct sockaddr_in  s_cli, s_serv;
	char str[1250], ip_srv[16];
	int PORTA_CLI= 2345, PORTA_SRV= 2023;
  char ch;
	pthread_t countTimeThread;
	char fileName[25] = "log.txt"; // default name

#ifdef _WIN32
	 WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		printf("Erro no startup do socket\n");
		exit(1);
	}
#endif

if(argc < 5) {
	 printf("Utilizar:\n");
	 printf("trans -h <numero_ip> -p <porta>\n");
	 exit(1);
}
// Parsing the options
for(i=1; i<argc; i++) {
	 if(argv[i][0]=='-') {
		 switch(argv[i][1]) {
				case 'h': // Numero IP
					 i++;
					 strcpy(ip_srv, argv[i]);
					 break;
				case 'p': // porta
					 i++;
					 PORTA_SRV = atoi(argv[i]);
					 if(PORTA_SRV < 1024) {
						 printf("Valor da porta invalido\n");
						 exit(1);
					 }
					 break;
				// To select the log file name
				case 'f':
						i++;
						strcpy(fileName, argv[i]);
						break;
				default:
					 printf("Parametro invalido %d: %s\n",i,argv[i]);
					 exit(1);
		 }
	 } else {
		 printf("Parametro %d: %s invalido\n",i, argv[i]);
		 exit(1);
	 }
}

logFile = fopen(fileName, "w");
if(logFile==NULL)
{
		printf("Error opening the log file.");
		exit(0);
}

  // Abre socket TCP
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    printf("Erro iniciando socket\n");
    return(0);
  }

  // seta informacoes IP/Porta do servidor remoto
  s_serv.sin_family = AF_INET;
  s_serv.sin_addr.s_addr = inet_addr(ip_srv);
  s_serv.sin_port = htons(PORTA_SRV);

  // connecta socket aberto no cliente com o servidor
  if(connect(s, (struct sockaddr*)&s_serv, sizeof(s_serv)) != 0)
  {
    printf("erro na conexao");
    close(s);
    exit(1);
  }
	// Just copy some message into the buffer...
	strcpy(str, "Just Fooling Around...");

	// Create Mutex for mutual exclusion on the time
	int res = pthread_mutex_init(&blockSending, NULL);
	if (res != 0 ){
		printf("Mutex creation error!\n");
		exit(EXIT_FAILURE);
	}
	// Inicia thread para monitorar o tempo e salvar o log
	int shit = pthread_create(&countTimeThread, NULL, getRate, NULL);
	// Seta i=0 para contar o numero de pacotes enviados
	i = 0;
	// Seta o tempo de inicio para o calculo da taxa de envio
	clock_gettime(CLOCK_REALTIME, &startCount);

	// Loop that sends to the server
	while(1)
  {
		pthread_mutex_lock(&blockSending);
		// Send a buffer of 1250 bytes to the server over the TCP connection
		if (send(s, (const char *)&str, sizeof(str),0) < 0 ) {
			printf("Error during transmission!\n");
      close(s);
      break;
		}
    //send(s, (const char *)&str, sizeof(str),0);
		clock_gettime(CLOCK_REALTIME, &stopCount);
		i++;
		pthread_mutex_unlock(&blockSending);
  }
	fclose(logFile);
  return 0;
}

void *getRate(void *arg)
{
	while(1)
	{
		// Usa 1 segundos para o calculo da taxa de envio
		if ((stopCount.tv_sec - startCount.tv_sec) > 0){
			pthread_mutex_lock(&blockSending);
			fprintf(logFile, "%lu\n", (unsigned long int) (i * 1250 * 8));
			i = 0;
			clock_gettime(CLOCK_REALTIME, &stopCount);
			clock_gettime(CLOCK_REALTIME, &startCount);
			pthread_mutex_unlock(&blockSending);
		}
		fflush(logFile);
	}
}
