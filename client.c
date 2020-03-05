#include<event2/event.h>
#include<event2/buffer.h>
#include<event2/bufferevent.h>
#include<event2/listener.h>
#include<event2/util.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include"message.h"


int read_n = 0;
int cont_ = 1280;
message_t msg_r ;

struct event_base* base;

void print_err(const char* error){
	printf("--%s\n",error);
}

void read_cb(struct bufferevent* buffer,void* arg){
	//printf("flag \n");
	while(1){
		//printf("read_n = %d , cont_ = %d \n",read_n,cont_);
		read_n = bufferevent_read(buffer,((char*)&msg_r) + (1280 - cont_),cont_ );
		if(read_n == 0){
			break;	
		}
		//printf("read_n = %d\n",read_n);
		cont_ = cont_ - read_n;
		//printf("cont_ = %d\n",cont_);
		if(cont_ == 0){
			printf("\033[31m%s say : ",msg_r.name);
			printf("%s \033[0m\n",msg_r.msg);
			cont_ = 1280;
			read_n = 0;
		}
		//printf("read_n = %d  ",read_n);
	}	
}

void ev_dispatch(){
	event_base_dispatch(base);
}

int main(int argc, char*argv[]){
	struct sockaddr_in srv;
	struct bufferevent* buffer;
	char name[256];
	char text[1024];
	pthread_t thread_id;
	message_t msg;
	
	base = event_base_new();
	memset(&srv,0,sizeof(srv));
	srv.sin_port = htons(10001);
	srv.sin_family = AF_INET;
	inet_pton(AF_INET,argv[1],&srv.sin_addr);

	buffer = bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

	if(bufferevent_socket_connect(buffer,(struct sockaddr*)&srv,sizeof(srv)) == -1)
	{
			
	}

	bufferevent_setcb(buffer,read_cb,NULL,NULL,NULL);
	bufferevent_enable(buffer, EV_READ | EV_WRITE);

	
	if(pthread_create(&thread_id,NULL,(void*)ev_dispatch,NULL) != 0){
		print_err("create thread failed");	
	}
	usleep(10000);

	printf("\033[1;34mplease bind your chat name :\033[0m\n");
	scanf("%s",name);
	memcpy(msg.name,name,sizeof(name));

	printf("\033[1;34minput your message (next) :\033[0m\n");
	while(1){
		//printf("please input your text\n");
		scanf("%s",text);
		memcpy(msg.msg,text,sizeof(text));
		bufferevent_write(buffer,&msg,sizeof(msg));
	}
	return 0;
}
