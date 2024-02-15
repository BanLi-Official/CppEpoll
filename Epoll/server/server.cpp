// server.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>

using namespace std;

int main()
{
    // 1. 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 2. 将socket()返回值和本地的IP端口绑定到一起
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9996);   // 大端端口
    // INADDR_ANY代表本机的所有IP, 假设有三个网卡就有三个IP地址
    // 这个宏可以代表任意一个IP地址
    // 这个宏一般用于本地的绑定操作
    addr.sin_addr.s_addr = INADDR_ANY;  // 这个宏的值为0 == 0.0.0.0
//    inet_pton(AF_INET, "192.168.8.161", &addr.sin_addr.s_addr);
    int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        exit(0);
    }

    // 3. 设置监听
    ret = listen(lfd, 128);
    if(ret == -1)
    {
        perror("listen");
        exit(0);
    }

    int epfd=epoll_create(1);
    struct epoll_event even;
    even.events=EPOLLIN;
    even.data.fd=lfd;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&even);

    struct epoll_event evens[100];//用于接取传出的内容
    int len=sizeof(evens)/sizeof(struct epoll_event);

    while (1)
    {
        cout<<"                     開始等待！！！"<<endl;
        int num=epoll_wait(epfd,evens,len,-1);
        cout<<"                     等待結束！！！"<<"   num="<<num<<endl;
        for(int i=0;i<num;i++)//取出所有的检测到的事件
        {

            int curfd = evens[i].data.fd;
            if(evens[i].data.fd==lfd)
            {
                struct sockaddr_in *add;
                int len=sizeof(struct sockaddr_in);
                int cfd=accept(evens[i].data.fd,NULL,NULL);
                struct epoll_event even;
                even.events=EPOLLIN;
                even.data.fd=cfd;
                //将接收到的cfd放入epoll检测的红黑树当中
                ret=epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&even);
                if(ret==-1)
                {
                    cout<<"登錄失敗"<<endl;
                }
                else
                {
                    cout<<"登陸成功，已加入紅黑樹"<<endl;
                }
                
            }
            else
            {
                // 接收数据
                char buf[10];
                memset(buf, 0, sizeof(buf));
                cout<<"正在讀！！！！"<<endl;
                int len = read(evens[i].data.fd, buf, sizeof(buf));
                if(len > 0)
                {
                    // 发送数据
                    if(len<=2)
                    {
                        cout<<"          out!!"<<endl;
                        break;
                    }
                    printf("客户端say: %s\n", buf);
                    write(evens[i].data.fd, buf, len);
                    sleep(0.1);
                }
                else if(len  == 0)
                {
                    printf("客户端断开了连接...\n");
                    ret=epoll_ctl(epfd,EPOLL_CTL_DEL,evens[i].data.fd,NULL);        
                    close(curfd);
                    //break;
                }
                else
                {
                    perror("read");
                    ret=epoll_ctl(epfd,EPOLL_CTL_DEL,evens[i].data.fd,NULL);       
                    close(curfd); 
                    //break;
                }
            }

            

        }
    }
    



    return 0;
}
