#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#define BUF_SIZE 100
#define MAX_CLNT 256

void *handle_clnt(void* arg);
void send_msg(char* msg, int len);
void error_handling(char* msg);

int clnt_cnt_first = 0;
int clnt_cnt_second = 0;
//int clnt_socks[MAX_CLNT];
int clnt_socks_first[MAX_CLNT];
int clnt_socks_second[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char* argv[])
{
	int serv_sock, clnt_sock;
	int pass;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	char key[4];
	pthread_t t_id;
	if(argc!=2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error!");
	while(1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		pass = read(clnt_sock, key, sizeof(key));
		key[3] = '\0';
		printf("%s\n", key);
		if(!strcmp(key, "one")){
			printf("it is one chat room!\n");
			pthread_mutex_lock(&mutx);
			clnt_socks_first[clnt_cnt_first++] = clnt_sock;
			pthread_mutex_unlock(&mutx);

			pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
			pthread_detach(t_id);
			printf("Connected client IP : %s \n", inet_ntoa(clnt_adr.sin_addr));
		} else if(!strcmp(key, "two")){
			printf("it is two chat room!\n");
			pthread_mutex_lock(&mutx);
			clnt_socks_second[clnt_cnt_second++] = clnt_sock;
			pthread_mutex_unlock(&mutx);

			pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
			pthread_detach(t_id);
			printf("Connected client IP : %s \n", inet_ntoa(clnt_adr.sin_addr));
		}
		
	}

	close(serv_sock);
	return 0;
}

void* handle_clnt(void* arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	
	while((str_len = read(clnt_sock, msg, sizeof(msg)))!=0){
		pthread_mutex_lock(&mutx);
		for(i=0; i<clnt_cnt_first; i++)
			if(clnt_socks_first[i] == clnt_sock)
				send_msg(msg, str_len);
			else
				printf("wait...\n");
		pthread_mutex_unlock(&mutx);
	}


	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt_first; i++)
	{
		if(clnt_sock == clnt_socks_first[i])
		{
			while(i++< clnt_cnt_first-1)
				clnt_socks_first[i] = clnt_socks_first[i+1];
			break;
		}
	}

	clnt_cnt_first--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len)
{
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt_first; i++)
		write(clnt_socks_first[i], msg, len);
	pthread_mutex_unlock(&mutx);
}

void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

















