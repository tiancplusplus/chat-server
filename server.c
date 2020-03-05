#include<event2/event.h>
#include<event2/buffer.h>
#include<event2/bufferevent.h>
#include<event2/listener.h>
#include<event2/util.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"message.h"

struct event_base* base;
struct bufferevent* buffers[MAX_CONNECTION];
int cur_con = 0;
int msgs_len = 2;

void print_err(const char* error){
	printf("---%s---\n",error);
}
void read_cb(struct bufferevent* buffer,void* arg){

	message_t* msgs = arg;
	int i = 0;
	while(bufferevent_read(buffer,(msgs + msgs_len),sizeof(message_t)) != 0){
		if(msgs_len == MAX_MESSAGE_TOTAL - 1){
			memcpy(msgs + 2,msgs+(MAX_MESSAGE_TOTAL/2 + 2),sizeof(message_t)*(MAX_MESSAGE_TOTAL/2 - 2));
			msgs_len = (MAX_MESSAGE_TOTAL/2 - 1 ); 
		}
		msgs_len++;
		printf("current message cont = %d \n",msgs_len);
		for(i = 0; i < cur_con; i++){
			deliver_msg(buffers[i],(msgs + msgs_len - 1));
		}
	}
}

void event_cb(struct bufferevent* buffer,short event,void* arg){
	int i = 0;
	int n = 0;
	if(event && BEV_EVENT_EOF){
		printf("connection %d closed\n",bufferevent_getfd(buffer));	
	}else if(event && BEV_EVENT_ERROR){
		print_err("got error on thin connection");	
	}
	if(event && (BEV_EVENT_EOF | BEV_EVENT_ERROR)){
		for(i = 0; i < cur_con;i++){
			if(buffers[i] == buffer){
				for(n = i ; n < cur_con - 1; n++){
					buffers[n] = buffers[n+1];	
				}	
				break;	
			}	
		}
		cur_con --;
		bufferevent_setcb(buffer,NULL,NULL,NULL,NULL);
		bufferevent_free(buffer);	
	}
}

void deliver_msg(struct bufferevent* buffer,message_t* msg){
	bufferevent_write(buffer,msg,sizeof(message_t));
	//printf("from clien name : %s , msg : %s \n",msg->name,msg->msg);
}

void evlistener_cb(struct evconnlistener* evlistener, evutil_socket_t connect_fd,
		struct sockaddr* cli, int cli_len, void *arg)
{
	int i = 0;
	struct bufferevent* buffer ;
	buffer = bufferevent_socket_new(base,connect_fd,BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	
	bufferevent_enable(buffer,EV_READ | EV_WRITE);
	message_t* msgs = arg ;
	for(i = 0; i < msgs_len; i++){
		deliver_msg(buffer,msgs);
		msgs ++;
	}
	
	
	bufferevent_setcb(buffer,read_cb,NULL,event_cb,arg);

	buffers[cur_con] = buffer;
	cur_con++;
}

void evlistener_error_cb(struct evconnlistener* listener,void *arg){
	print_err("listen errir ,exit");
	event_base_loopbreak(base);
}

void set_init_msgs(message_t* msgs){
	char buf[1024];
	sprintf(buf,"root");
	memcpy(msgs->name,buf,strlen(buf)+1);
	sprintf(buf,"This is my space");
	memcpy(msgs->msg,buf,strlen(buf)+1);
	msgs ++ ;
	sprintf(buf,"master");
	memcpy(msgs->name,buf,strlen(buf)+1);
	sprintf(buf,"yes this is root space");
	memcpy(msgs->msg,buf,strlen(buf)+1);
}

int main(){
	int listen_fd;
	int connect_fd;
	struct sockaddr_in srv;
	struct sockaddr_in cli;

	struct evconnlistener* evlistener;

	message_t* msgs;
	msgs = (message_t*)malloc(sizeof(message_t)*MAX_MESSAGE_TOTAL);
	if(!msgs){
		print_err("malloc message error");	
	}
	//memset(msgs,0,(sizeof(message_t)*MAX_MESSAGE_TOTAL));
	set_init_msgs(msgs);//add two message for test.

	base = event_base_new();

	memset(&srv,0,sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_port = htons(10001);
	srv.sin_addr.s_addr = htonl(INADDR_ANY);

	evlistener = evconnlistener_new_bind(base,evlistener_cb,msgs,LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,16,(struct sockaddr*)&srv,sizeof(srv));
	if(!evlistener){
		print_err("evlistener error");	
	}

	evconnlistener_set_error_cb(evlistener,evlistener_error_cb);

	event_base_dispatch(base);

	return 0;
}
