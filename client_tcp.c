
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <netinet/in.h>

#include <netdb.h>


#define BUF_SIZE 1024
#define MAX_GRUPOS 100

int sock;

char menu[200];

void handler_ctrlc(){}
void handler_ctrlc_process(){}

int verificaSubstring(const char* string, const char* substring) {
    int len_string = strlen(string);
    int len_substring = strlen(substring);

    for (int i = 0; i <= len_string - len_substring; i++) {
        int j;
        for (j = 0; j < len_substring; j++) {
            if (string[i + j] != substring[j])
                break;
        }
        if (j == len_substring)
            return 1; // A substring foi encontrada
    }
    return 0; // A substring não foi encontrada
}


//Esta função é usada para exibir uma mensagem de erro e encerrar o programa em caso de erro.
void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}

void recebe_news(char *ip,char *port,char *id_topic){

	struct sigaction sa;
    sa.sa_handler = handler_ctrlc_process;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa,NULL);
	
	
    char msg[100];
    
    struct sockaddr_in addr;

    
    
    // set up the multicast address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(port));
    // bind the socket to the port
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // join the multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1);
    }

   
    // receive the multicast message
    socklen_t addrlen = sizeof(addr);
    while(1){
   		
        if ((recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr *)&addr, &addrlen)) < 0) {
        	break;
            perror("recvfrom");
            
        }
        printf("\n\nTOPICO %s\nNOTICIA de ULTIMA HORA: %s\n%s",id_topic, msg,menu);
        memset(msg, 0, sizeof(msg));
    }
    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    close(sock);
}

int main(int argc, char *argv[]) {

	
    struct sigaction sa;
    sa.sa_handler = handler_ctrlc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa,NULL);
    
    char endServer[100];//nome do servidor a ser conectado
    int fd, nread;//descritor de arquivo de socket, inteiro usado para contar o número de bytes lidos
    struct sockaddr_in server_addr;//estrutura para armazenar o endereço do servidor
    char buffer[BUF_SIZE];//buffer para ler e escrever dados
    struct hostent *hostPtr;
    char comando[BUF_SIZE];
    char * token, *token_config;
    int flag_read=1;
    char *port,*ip;
   

    // Verifica se os argumentos da linha de comando são válidos
    if (argc != 3) {
        printf("Usage: %s <endereço do servidor> <PORTO_NOTICIAS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    strcpy(endServer, argv[1]);
    //nome do servidor a ser conectado
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Não consegui obter endereço");

    // cria um socket com a família de endereços IPV4 e o descritor de arquivo do socket é armazenado na variável "client_fd"
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        erro("na funcao socket");
    }

    // "server_addr" é preenchida com informações sobre o endereço do servidor
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) < 0) {
        erro("na funcao inet_pton");
    }

    // Conecta o socket ao servidor
    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        erro("na funcao connect");
    }
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
	}

   
    while(1){
    	memset(buffer,0,sizeof (buffer));
    	nread = read(fd, buffer, BUF_SIZE);
    	
    	if(verificaSubstring(buffer,"aceite")){
    		token=strtok(buffer,"!");
			printf("%s!\n",token);
			token=strtok(NULL,"!");
			strcpy(menu,token);
			
    		break;
    	}
    	write(STDOUT_FILENO, buffer, nread);
    	
    	do{
            fgets(comando, BUF_SIZE, stdin);
        }while(comando[0]=='\n');
        
        comando[strlen(comando)-1]='\0';
        
        fflush(stdout);
        // escreve o comando para o servidor
        write(fd, comando, strlen(comando));
        
    }
    
   

    do{
        comando[0]='\0';

        printf("%s",menu);

        do{
            fgets(comando, BUF_SIZE, stdin);
        }while(comando[0]=='\n');


        comando[strlen(comando)-1]='\0';

        fflush(stdout);
        // escreve o comando para o servidor
        write(fd, comando, strlen(comando));

        

        token=strtok(comando," ");
        
        if(!strcmp(comando,"QUIT")){
        	kill(0,SIGINT);
            nread = read(fd, buffer, BUF_SIZE);
            write(STDOUT_FILENO, buffer, nread);
            break;
        }

        else if(!strcmp(token,"SUBSCRIBE_TOPIC")){
            flag_read = 0;
            memset(buffer,0,sizeof (buffer));

            nread = read(fd, buffer, BUF_SIZE);
           
            token=strtok(NULL," ");

            token_config=strtok(buffer,";");

            if(!strcmp(token_config,"leitor")) {


                token_config=strtok(NULL,";");

                ip = token_config;
                printf("Endereco MULTICAST: %s\nSUBSCRITO!\n",ip);

                token_config = strtok(NULL, ";");
                port = token_config;
                
                
                if (fork() == 0){
             
                    recebe_news(ip, port, token);
                    exit(0);
                }
              

            }else{
                printf("Comando Invalido\n");
            }
        }else{
            flag_read=1;
        }
        
        if(flag_read==1){
            nread = read(fd, buffer, BUF_SIZE);

            if(verificaSubstring(buffer,"ENCERRAR")){
            	kill(0,SIGINT);
               	printf("ENCERRANDO....\n");
                break;
            }else{
                write(STDOUT_FILENO, buffer, nread);
            }

        }
        memset(buffer,0,sizeof (buffer));

    }while ( nread>0);
    
    wait(NULL);
    close(fd);


    return 0;
}
