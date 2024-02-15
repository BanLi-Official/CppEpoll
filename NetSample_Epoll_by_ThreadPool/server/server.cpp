#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include "ThreadPool.hpp"
#include <poll.h>
#include <sys/epoll.h>

using namespace std;

pthread_mutex_t mymutex;

typedef struct fdInfo
{
    int fd;
    int *maxfd;
    socklen_t *cliaddr;
    struct epoll_event *events;
    int epfd;
} fdInfo;

typedef struct CommunicateInfo
{
    int fd;
    int *maxfd;
    struct epoll_event *events;
    int epfd;
} CommunicateInfo;

void ConnAccept(void *arg)
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
    int res=epoll_ctl(info->epfd,EPOLL_CTL_ADD,cfd,&event);
    if(res==-1)
    {
        cout<<"添加客户端标识符失败"<<endl;
        return;
    }

    *info->maxfd = cfd > *info->maxfd ? cfd : *info->maxfd;
    pthread_mutex_unlock(&mymutex);
    cout << "设置成功！ " << endl;
    free(info);

    return;
}

void Communication(void *arg)
{
    cout << "開始通信" << endl;
    CommunicateInfo *info = (CommunicateInfo *)arg;
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
            int res=epoll_ctl(info->epfd,EPOLL_CTL_DEL,info->fd,NULL);
            if (res==-1)
            {
                cout << "删除标识符失败 " << endl;
            }
            
            pthread_mutex_unlock(&mymutex);
            free(info);
            close(info->fd);
            return;
        }
        else if (len > 0) // 收到了数据
        {
            // 发送数据
            write(info->fd, buf, strlen(buf) + 1);
            cout << "写了一次" << endl;
            // return ;
        }
        else
        {
            // 异常
            perror("read");
            // 将该文件描述符从集合中删除
            pthread_mutex_lock(&mymutex);
            int res=epoll_ctl(info->epfd,EPOLL_CTL_DEL,info->fd,NULL);
            if (res==-1)
            {
                cout << "删除标识符失败 " << endl;
            }
            pthread_mutex_unlock(&mymutex);
            free(info);
            return;
        }
        len = read(info->fd, buf, sizeof(buf));
        
    }
    free(info);
    return;
}

int main() // 基于多路复用select函数实现的并行服务器
{

    ThreadPool pool(1, 8);
    cout << "线程池初始化完毕............................." << endl;
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

    // 将监听的fd的状态交给内核检测
    int maxfd = lfd;

    // 初始化Epoll实例
    int epfd = epoll_create(1);

    // 将lfd放入epoll中
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = lfd;

    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &event);
    if (res == -1)
    {
        cout << "监听文件标识符放入失败" << endl;
    }

    // 创建一个数组用于接收epoll解除阻塞之后传出的事件,多余的未传出的事件由下一轮进行传出
    struct epoll_event events[100];
    int len = sizeof(events) / sizeof(struct epoll_event);

    while (1)
    {
        //sleep(1);
        // 差开始通讯的循环

        cout << "開始等待" << endl;

        int num = epoll_wait(epfd, events, len, -1);
        // cout<<"               rdtemp="<<rdtemp<<endl;

        cout << "epll等待結束 num=" <<num<< endl;

        sleep(2);

        // 判断连接请求还在不在里面，如果在，则运行accept
        for (int i = 0; i < num; i++)
        {
            if (events[i].data.fd == lfd && events[i].events == EPOLLIN)
            {
                cout << "开始分配连接请求处理进程" << endl;
                // 添加一个任务用于连接客户端
                pthread_t conn;
                fdInfo *info = (fdInfo *)malloc(sizeof(fdInfo));
                info->fd = lfd;
                info->maxfd = &maxfd;
                info->events=events;
                info->epfd=epfd;

                Task conTask;
                conTask.function = ConnAccept;
                conTask.arg = info;
                pool.addTask(conTask);
            }
            else
            {
                // myfd[i].revents=NULL;

                cout << "开始分配工作线程,线程位置为:" << i << endl;
                // 添加一个任务用于通信
                pthread_t Communicate;
                CommunicateInfo *info = (CommunicateInfo *)malloc(sizeof(CommunicateInfo));
                info->fd = events[i].data.fd;
                info->maxfd = &maxfd;
                info->events=events;
                info->epfd=epfd;

                Task CommuTask;
                CommuTask.function = Communication;
                CommuTask.arg = info;

                pool.addTask(CommuTask);
                epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,NULL);
            }
        }

    }

    pthread_mutex_destroy(&mymutex);
    return 0;
}