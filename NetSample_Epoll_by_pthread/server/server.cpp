#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace std;

pthread_mutex_t mymutex;

typedef struct fdInfo
{
    int fd;
    struct sockaddr_in cliaddr;
    int epdf;
} fdInfo;

typedef struct CommunicateInfo
{
    int fd;
    int epdf;
} CommunicateInfo;

void *ConnAccept(void *arg)
{
    fdInfo *info = (fdInfo *)arg;

    int cliaddrLen = sizeof(info->cliaddr);
    int cfd = accept(info->fd, (struct sockaddr *)&info->cliaddr, (socklen_t *)&cliaddrLen);
    cout << "連接成功！ " << endl;
    
    // 得到了有效的客户端文件描述符，将这个文件描述符放入读集合当中，并更新最大值
    pthread_mutex_lock(&mymutex);
    struct epoll_event event;
    event.data.fd=cfd;
    event.events=EPOLLIN;
    int res=epoll_ctl(info->epdf,EPOLL_CTL_ADD,cfd,&event);
    pthread_mutex_unlock(&mymutex);
    cout << "设置成功！ " << endl;
    
    free(info);

    return NULL;
}

void *Communication(void *arg)
{

    cout << "開始通信" << endl;
    CommunicateInfo *info = (CommunicateInfo *)arg;
    int tmpfd=info->fd;

                
    // 接收数据，一次接收10个字节，客户端每次发送100个字节,下一轮select检测的时候, 内核还会标记这个文件描述符缓冲区有数据 -> 再读一次
    // 循环会一直持续, 知道缓冲区数据被读完位置
    char buf[10] = {0};
    int len = read(info->fd, buf, sizeof(buf));
    
    while (len)
    {
        if (len == 0) // 客户端关闭了连接，，因为如果正好读完，会在select过程中删除
        {
            printf("客户端关闭了连接.....\n");
            // 将该文件描述符从集合中删除
            pthread_mutex_lock(&mymutex);
            int res=epoll_ctl(info->epdf,EPOLL_CTL_DEL,info->fd,NULL);
            if(res==-1)
            {
                cout<<"删除失败！"<<endl;
            }
            pthread_mutex_unlock(&mymutex);
            close(tmpfd);
            free(info);

            return NULL;
        }
        else if (len > 0) // 收到了数据
        {
            // 发送数据
            write(info->fd, buf, strlen(buf) + 1);
            cout << "写了一次" << endl;
            //return NULL;
        }
        else
        {
            // 异常
            perror("read");
            pthread_mutex_lock(&mymutex);
            int res=epoll_ctl(info->epdf,EPOLL_CTL_DEL,info->fd,NULL);
            pthread_mutex_unlock(&mymutex);
            close(tmpfd);
            free(info);
            return NULL;
        }
        len = read(info->fd, buf, sizeof(buf));
        if (len == 0) // 客户端关闭了连接，，因为如果正好读完，会在select过程中删除
        {
            printf("客户端关闭了连接.....\n");
            // 将该文件描述符从集合中删除
            pthread_mutex_lock(&mymutex);
            int res=epoll_ctl(info->epdf,EPOLL_CTL_DEL,info->fd,NULL);
            if(res==-1)
            {
                cout<<"删除失败！"<<endl;
            }
            close(tmpfd);
            pthread_mutex_unlock(&mymutex);
            free(info);
            close(tmpfd);
            return NULL;
        }
    }
    free(info);
    return NULL;
}

int main() // 基于多路复用select函数实现的并行服务器
{
    pthread_mutex_init(&mymutex, NULL);
    // 1 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2 绑定
    struct sockaddr_in addr; // struct sockaddr_in是用于表示IPv4地址的结构体，它是基于struct sockaddr的扩展。
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9997);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));

    // 3 设置监听
    listen(lfd, 128);

    // 创建Epoll实例
    int epdf=epoll_create(1);

    //将lfd加入这个实例中，让其检查
    struct epoll_event event;
    event.events=POLLIN;
    event.data.fd=lfd;
    int res=epoll_ctl(epdf,EPOLL_CTL_ADD,lfd,&event);

    struct epoll_event events[100];
    int len=sizeof(event)/sizeof(struct epoll_event);


    while (1)
    {
        sleep(2);

        cout << "開始等待" << endl;
        int num=epoll_wait(epdf,events,len,-1);
        cout << "poll等待結束" << endl;

        // 判断连接请求还在不在里面，如果在，则运行accept
        for(int i=0;i<len;i++)
        {
            if(events[i].data.fd==lfd)
            {
                 // 添加一个子线程用于连接客户端
                pthread_t conn;
                //fdInfo *info = (fdInfo *)malloc(sizeof(fdInfo));
                fdInfo* info = new fdInfo;  // 创建一个非 const 的指针对象
                info->fd = events[i].data.fd;
                info->epdf=epdf;
                pthread_create(&conn, NULL, ConnAccept, info);
                pthread_detach(conn);
            }
            else
            {
                if (events[i].data.fd != lfd  )
                {
                
                    // 添加一个子线程用于通信
                    cout<<"創建一個通訊"<<endl;
                    pthread_t Communicate;
                    //fdInfo *info = (fdInfo *)malloc(sizeof(fdInfo));
                    CommunicateInfo* info = new CommunicateInfo;  // 创建一个非 const 的指针对象
                    info->fd = events[i].data.fd;
                    info->epdf=epdf;
                    pthread_create(&Communicate, NULL, Communication, info);
                    pthread_detach(Communicate);
                }
            }
        }

        
    }

    pthread_mutex_destroy(&mymutex);
    return 0;
}