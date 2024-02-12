#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


#define BUF_SIZE 5000

void erro(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc,char *argv[]) {
    if(argc!=3){
        printf("Numero inválido de argumentos\nCommand: client_udp {IP DO SERVIDOR} {PORTO_NOTICIAS}");
        return 0;
    }
    struct sockaddr_in si_servidor;
    int socket_cliente;
    socklen_t slen = sizeof(si_servidor);
    char buffer[BUF_SIZE];
    char comando[BUF_SIZE];
    char menu[200];

    // Cria um socket UDP
    if ((socket_cliente = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    // Configura a estrutura de endereço do servidor
    memset((char *)&si_servidor, 0, sizeof(si_servidor));
    si_servidor.sin_family = AF_INET;
    si_servidor.sin_port = htons(atoi(argv[2]));

    if (inet_aton(argv[1], &si_servidor.sin_addr) == 0) {
        fprintf(stderr, "Endereço IP inválido: %s\n", argv[1]);
        exit(1);
    }

    fgets(comando, BUF_SIZE, stdin);

    // Envia o comando para o servidor
    if (sendto(socket_cliente, comando, sizeof (comando), 0, (struct sockaddr *)&si_servidor, slen) == -1) {
        erro("Erro no sendto");
    }


    if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
        erro("Erro no recvfrom");
    }
    strcpy(menu,buffer);




    while(1) {
        memset(buffer, 0, sizeof(buffer));

        if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *) &si_servidor, &slen) == -1) {
            erro("Erro no recvfrom");
        }
        if (!strcmp(buffer, "Permissao aceite")){
            printf("%s\n", buffer);
            break;
        }else if(!strcmp(buffer, "Permissao negada. Tente novamente")){
            printf("%s\n", buffer);
            continue;
        }

        printf("%s\n", buffer);

        do{
            fgets(comando, BUF_SIZE, stdin);
        }while(comando[0]=='\n');

        comando[strcspn(comando, "\n")] = '\0';

        // Envia o comando para o servidor
        if (sendto(socket_cliente, comando, sizeof(comando), 0, (struct sockaddr *) &si_servidor, slen) == -1) {
            erro("Erro no sendto");
        }
    }

    printf("%s",menu);


    while(1){


        do{
            fgets(comando, BUF_SIZE, stdin);
        }while(comando[0]=='\n');

        comando[strcspn(comando, "\n")] = '\0';
        // Envia o comando para o servidor
        if (sendto(socket_cliente, comando, sizeof (comando), 0, (struct sockaddr *)&si_servidor, slen) == -1) {
            erro("Erro no sendto");
        }


        if(!strcmp(comando,"ADD_USER")){
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }

            printf("%s\n", buffer);
            printf("%s",menu);
            memset(buffer, 0, sizeof(buffer));
        }
        else if(!strcmp(comando,"DEL")){
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }
            printf("%s\n", buffer);
            printf("%s",menu);
            memset(buffer, 0, sizeof(buffer));
        }
        else if(!strcmp(comando,"LIST")){
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }
            printf("%s\n", buffer);
            memset(buffer, 0, sizeof(buffer));
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }
            printf("%s\n", buffer);
            printf("%s",menu);
            memset(buffer, 0, sizeof(buffer));
        }
        else if(!strcmp(comando,"QUIT") || !strcmp(comando,"QUIT_SERVER")){
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *)&si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }
            printf("%s\n", buffer);
            memset(buffer, 0, sizeof(buffer));
            break;
        }
        else{
            if (recvfrom(socket_cliente, buffer, BUF_SIZE, 0, (struct sockaddr *) &si_servidor, &slen) == -1) {
                erro("Erro no recvfrom");
            }
            printf("%s\n", buffer);
            printf("%s",menu);
            memset(buffer, 0, sizeof(buffer));
        }

    }

    close(socket_cliente);
    return 0;
}
