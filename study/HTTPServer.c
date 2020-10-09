/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPServer.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yohlee <yohlee@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/10/09 18:24:27 by yohlee            #+#    #+#             */
/*   Updated: 2020/10/09 18:25:42 by yohlee           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);

int main(int argc, char *argv[])
{
	int server_socket, client_socket;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	int client_address_len;
	char buf[BUFFER_SIZE];
	pthread_t t_id;

	if (argc!=2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	server_socket=socket(PF_INET, SOCK_STREAM, 0);
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);
	server_address.sin_port = htons(atoi(argv[1]));
	if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))==-1)
		error_handling("bind() error");
	if (listen(server_socket, 20)==-1)
		error_handling("listen() error");

	while(1)
	{
		client_address_len=sizeof(client_address);
		client_socket=accept(server_socket, (struct sockaddr*)&client_address, (socklen_t *)&client_address_len);
		printf("Connection Request : %s:%d\n", 
			inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
		pthread_create(&t_id, NULL, request_handler, &client_socket);
		pthread_detach(t_id);
	}
	close(server_socket);
	return 0;
}

void* request_handler(void *arg)
{
	int client_socket=*((int*)arg);
	char req_line[SMALL_BUF];
	FILE* clnt_read;
	FILE* clnt_write;
	
	char method[10];
	char ct[15];
	char file_name[30];
  
	clnt_read=fdopen(client_socket, "r");
	clnt_write=fdopen(dup(client_socket), "w");
	fgets(req_line, SMALL_BUF, clnt_read);	
	if (strstr(req_line, "HTTP/")==NULL)
	{
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		return NULL;
	}
	
	strcpy(method, strtok(req_line, " /"));
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(ct, content_type(file_name));
	if (strcmp(method, "GET")!=0)
	{
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		return NULL;
	 }

	fclose(clnt_read);
	send_data(clnt_write, ct, file_name); 
	return NULL;
}

void send_data(FILE* fp, char* ct, char* file_name)
{
	char protocol[]="HTTP/1.0 200 OK\r\n";
	char server[]="Server:Linux Web Server \r\n";
	char cnt_len[]="Content-length:2048\r\n";
	char cnt_type[SMALL_BUF];
	char buf[BUFFER_SIZE];
	FILE* send_file;
	
	sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
	send_file=fopen(file_name, "r");
	if (send_file==NULL)
	{
		send_error(fp);
		return;
	}

	/* «Ï¥ı ¡§∫∏ ¿¸º€ */
	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);

	/* ø‰√ª µ•¿Ã≈Õ ¿¸º€ */
	while(fgets(buf, BUFFER_SIZE, send_file)!=NULL) 
	{
		fputs(buf, fp);
		fflush(fp);
	}
	fflush(fp);
	fclose(fp);
}

char* content_type(char* file)
{
	char extension[SMALL_BUF];
	char file_name[SMALL_BUF];
	strcpy(file_name, file);
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));
	
	if (!strcmp(extension, "html")||!strcmp(extension, "htm")) 
		return "text/html";
	else
		return "text/plain";
}

void send_error(FILE* fp)
{	
	char protocol[]="HTTP/1.0 400 Bad Request\r\n";
	char server[]="Server:Linux Web Server \r\n";
	char cnt_len[]="Content-length:2048\r\n";
	char cnt_type[]="Content-type:text/html\r\n\r\n";
	char content[]="<html><head><title>NETWORK</title></head>"
	       "<body><font size=+5><br>ø¿∑˘ πﬂª˝! ø‰√ª ∆ƒ¿œ∏Ì π◊ ø‰√ª πÊΩƒ »Æ¿Œ!"
		   "</font></body></html>";

	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	fflush(fp);
}

void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
