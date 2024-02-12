#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>


#define MAX_USER 20
#define BUF_SIZE 1024
#define MAX_TOPICS 5
#define MAX_PORT_SUBSCRIBES 5000
#define MAX_ADDRESS 1000
#define MAX_NOTICIAS 100
#define MAX_GRUPOS 50


int PORT_CONFIG;
int PORT_NOTICIAS;

int shmid_array;


sem_t * sem_shared_var_array;

int fd;





void erro(char *s) {
    perror(s);
    exit(1);
}


typedef struct{
  int id;
  char titulo[30];
  char multicast_address[30];
  char port[30];
  char noticias[MAX_NOTICIAS][MAX_NOTICIAS];
  int num_noticias;
}Topico;

typedef struct{
    int numero_grupos;
    char username[30];
    char password[30];
    char type[30];
    int grupos_topics_id[MAX_GRUPOS];
}User;

typedef struct {
    Topico topics[MAX_TOPICS];
    User array[MAX_USER];
    int ports[MAX_PORT_SUBSCRIBES];
    char addresses[MAX_ADDRESS][MAX_ADDRESS];
    int num_topics,num_ports,num_addresses,num_users_on;
    char terminar[20];
    int users_on[MAX_USER];
    int indice;
} Memory;

Memory *memory;


/*
void imprimir_topics() {
    printf("VOU IMPRIMIR OS TOPICOS:\n");
    for (int i = 0; i < memory->num_topics; i++) {
        printf("Tópico ID: %d\n", memory->topics[i].id);
        printf("Título: %s\n", memory->topics[i].titulo);
        printf("Endereço multicast: %s\n", memory->topics[i].multicast_address);

        // Imprimir as notícias do tópico
        printf("Notícias:\n");
        for (int j = 0; j < MAX_NOTICIAS; j++) {
            if (memory->topics[i].noticias[j][0] != '\0') {
                printf("- %s\n", memory->topics[i].noticias[j]);
            }
        }

        printf("TOPIC %d IMPRIMIDO\n", i);
        printf("\n");
    }
}
*/


int is_string_numeric(char* str) {
  int i=0;
    while (str[i]!= '\0') {
        if (!isdigit(str[i])) {
            return 0;
        }
        i++;
    }
    return 1;
}

int remove_user(char *username){
    sem_wait(sem_shared_var_array);
    for(int i=0;i<MAX_USER;i++){
        if(!strcmp(username,memory->array[i].username)){
            for(int j=i;j<MAX_USER-1;j++){
                memory->array[j]=memory->array[j+1];
            }
            sem_post(sem_shared_var_array);
            return 1;
        }
    }
    sem_post(sem_shared_var_array);
    return 0;
}


void pos_ocorrencias(char *linha,int *indice_inicial, int *indice_final){
  int tamanho = strlen(linha);
  *indice_inicial = -1;
  *indice_final = -1;

  // Procura a primeira ocorrência do caractere
  for (int i = 0; i < tamanho; i++) {
      if (linha[i] == ';') {
          *indice_inicial = i;
          break;
      }
  }

  // Procura a última ocorrência do caractere
  for (int i = tamanho-1; i >= 0; i--) {
      if (linha[i] == ';') {
          *indice_final = i;
          break;
      }
  }
}

int verifica_topico(char *id){
  int flag=0;
  for (int i = 0; i < memory->num_topics; i++) {
    if(memory->topics[i].id==atoi(id)){
      flag=1;
      break;
    }
  }
  return flag;
}

char * gerarPortoUnico() {
    srand(time(NULL)); // define a semente para a função rand() com base no tempo atual

    int num;
    int contem_num = 1; // assume que o número gerado está presente no array

    // Gerar um número aleatório até encontrar um número único que ainda não está presente no array
    while (contem_num) {
        num = rand() % 1001 + 2000; // gera um número aleatório entre 100 e 500
        contem_num = 0;

        for (int i = 0; i < memory->num_ports; i++) {
            if (memory->ports[i] == num) {
                contem_num = 1; // o número gerado está presente no array
                break;
            }
        }
    }
    char* num_str = malloc(sizeof(char) * 4); // aloca memória para a string do número
    sprintf(num_str, "%d", num);

    
    sem_wait(sem_shared_var_array);
    memory->ports[memory->num_ports]=num;
    (memory->num_ports)++;
    sem_post(sem_shared_var_array);
    return num_str;
}

char* generate_multicast_ip() {
    struct in_addr addr;
    unsigned int ip;
    char* str_ip = malloc(INET_ADDRSTRLEN);
    int contem_ip;

    srand(time(NULL));
    do {
        // Gerar um número aleatório entre 224 e 239
        ip = rand() % (239 - 224 + 1) + 224;
        addr.s_addr = htonl((224 << 24) | (ip << 16) | (rand() % (1 << 16)));

        // Verificar se o endereço gerado já existe no array
        contem_ip = 0;
        for (int i = 0; i < memory->num_addresses; i++) {
            if (strcmp(memory->addresses[i], inet_ntoa(addr)) == 0) {
                contem_ip = 1; // o endereço gerado já existe no array
                break;
            }
        }
    } while (contem_ip);

    inet_ntop(AF_INET, &addr, str_ip, INET_ADDRSTRLEN);

    // Adicionar o endereço gerado ao array
    sem_wait(sem_shared_var_array);
    strcpy(memory->addresses[memory->num_addresses], str_ip);
    (memory->num_addresses)++;
    sem_post(sem_shared_var_array);

    return str_ip;
}


int read_file(char * file_name){
    FILE *file;
    char linha[100];
    memory->indice=0;
    User var;

    file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Erro ao abrir o ficheiro\n");
        return 0;
    }
    int i,j;
    int first_pos,last_pos;
    while (fgets(linha, sizeof(linha), file) && memory->indice!=MAX_USER) {

        i=0,j=0;
        linha[strlen(linha)-1]='\0';

        pos_ocorrencias(linha,&first_pos,&last_pos);
        while(i<first_pos){
          if (isalpha(linha[i])) {
            var.username[j]=linha[i];
          }else{
            return 0;
          }
          j++;
          i++;
        }
        var.username[j]='\0';

        j=0;
        i=first_pos+1;
        while (i<last_pos) {
          var.password[j]=linha[i];
          i++;
          j++;
        }
        var.password[j]='\0';

        j=0;
        i=last_pos+1;
        while(linha[i]!='\0'){
          var.type[j]=linha[i];
          i++;
          j++;
        }
        var.type[j]='\0';


        if((strcmp(var.type,"leitor")!=0) && (strcmp(var.type,"administrator")!=0) && (strcmp(var.type,"jornalista")!=0)){
            return 0;
        }
        memory->array[memory->indice] = var;
        (memory->indice)++;

      }

    fclose(file);
    return 1;
}


int open_file(char *file_name){
  int f;
  f=open(file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  /*if(f==NULL){
    erro("Ao abrir o ficheiro de log");
    return f;
  }*/
  return f;
}
void write_file(int f){
    char temp[100];
    for(int i=0;i<memory->indice;i++){
      sprintf(temp,"%s;%s;%s\n",memory->array[i].username,memory->array[i].password,memory->array[i].type);
      write(f,temp,strlen(temp));
    }

    //fflush(f);
    //fflush(stdout);
}

void process_client(int client_fd){
  int sock;
  struct sockaddr_in addr;
  User user;
  int authentication=0;
  int nread;
  char buffer[BUF_SIZE];
  char mensagem_servidor[500];
  int id_topic;
  char *titulo_topic;
  char *porto_enviar;
  char configuracoes_enviar_multicast[100];

  int flag=1;
  int indice_user_on;



  // verificar autenticacao
  do{
      if(!strcmp(memory->terminar,"ENCERRAR")){
          break;
      }
      memset(mensagem_servidor, 0, sizeof(mensagem_servidor));

  	strcpy(mensagem_servidor,"Digite o seu username\n");
  	//escreva-a no socket do utilizador
  	write(client_fd, mensagem_servidor, strlen(mensagem_servidor));

    nread = read(client_fd, buffer, BUF_SIZE-1);

    if(buffer[0]=='\n'){
        continue;
    }
    

    buffer[nread] = '\0';
    printf("Recebido: %s\n",buffer);
    strcpy(user.username,buffer);


      strcpy(mensagem_servidor,"Digite a sua password\n");
    write(client_fd, mensagem_servidor, strlen(mensagem_servidor));

    nread = read(client_fd, buffer, BUF_SIZE-1);



    buffer[nread] = '\0';
    printf("Recebido: %s\n",buffer);

    strcpy(user.password,buffer);
    

    for(int i=0;i<memory->indice;i++){
      if ((!strcmp(user.username,memory->array[i].username)) && (!strcmp(user.password,memory->array[i].password))) {
        authentication=1;
        strcpy(user.type,memory->array[i].type);
        indice_user_on=i;
        break;

      }
    }
    if (!strcmp(user.type,"administrator")) {
      authentication=0;
    }
    if(authentication==0 ){
      strcpy(mensagem_servidor,"\nPermissão negada. Tente novamente\n");
      write(client_fd, mensagem_servidor, strlen(mensagem_servidor));

    }else if(authentication==1){
    	char permissao_accept[500];
    
    	if(!strcmp(user.type,"leitor")){
    		sprintf(permissao_accept, "\nPermissão aceite. Bem-vindo(a) %s!\n-------------MENU-------------\nLIST_TOPICS\nLIST_NEWS\nSUBSCRIBE_TOPIC <id do tópico>\nQUIT\n-----------------------------\n\n", user.username);
              
          }
        else if(!strcmp(user.type,"jornalista")){
        	sprintf(permissao_accept, "\nPermissão aceite. Bem-vindo(a) %s!\n-------------MENU-------------\nLIST_TOPICS\nCREATE_TOPIC <id do tópico> <título do tópico>\nSEND_NEWS <id do tópico> <noticia>\nQUIT\n-----------------------------\n\n", user.username);
              
         }
            
      //mensagem_servidor= "Permissão aceite\n";
      write(client_fd, permissao_accept, strlen(permissao_accept));
    }

  }while(authentication==0);



  do {



      memset(mensagem_servidor, 0, sizeof(mensagem_servidor));
    flag=1;
    

      if(!strcmp(memory->terminar,"ENCERRAR")){
          write(client_fd, memory->terminar, strlen(memory->terminar));
          for (int i = 0; i < memory->num_users_on; ++i) {
              close(memory->users_on[i]);
          }
          close(fd);
          kill(0,SIGINT);
      }

    nread = read(client_fd, buffer, BUF_SIZE-1);

    //printf("%s\n",buffer );
    buffer[nread] = '\0';

      if(buffer[0]!='\0'){
          printf("Recebido: %s\n",buffer);
      }else{
          printf("Recebido: ENTER\n");
      }

    char * token;
    token=strtok(buffer," ");

	  //Se o cliente enviar a string "SAIR", o servidor responde com uma mensagem de despedida e sai do loop
    if ((!strcmp(user.type,"leitor") || !strcmp(user.type,"jornalista")) && (!strcmp(token, "QUIT"))) {
      //o cliente deve ser removido de todos os grupos multicast que tenha subscrito
      char goodbye_message[] = "Até logo!\n";
      write(client_fd, goodbye_message, strlen(goodbye_message));
      break;
    }
    else if((!strcmp(user.type,"leitor") || !strcmp(user.type,"jornalista")) && (!strcmp(token, "LIST_TOPICS"))){
      char lista[1000]=""; // define uma string para armazenar a string resultante
      int pos = 0; // posição atual no buffer

      // percorre o array de tópicos
      for (int i = 0; i < memory->num_topics; i++) {

        // adiciona a informação do ID do tópico ao buffer
        pos += sprintf(lista + pos, "\nID: %d\n", memory->topics[i].id);

        // adiciona a informação do título do tópico ao buffer
        pos += sprintf(lista + pos, "TITULO: %s\n", memory->topics[i].titulo);
        // adiciona uma linha em branco para separar os tópicos
        pos += sprintf(lista + pos, "\n");
      }

      if(memory->num_topics==0){
        strcpy(lista,"\nNao existem topicos\n");
      }
      write(client_fd, lista, strlen(lista));

    }
    else if(!strcmp(user.type,"leitor") && !strcmp(token, "LIST_NEWS")){
        char lista_news[1000]=""; // define uma string para armazenar a string resultante
        int posi = 0; // posição atual na lista

        for (int i = 0; i < memory->array[indice_user_on].numero_grupos; ++i) {
            for (int j = 0; j < memory->num_topics; ++j) {
                if (memory->topics[j].id == memory->array[indice_user_on].grupos_topics_id[i]) {
                    posi += sprintf(lista_news + posi, "\nTITULO: %s\n", memory->topics[j].titulo);

                    // percorre a matriz de notícias do tópico e adiciona cada notícia à lista
                    for (int k = 0; k < memory->topics[j].num_noticias; ++k) {
                        if (memory->topics[j].noticias[k][0] != '\0') {
                            posi += sprintf(lista_news + posi, "- %s\n", memory->topics[j].noticias[k]);
                        }
                    }
                }
            }
        }

        if (posi == 0) {
            strcpy(lista_news, "\nNao existem noticias\n");
        }
        write(client_fd, lista_news, strlen(lista_news));
    }
    else if((!strcmp(user.type,"leitor")) && (!strcmp(token, "SUBSCRIBE_TOPIC"))){
    /*
        char tipo[40];
        sprintf(tipo,"%s;",user.type);
        write(client_fd, tipo, strlen(tipo));*/
      token=strtok(NULL," ");
      if((token==NULL) || (verifica_topico(token)==0)){
        //reponder que tem o comando mal
        flag=0;
      }
      if(flag==0){
        strcpy(mensagem_servidor,"\nArgumentos inválidos\ncommand: SUBSCRIBE_TOPIC <id do tópico>\n");
        write(client_fd, mensagem_servidor, strlen(mensagem_servidor));
      }else{
          id_topic= atoi(token);
        for(int i=0; i<memory->num_topics;i++){
            printf("I: %d",i);
          if(memory->topics[i].id==id_topic){
              sprintf(configuracoes_enviar_multicast,"%s;%s;%s;",user.type,memory->topics[i].multicast_address,memory->topics[i].port);
              write(client_fd, configuracoes_enviar_multicast, strlen(configuracoes_enviar_multicast));
          }
        }
        sem_wait(sem_shared_var_array);
        memory->array[indice_user_on].grupos_topics_id[memory->array[indice_user_on].numero_grupos] = id_topic;
       memory->array[indice_user_on].numero_grupos++;
        sem_post(sem_shared_var_array);
      }
    }
    else if(!strcmp(user.type,"jornalista") && (!strcmp(token, "CREATE_TOPIC"))){
      Topico new_topic;
      token=strtok(NULL," ");
      flag=1;

      if((token==NULL) || (verifica_topico(token)==1)){
        flag=0;
      }else{
          id_topic=atoi(token);

      }
      token=strtok(NULL," ");

      if(token==NULL){
        flag=0;

      }else{
        titulo_topic=token;
      }

      if(flag==1){
          new_topic.num_noticias=0;
          strcpy(new_topic.port,gerarPortoUnico());
          new_topic.id=id_topic;
          strcpy(new_topic.titulo,titulo_topic);
          strcpy(new_topic.multicast_address,generate_multicast_ip());
        sem_wait(sem_shared_var_array);
          memory->topics[memory->num_topics]=new_topic;
          (memory->num_topics)++;
        sem_post(sem_shared_var_array);

          strcpy(mensagem_servidor,"\nTopico Criado\n");
        write(client_fd, mensagem_servidor, strlen(mensagem_servidor));
      }else{
        strcpy(mensagem_servidor,"\nArgumentos inválidos\ncommand: CREATE_TOPIC <id do tópico> <título do tópico>\n");
        write(client_fd, mensagem_servidor, strlen(mensagem_servidor));
      }
    }

    else if(!strcmp(user.type,"jornalista") && (!strcmp(token, "SEND_NEWS")) ){
        flag=1;
        char noticia[100] = "";
        token=strtok(NULL," ");
      if((token==NULL) || verifica_topico(token)==0){

        flag=0;
      }else{
        id_topic=atoi(token);
      }
      token=strtok(NULL," ");
      if(token==NULL){
        flag=0;
      }else{
          strcat(noticia, token);
          token = strtok(NULL, "");
          while (token != NULL) {
              strcat(noticia, " ");
              strcat(noticia, token);
              token = strtok(NULL, "");
          }
          for (int i = 0; i < memory->num_topics; ++i) {
              if (memory->topics[i].id == id_topic) {
                  // Encontrar a primeira posição vazia na matriz
                  int j = 0;
                  while (j < MAX_NOTICIAS && memory->topics[i].noticias[j][0] != '\0') {
                      j++;
                  }

                  // Copiar a nova string para a posição encontrada
                  if (j < MAX_NOTICIAS) {
                      strncpy(memory->topics[i].noticias[j], noticia, MAX_NOTICIAS - 1);
                      memory->topics[i].num_noticias++;
                      memory->topics[i].noticias[j][MAX_NOTICIAS - 1] = '\0'; // definir o caractere nulo de término
                  }
              }
          }

      }

      char *endereco_topic;

      if(flag!=0){
        for(int i=0; i<memory->num_topics;i++){
          if(memory->topics[i].id==id_topic){
            endereco_topic=memory->topics[i].multicast_address;
            porto_enviar=memory->topics[i].port;
          }

        }

        // create a UDP socket
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
          perror("socket");
          exit(1);
        }
        // set up the multicast address structure
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(endereco_topic);
        addr.sin_port = htons(atoi(porto_enviar));
        // enable multicast on the socket
        int enable = 225;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0) {
          perror("setsockopt");
          exit(1);
        }
        // send the multicast message
        if (sendto(sock, noticia, strlen(noticia), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
          perror("sendto");
          exit(1);
        }
        
        strcpy(mensagem_servidor,"\nNoticia enviada.\n");
      write(client_fd, mensagem_servidor, strlen(mensagem_servidor));

      }else{
        strcpy(mensagem_servidor,"\nArgumentos inválidos\ncommand: SEND_NEWS <id do tópico> <noticia>\n");
        write(client_fd, mensagem_servidor, strlen(mensagem_servidor));
      }

    }
    else{
      //responder que o comando que digitou está incorreto
      strcpy(mensagem_servidor,"\nComando Invalido\n");
      write(client_fd, mensagem_servidor, strlen(mensagem_servidor));
    }
  } while ((nread>0));


  	// fecha a conexão com o cliente atual
	close(client_fd);
}


void comunicacao_tcp(){
  //descritores dos socket do servidor e do cliente
  int  client;
  //endereços do servidor e do cliente
  struct sockaddr_in addr, client_addr;
  //tamanho do endereço do cliente
  int client_addr_size;
  //inicializada com as informações do endereço do servidor
  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PORT_NOTICIAS);
  // cria um socket com a família de endereços IPV4 e o descritor do socket é armazenado na variável "fd"
  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      erro("na funcao socket");
  }
    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        erro("na funcao setsockopt");

  // Associa o socket criado com o endereço do servidor
  if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    erro("na funcao bind");

  if( listen(fd, 5) < 0)
   erro("na funcao listen");
  // Define o tamanho do endereço do cliente
  client_addr_size = sizeof(client_addr);
  while (1) {
      

    //clean finished child processes, avoiding zombies
    //must use WNOHANG or would block whenever a child process was working
    while(waitpid(-1,NULL,WNOHANG)>0);

    //wait for new connection
    client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *)&client_addr_size);
      sem_wait(sem_shared_var_array);
    memory->users_on[memory->num_users_on]=client;
    memory->num_users_on++;
      sem_post(sem_shared_var_array);



    if (client > 0) {
    // Cria um processo filho para lidar com a conexão
      if (fork() == 0) {
      // Fecha o socket do servidor no processo filho
        close(fd);
        // Processa a conexão no processo filho
        process_client(client);
        // Encerra o processo filho
        exit(0);
      }
      // Fecha o socket do cliente no processo pai
    close(client);
    }
  }
}

void udp_comunicacao(char *file_name){
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUF_SIZE];
    int autenticacao=0;
    char nome[20],passe[20];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT_CONFIG);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr*)&si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }


    int flag=1;

    User var;
    int sendto_len;

    int config_id;

    if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
    }
    memset(buf, 0, sizeof(buf));

    if ((sendto_len = sendto(s, "\n-------------MENU-----------\nADD_USER {username} {password} {administrador/cliente/jornalista}\nDEL {username}\nLIST\nQUIT\nQUIT_SERVER\n----------------------------\n", sizeof("\n-------------MENU-----------\nADD_USER {username} {password} {administrador/cliente/jornalista}\nDEL {username}\nLIST\nQUIT\nQUIT_SERVER\n----------------------------\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
        erro("Erro no sendto do bin");
    }

  do{

      if ((sendto_len = sendto(s, "Digite o username", sizeof ("Digite o username"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
          erro("Erro no sendto do bin");
      }

      if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
          erro("Erro no recvfrom");
      }

      printf("Buffer: %s\n",buf);


      if(buf[0]!='\n'){
          buf[recv_len-1] = '\0';
      }
      strcpy(nome,buf);

      memset(buf, 0, sizeof(buf));



      if ((sendto_len = sendto(s, "Digite a password", sizeof ("Digite a password"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
          erro("Erro no sendto do bin");
      }

      if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
          erro("Erro no recvfrom");
      }
      printf("Buffer: %s\n",buf);

      if(buf[0]!='\n'){
          buf[recv_len-1] = '\0';
      }
      strcpy(passe,buf);

      memset(buf, 0, sizeof(buf));

      for(int i=0;i<memory->indice;i++){
          if ((!strcmp(nome,memory->array[i].username)) && (!strcmp(passe,memory->array[i].password))) {
              autenticacao=1;
              break;
          }
      }
      if(autenticacao==1){
          printf("entrei aqui");
          if ((sendto_len = sendto(s, "Permissao aceite", sizeof("Permissao aceite"),0, (struct sockaddr *) &si_outra,slen)) == -1) {
              erro("Erro no sendto do bin");
          }
      }else{
          if ((sendto_len = sendto(s, "Permissao negada. Tente novamente", sizeof ("Permissao negada. Tente novamente"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
              erro("Erro no sendto do bin");
          }
      }


  }while(autenticacao==0);



    // Espera recepção de mensagem (a chamada é bloqueante)
    while(flag){

        memset(buf, 0, sizeof(buf));

        if ((recv_len = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        if(recv_len!=1){

            // Para ignorar o restante conteúdo (anterior do buffer)
            if(buf[0]!='\n'){
                buf[recv_len-1] = '\0';
            }
            char * token;


            token=strtok(buf," ");

            if(token[0]!='\0'){
                printf("token: %s\n",token);
            }else{
                printf("token: ENTER\n");
            }
            if(!strcmp(token,"ADD_USER")){
                token=strtok(NULL," ");
                if(token==NULL){
                    if ((sendto_len = sendto(s, "ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n", sizeof ("ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                    continue;
                }
                printf("username: %s\n",token);
                strcpy(var.username,token);
                token=strtok(NULL," ");
                if(token==NULL){
                    if ((sendto_len = sendto(s, "ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n", sizeof ("ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                    continue;
                }
                printf("password: %s\n",token);
                strcpy(var.password,token);
                token=strtok(NULL," ");
                if(token==NULL){
                    if ((sendto_len = sendto(s, "ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n", sizeof ("ERRO: ADD_USER <username> <password> <administrator/leitor/jornalista>\n\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                    continue;
                }else if(strcmp(token,"administrator")!=0 && strcmp(token,"leitor")!=0 && strcmp(token,"jornalista")!=0){
                    if ((sendto_len = sendto(s, "Tipo incorreto <administrator/leitor/jornalista>\n", sizeof ("Tipo incorreto <administrator/leitor/jornalista>\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                    continue;
                }
                printf("tipo: %s\n",token);
                strcpy(var.type,token);
                sem_wait(sem_shared_var_array);
                memory->array[memory->indice]=var;
                memory->indice++;
                sem_post(sem_shared_var_array);
                printf("indice: %d\n",memory->indice);

                if ((sendto_len = sendto(s, "Utilizador adicionado\n", sizeof ("Utilizador adicionado\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                    erro("Erro no recvfrom do bin");
                }
            }else if(!strcmp(token,"DEL")){
                token=strtok(NULL," ");
                if(token!=NULL){
                    if(remove_user(token)){
                        memory->indice--;
                        if ((sendto_len = sendto(s, "Utilizador eliminado\n", sizeof ("Utilizador eliminado\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                            erro("Erro no recvfrom do bin");
                        }
                    }else{
                        if ((sendto_len = sendto(s, "Utilizador nao existente\n", sizeof ("Utilizador nao existente\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                            erro("Erro no recvfrom do bin");
                        }
                    }
                }else{
                    if ((sendto_len = sendto(s, "Parametros incorretos\nDEL {username}\n\n", sizeof ("Parametros incorretos\nDEL {username}\n\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                }

            }else if(!strcmp(token,"LIST")){

                if ((sendto_len = sendto(s, "-----LISTAGEM-----\n", sizeof ("-----LISTAGEM-----\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                    erro("Erro no recvfrom do bin");
                }
                char temp[500];
                if(!memory->indice){
                    if ((sendto_len = sendto(s, "Sem utilizadores para listar\n", sizeof ("Sem utilizadores para listar\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                }else{
                    int offset = 0;
                    for(int i=0;i<memory->indice;i++){
                        offset += sprintf(temp+offset,"Username: %s\nPassword: %s\nType: %s\n\n",memory->array[i].username,memory->array[i].password,memory->array[i].type);

                    }
                    if ((sendto_len = sendto(s, temp, strlen(temp), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                        erro("Erro no recvfrom do bin");
                    }
                    printf("TOODOS OS USER:\n%s",temp);
                }


            }else if(!strcmp(token,"QUIT")){
                flag=0;
                if ((sendto_len = sendto(s, "Sair\n", sizeof ("Sair\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                    erro("Erro no recvfrom do bin");
                }
            }else if(!strcmp(token,"QUIT_SERVER")){
                strcpy(memory->terminar,"ENCERRAR");
                flag=0;
                if ((sendto_len = sendto(s, "Sair do servidor\n",sizeof ("Sair do servidor\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                    erro("Erro no recvfrom do bin");
                }
            }else{
                if ((sendto_len = sendto(s, "Comando nao reconhecido\n",sizeof ("Comando nao reconhecido\n"), 0, (struct sockaddr *) &si_outra,slen)) == -1) {
                    erro("Erro no recvfrom do bin");
                }
            }
        }
    }
    config_id=open_file(file_name);
    write_file(config_id);
    close(config_id);

    close(s);
}



int main(int argc,char *argv[]) {

    if(argc!=4){
      printf("Numero inválido de argumentos\nCommand: news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}");
      return 0;
    }
    if(!is_string_numeric(argv[1]) || !is_string_numeric(argv[2])){
      printf("PORT_NOTICIAS e o PORT_NOTICIAS tem que ser valores inteiros\n");
      return 0;
    }

    int pid_udp, pid_tcp;

    PORT_CONFIG=atoi(argv[2]);
    PORT_NOTICIAS=atoi(argv[1]);


    shmid_array= shmget(IPC_PRIVATE, sizeof(Memory), IPC_CREAT | 0777);
    if (shmid_array == -1) {
        perror("shmget");
        exit(1);
    }

    // SMM para USERS
    memory = (Memory*) shmat(shmid_array, NULL, 0);
    if (memory == (Memory*) -1) {
        perror("shmat");
        exit(1);
    }
    memory->num_topics=0;
    memory->num_ports=0;
    memory->num_addresses=0;
    memory->num_users_on=0;


    sem_unlink("SEM_SHARED_VAR_ARRAY");
    sem_shared_var_array=sem_open("SEM_SHARED_VAR_ARRAY",O_CREAT|O_EXCL,0700,1);


    char *file_name;
    file_name=(argv[3]);

    if(!read_file(file_name)){
      printf("Erro ao ler ficheiro de configuracao\n");
      return 0;
    }


    if(fork()==0){
      pid_udp=getpid();
      //comunicacao_udp(file_name);
        udp_comunicacao(file_name);
      exit(0);
    }

    if(fork()==0){
      pid_tcp=getpid();
      comunicacao_tcp();
      exit(0);
    }


    wait(&pid_tcp);
    wait(&pid_udp);


    sem_close(sem_shared_var_array);
    sem_unlink("SEM_SHARED_VAR_ARRAY");


    if (shmdt(memory) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Libera a região de memória compartilhada
    if (shmctl(shmid_array, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    return 0;
 }
