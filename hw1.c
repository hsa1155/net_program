#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 200000

struct
{
    char *ext;
    char *filetype;
} extensions[] = {
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"zip", "image/zip"},
    {"gz", "image/gz"},
    {"tar", "image/tar"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"exe", "text/plain"},
    {"c","text/x-csrc"},
    {0, 0}};

//count content lenth
int get_len(char *pack) {
    char *line = strstr(pack, "th: ");
    if(line==NULL) printf("not found\n");
    //else printf("%s\n",line);
    char n[10];
    int idx = 0;
    while(*(++line)!= '\0') {
    if(isdigit(*line)) n[idx++] = *line;
    }
    n[idx]= 0;
    //printf("done\n");
    return atoi(n);
}


void handle_socket(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char *fstr;
    static char buffer[BUFSIZE + 1];
    int context_len;

    ret = read(fd, buffer, BUFSIZE); /* 讀取瀏覽器要求 */
    //printf("line40 ret=%ld\n",ret);
    //printf("%s",buffer);
    if (ret == 0 || ret == -1)
    {
        /* 網路連線有問題，所以結束行程 */
        exit(3);
    }
    
    /* 程式技巧：在讀取到的字串結尾補空字元，方便後續程式判斷結尾 */
    if (ret > 0 && ret < BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;
    //context_len=get_len(buffer);
    //printf("%d",context_len);
    /* 移除換行字元 */
    for (i = 0; i < ret; i++)
        if (buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i] = 0;

    

    /* 只接受 GET or post 命令要求 */ //
    if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4))
       { 
           if (strncmp(buffer, "POST ", 5) && strncmp(buffer, "POST ", 5))
           exit(3);
           else
           {
                printf("doing post\n");
                //printf("%s\n",buffer+64);

                //count size of file
                context_len=get_len(buffer+64);
                printf("content len= %d\n",context_len);
                
                sprintf(buffer, "HTTP/1.0 200 OK\r\n\r\n");
                write(fd, buffer, strlen(buffer));
                //printf("write back %s\n",buffer);

                //read upload file
                ret=read(fd, buffer, BUFSIZE);
                if(ret<=0) exit(1);
                if (ret > 0 && ret < BUFSIZE)
                    buffer[ret] = 0;
                else
                    buffer[0] = 0;
                
                //printf("upload ret=%ld\n",ret);
                int counter=0;
                int flags=0;
                char tmp[1024];
                char upload_file_name[2000];
                int tmplen=0;

                //cut file name
                while(1)
                {
                    if(buffer[counter]=='"') flags++;
                    //printf("!");
                    if(flags==3)
                    {
                        counter++;
                        while(1)
                        {
                            
                            if(buffer[counter]!='"')
                            {
                                //printf("!!%c\n",buffer[counter]);
                                tmp[tmplen]=buffer[counter];
                                counter++;
                                tmplen++;

                            }
                            else
                            {
                                flags++;
                                break;
                            }
                        }
                    }
                    if(flags==4) break;


                    counter++;
                }
                tmp[tmplen]='\0';
                //printf("tmp=%s\n",tmp);

                //create file name
                sprintf(upload_file_name,"upload-%s",tmp);
                upload_file_name[strlen(upload_file_name)]='\0';
                //printf("%s",upload_file_name);

                //open file and type check
                FILE *upFile;
                int ispic=0;
                if((strstr(upload_file_name,".png")!=NULL)||(strstr(upload_file_name,".jpg")!=NULL)||(strstr(upload_file_name,".jpeg")!=NULL)||(strstr(upload_file_name,".bmp")!=NULL))
                {upFile=fopen(upload_file_name, "wb");ispic=1;}
                else
                upFile=fopen(upload_file_name, "w");

                //printf("%d",upload_fd);
                if(upFile==NULL) printf("open file error\n");
                //printf("%s",buffer);

                //find the start of file
                char* pos;
                pos=strstr(buffer,"\r\n\r\n");
                pos+=4;//this is start of true file
                //printf("%ld\n",strlen(pos));
                
                int writein;
                if(ispic==1) writein=fwrite(pos,sizeof(char)*context_len,1,upFile);
                else writein=fwrite(pos,sizeof(char),strlen(pos)-44,upFile);
                //printf("write %d\n",writein);
                //write back main page
                int write_back_fd = open("index.html", O_RDONLY) ;
                while ((ret = read(write_back_fd, buffer, BUFSIZE)) > 0)
                {
                    write(fd, buffer, ret);
                }
                
                exit(1);
           }
       }
    /* 把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
    for (i = 4; i < BUFSIZE; i++)
    {
        if (buffer[i] == ' ')
        {
            buffer[i] = 0;
            //printf("i=%ld\n",i);
            break;
        }
    }
    

    /* 檔掉回上層目錄的路徑『..』 */
    for (j = 0; j < i - 1; j++)
        if (buffer[j] == '.' && buffer[j + 1] == '.')
            exit(3);

    /* 當客戶端要求根目錄時讀取 index.html */
    if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
        strcpy(buffer, "GET /index.html\0");

    /* 檢查客戶端所要求的檔案格式 */
    buflen = strlen(buffer);
    //printf("buflen=%d\n",buflen);
    //printf("buffer=%s\n",buffer);
    fstr = (char *)0;

    for (i = 0; extensions[i].ext != 0; i++)
    {
        len = strlen(extensions[i].ext);
        if (!strncmp(&buffer[buflen - len], extensions[i].ext, len))
        {
            fstr = extensions[i].filetype;
            break;
        }
    }

    /* 檔案格式不支援 */
    if (fstr == 0)
    {
        fstr = extensions[i - 1].filetype;
    }

    printf("Opening file %s\n", &buffer[5]);

    /* 開啟檔案 */
    if ((file_fd = open(&buffer[5], O_RDONLY)) == -1) {
        //printf("Failed to open file: %s\n", strerror(errno));
    }

    /* 傳回瀏覽器成功碼 200 和內容的格式 */
    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    write(fd, buffer, strlen(buffer));

    /* 讀取檔案內容輸出到客戶端瀏覽器 */
    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
    {
        write(fd, buffer, ret);
    }

    exit(1);
}

main(int argc, char **argv)
{
    int i, pid, listenfd, socketfd;
    size_t length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;

   
    /* 讓父行程不必等待子行程結束 */
    signal(SIGCLD, SIG_IGN);

    /* 開啟網路 Socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("ERROR: cannot open socket");
        exit(3);
    }

    int reuseAddrOpt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(reuseAddrOpt));

    /* 網路連線設定 */
    serv_addr.sin_family = AF_INET;
    /* 使用任何在本機的對外 IP */
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* assign Port */
    serv_addr.sin_port = htons(5269);

    /* 開啟網路監聽器 */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR: cannot bind");
        exit(3);
    }

    /* 開始監聽網路 */
    if (listen(listenfd, 64) < 0) {
        printf("ERROR: cannot listen");
        exit(3);
    }

    //pid = getpid();
    //printf("pid:%d\n",pid);


    while (1)
    {
        length = sizeof(cli_addr);
        /* 等待客戶端連線 */
        if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
            exit(3);

        /* 分出子行程處理要求 */
        if ((pid = fork()) < 0)
        {
            exit(3);
        }
        else
        {
            if (pid == 0)
            { /* 子行程 */
                //printf("pid=%d",pid=getpid());
                close(listenfd);
                handle_socket(socketfd);
            }
            else
            { /* 父行程 */
                close(socketfd);
            }
        }
    }
}