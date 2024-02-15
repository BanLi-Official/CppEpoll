#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;


int main()  //����ͨ�ŵĿͻ���
{
    // 1 ��������ͨ�ŵ��׽���
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        perror("socket");
        exit(0);
    }

    // 2 ���ӷ�����
    struct sockaddr_in addr;
    addr.sin_family=AF_INET; //ipv4
    addr.sin_port=htons(9996);// �����������Ķ˿�, �ֽ���Ӧ���������ֽ���
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr.s_addr);
    int ret=connect(fd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret==-1)
    {
        perror("connect");
        exit(0);
    }

    //ͨ��
    while (1)
    {
        //������
        char recvBuf[1024];
        //д����
        fgets(recvBuf,sizeof(recvBuf),stdin);
        write(fd,recvBuf,strlen(recvBuf)+1);

        int oriLen=strlen(recvBuf)-1;

        cout<<"strlen(recvBuf)="<<oriLen<<endl;
        
        int total_get=0;
        while (total_get<oriLen)
        {
            //cout<<"�_ʼ�x"<<endl;
            char recvBuf2[1024];
            read(fd,recvBuf2,sizeof(recvBuf2));
            total_get+=10;
            cout<<"total_get="<<total_get<<"          strlen(recvBuf)="<<oriLen<<endl;
            printf("recv buf: %s\n", recvBuf2);
            if (total_get>=oriLen)
            {
                cout<<"out"<<endl;
                break;
            }
            
            

        }
        
        
        sleep(1);
    }

    close(fd);

    return 0;
}

