#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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

#define MAX_PACKET 1250

enum erros {WSTARTUP, ABRESOCK, BIND, ACCEPT, LISTEN,RECEIVE};

void TrataErro(SOCKET, int);

int main(int argc, char* argv[])
{
  SOCKET s= 0, s_cli;
  struct sockaddr_in  addr_serv, addr_cli;
  int addr_cli_len = sizeof(addr_cli);
	int PORTA_SRV, i;

#ifdef _WIN32
	 WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		TrataErro(s, WSTARTUP);
		exit(1);
		}
#endif

	char recvbuf[MAX_PACKET];

	// Parsing the options
	for(i=1; i<argc; i++) {
		 if(argv[i][0]=='-') {
			 switch(argv[i][1]) {
					case 'p': // porta
						 i++;
						 PORTA_SRV = atoi(argv[i]);
						 if(PORTA_SRV < 1024) {
							 printf("Valor da porta invalido\n");
							 exit(1);
						 }
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


  // Cria o socket na familia AF_INET (Internet) e do tipo TCP (SOCK_STREAM)
  if ((s = socket(AF_INET, SOCK_STREAM, 0))==INVALID_SOCKET)
	  TrataErro(s, ABRESOCK);

  // Define dom�nio, IP e porta a receber dados
  addr_serv.sin_family = AF_INET;
  addr_serv.sin_addr.s_addr = htonl(INADDR_ANY); // recebe de qualquer IP
  addr_serv.sin_port = htons(PORTA_SRV);

  // Associa socket com estrutura addr_serv
  if ((bind(s, (struct sockaddr *)&addr_serv, sizeof(addr_serv))) != 0)
	  TrataErro(s, BIND);

  // Coloca socket em estado de escuta para as conexoes na porta especificada
  if((listen(s, 8)) != 0) // permite ateh 8 conexoes simultaneas
	  TrataErro(s, LISTEN);

  // permite conexoes entrantes utilizarem o socket
  if((s_cli = accept(s, (struct sockaddr *)&addr_cli, (socklen_t *) &addr_cli_len)) < 0)
	  TrataErro(s, ACCEPT);

  // fica esperando chegar mensagem
  while(1)
  {
    if ((recv(s_cli, recvbuf, MAX_PACKET, 0)) < 0)
    {
      close(s_cli);
	  TrataErro(s, RECEIVE);
    }

    // mostra na tela
    if(strcmp((const char *)&recvbuf, "q")==0)
      break;
    else
      printf("%s\n", recvbuf);
  }

  // fecha socket e termina programa
  printf("Fim da conexao\n");
  close(s);
  close(s_cli);
  exit(1);
}

void TrataErro(SOCKET s, int tipoerro)
{
	char tipo[20];

	switch(tipoerro) {
		case WSTARTUP:
			strcpy(tipo, "Windows Startup");
			break;
		case ABRESOCK:
			strcpy(tipo, "Open Socket");
			break;
		case BIND:
			strcpy(tipo, "Bind");
			break;
		case ACCEPT:
			strcpy(tipo, "Accept");
			break;
		case LISTEN:
			strcpy(tipo, "Listen");
			break;
		case RECEIVE:
			strcpy(tipo, "Receive");
			break;
		default:
			strcpy(tipo, "Indefinido. Verificar");
			break;
	}
    printf("erro no %s", tipo);
    close(s);
    exit(1);
}
