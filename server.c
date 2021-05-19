#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <uv.h>
#include "hashdb.c"
#include "hashdb.h"
#include <netinet/in.h>
typedef struct _DB DB;
//DB* dbh;
//dbh = ht_open("t", 1024);
int n = 0;
char buf1[4096];
int ht_set(DB* dbh, const char* key, const char* value);
int ht_get(DB* dbh, const char* key, char** value);
int ht_del(DB* dbh, const char* key);
typedef int (*cmd_handler)(uv_buf_t* buf);
DB* ht_open(const char* filename, size_t initial_capacity);
int ht_set(DB* dbh, const char* key, const char* value);
int ht_get(DB* dbh, const char* key, char** value);
int ht_del(DB* dbh, const char* key);
int ht_get(DB* dbh, const char* key, char** value);
typedef struct _my_client_t {
	uv_tcp_t super;
	char* buf;

} my_client_t;

//DB* db;
//const char* t;
//db = ht_open(t, 1024);

uv_loop_t* loop;

int read_st(uv_buf_t* buf, int offset, char* key) {
	if ( sscanf(buf->base+offset, "%s", key) == 1) {
		fflush(stdin);
		//buf->len = snprintf(buf->base, buf->len, "%s\n", key);
		return 1;
	}
}

int read_str(uv_buf_t* buf, int offint, char* key, char* val) { 
	//char buf2[4096];
	//struct sockaddr_in client;
	//int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	//socklen_t client_len = sizeof(client);
	//int client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
	//int read = recv(client_fd, buf2, 4096, 0);
	printf("%d\n", sscanf(buf->base+offint, "%s\n", key));	
	printf("%d\n", sscanf(buf->base+offint+strlen(key)+1, "%s\n", val));
	//printf("%s\n%s\n", key, val);
	//char* s = buf2;
	//key = strsep(&s, " \t\n");
	//val = strsep(&s, " \n");
	fflush(stdin);
	return 1;
}

int cmd_get(uv_buf_t* buf) {
	DB* dbh;
	dbh = ht_open("t", 1024);	
	char* key;	
	if ( read_st(buf, 4, key) ) {	
		char* val;	
		if ( ht_get(dbh, key, &val) ) {	
			buf->len = snprintf(buf->base, buf->len, "%s\n", val);
			fflush(stdout);
			free(val);	
		} else {	
			buf->len = snprintf(buf->base, buf->len, "No such key\n");
			fflush(stdout);
		}	
	}
	return 0;
}
int cmd_set(uv_buf_t* buf) {
	 DB* dbh;
 	 //char s[80] = "******************";		 
	 dbh = ht_open("t", 1024);		
 	 char* key;
	 char* val;	 
	 if (read_str(buf, 4, key, val) == 1) {
		ht_set(dbh, key, val);
		buf->len = snprintf(buf->base, buf->len,"Setting '%s' = '%s'\nOK \n", key, val); //"'%s'\n OK \n", val
	 	fflush(stdout);
	 } else {
		buf->len = snprintf(buf->base, buf->len, "Error. Try again.\n");
		fflush(stdout); 
	}
	return 0;
}
int cmd_del(uv_buf_t* buf) {
	DB* dbh;
	dbh = ht_open("t", 1024);
	char* key;
	if ( read_st(buf, 4, key) ) {
		if (ht_del(dbh, key)) {
			buf->len = snprintf(buf->base, buf->len, "OK\n");
			fflush(stdout);
		} else {	
			buf->len = snprintf(buf->base, buf->len, "No such key\n");
			fflush(stdout);
		}	
	}
	return 0;
}
int cmd_num(uv_buf_t* buf) {
	DB* dbh;
	dbh = ht_open("t", 1024);
	Stat stat;	
	ht_get_stat(dbh, &stat);		
	buf->len = snprintf(buf->base, buf->len, "Number of keys: %lu\n", stat.keys);
	return 0;
}

int cmd_null(uv_buf_t* buf) { buf->len = snprintf(buf->base, buf->len, "Wrong. Try again.\n"); return 0;}

struct _command {
	char name[20];
	cmd_handler handler;
};

struct _command COMMANDS[] = {
	{"SET", cmd_set},//добавляет ключ
	{"GET", cmd_get},//находит ключ
	{"DEL", cmd_del},//удаляет ключ
	{"NUM", cmd_num},//выводит кол-во ключей
	{"", cmd_null}//выводит ошибку
};

int process_userinput(ssize_t nread, uv_buf_t *buf) {
	struct _command* cmd;
	for(cmd = COMMANDS; cmd->handler; cmd++ ) {
		fprintf(stderr, "Comparing handler %s\n", cmd->name);
		fflush(stdout);
		if ( !strncmp(buf->base, cmd->name, strlen(cmd->name)) ) {
			fprintf(stderr, "Found\n");
			fflush(stdout);
			return cmd->handler(buf);
		}
	}
	buf->len = snprintf(buf->base, buf->len, "UNKNOWN COMMAND\n");
	fflush(stdout);
	return 0;
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	my_client_t* client = (my_client_t*)handle;
	buf->base = client -> buf;
	buf->base = (char*)realloc(buf->base, suggested_size);
	buf->len = suggested_size;
	//my_client_t* client = (my_client_t*)handle;
	client->buf = buf->base;
}

void on_conn_close(uv_handle_t* client1) {
	/*void* buf = uv_handle_get_data(client);*/
	my_client_t* client = (my_client_t*)client1;
	void* buf = client;
	if( buf ) {
		fprintf(stderr, "Freeing buf\n");
		fflush(stdout);
		free(buf);
	}
	fprintf(stderr, "Freeing client\n");
	fflush(stdout);
	free(client);
}

void on_conn_write(uv_write_t* req, int status) {
	if ( status ) {
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
		fflush(stdout);
	}
	free(req);
}

void on_client_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
		if (nread != UV_EOF) {
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
			fflush(stdout);
			uv_close((uv_handle_t*)client, on_conn_close);
		}
	} else if (nread > 0) {
		int res;
		uv_buf_t wrbuf = uv_buf_init(buf->base, buf->len);
		fprintf(stderr, "Read cb with nread=%ld, buf size: %lu\n", nread, buf->len);
		fflush(stdout);
		res = process_userinput(nread, &wrbuf);
		uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
		uv_write(req, client, &wrbuf, 1, on_conn_write);
		if ( res ) {
			uv_close((uv_handle_t*)client, on_conn_close);
		}
	}
}

void on_new_connection(uv_stream_t *server, int status) {
	if (status < 0) {
		fprintf(stderr, "New connection error %s\n", uv_strerror(status));
		return;
	}
	
	my_client_t* client = (my_client_t*)malloc(sizeof(my_client_t));
	bzero(client, sizeof(my_client_t));
	uv_tcp_init(loop, (uv_tcp_t*)client);

	if (uv_accept(server, (uv_stream_t*) client) == 0) {
		fprintf(stderr, "Client %p connected\n", client);
		n += 1;
		printf("%d", n);
		fflush(stdout);
		uv_read_start((uv_stream_t*)client, alloc_buffer, on_client_read);
	} else {
		fprintf(stderr, "Error while accepting\n");
		fflush(stdout);
		uv_close((uv_handle_t*) client, on_conn_close);
	}
	fflush(stdin);
}

int main() {
	DB* dbh;
 	//char buf1[4096];        	 
	dbh = ht_open("t", 1024);
	int result;
	struct sockaddr_in addr;
	loop = uv_default_loop();
	uv_tcp_t server;
	uv_tcp_init(loop, &server);
	uv_ip4_addr("0.0.0.0", 17737, &addr);
	uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
	int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
	if (r) {
		fprintf(stderr, "Listen error %s\n", uv_strerror(r));
		fflush(stdout);
		return 1;
	}
	result = uv_run(loop, UV_RUN_DEFAULT);
	fprintf(stderr, "Finalization...\n");
	return result;
}

