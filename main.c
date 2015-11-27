#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <sys/queue.h>

struct {//ставит соотв между расширением и content type
	char *ext;
	char *conttype;
} extensions[] = {
	{".txt", "text/html"},
	{".htm", "text/html"},
	{".html", "text/html"},
	{".jpg", "text/jpeg"},
	{".jpeg", "text/jpg"},
	{".png", "image/png"},
	{".ico", "image/ico"},
	{".css", "text/css"},
	{".js", "text/javascript"},
	{".php", "text/php"},
	{".xml", "text/xml"},
	{".pdf", "application/pdf"},
	{0, 0}	
};

//char content_type[255];

const int N = 5;

pthread_t ntid[5];
pthread_t servtid;
pthread_mutex_t lock[5];
int cd[5];

struct qnode {
        int value;
        TAILQ_ENTRY(qnode) entries;
};

TAILQ_HEAD(, qnode) qhead;

void headers(int client, int size, int httpcode, char* content_type) {//заголовки ответа сервера
	char buf[1024];
	char strsize[20];
	sprintf(strsize, "%d", size);
	if (httpcode == 200) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	}
	else if (httpcode == 404) {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
	}
	else {
		strcpy(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	}
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Connection: keep-alive\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Content-length: ");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, strsize);
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "simple-server");
	send(client, buf, strlen(buf), 0);
	if (content_type != NULL) {
		sprintf(buf, "Content-Type: %s\r\n", content_type);
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0); 
	}
}

void parsingFileName(char *line, char **filepath, size_t *len) {//из строки запроса вычленяет название файла который нужно открыть
	char *start = NULL;
	while ((*line) != '/') line++;
	start = line + 1;
	while ((*line) != ' ') line++;
	(*len) = line - start;
	*filepath = (char*)malloc(*len + 1);
	*filepath = strncpy(*filepath, start, *len);
	(*filepath)[*len] = '\0';
	printf("%s \n", *filepath);
}

char* getFileExtension(char *filename) {//выдать расширение файла
	return strrchr(filename, '.');
}


int main() {//инициализируем очередь
	int ld = 0;
	int res = 0;
	int _cd = 0;
	const int backlog = 10;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	socklen_t size_saddr;
	socklen_t size_caddr;

	struct qnode *qitem;

	int i = 0;

	//init queue
	TAILQ_INIT(&qhead);

	// creating threads
	while (i < N) {
		pthread_mutex_init(&lock[i], NULL);
		pthread_mutex_lock(&lock[i]);
		createThread(i);
		i++;
	}

	int err = pthread_create(&servtid, NULL, serv, NULL);//создаём поток для метода serv
	if (err != 0) {
		printf("You can't to create a thread %s\n", strerror(err));
	}

	ld = socket(AF_INET, SOCK_STREAM, 0);//открываем слушателя дескриптора и конфигурируем сервер (до puts start)
	if (ld == -1) {
		printf("listener creation error \n");
	}
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8080);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(ld, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
		printf("bind error \n");
	}
	res = listen(ld, backlog);
	if (res == -1) {
		printf("listen error \n");
	}

	puts("Start");

	while (1) {//бесконечный цикл,получаем cd, и кладём его в очередь
		_cd = accept(ld, (struct sockaddr *)&caddr, &size_caddr);
		if (_cd == -1) {
			printf("accept denied error \n");
		}
		printf("client in %d descriptor. Client addr is %d \n", _cd, caddr.sin_addr.s_addr);

		qitem = malloc(sizeof(*qitem));
		qitem->value = _cd;
                TAILQ_INSERT_TAIL(&qhead, qitem, entries);

	}
	return 0;
}
