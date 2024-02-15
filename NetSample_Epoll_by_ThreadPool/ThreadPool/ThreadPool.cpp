#include "ThreadPool.hpp"
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <pthread.h>



using namespace std;
extern pthread_mutex_t IOMutex;

ThreadPool::ThreadPool(int min, int max)
{


    //????????????
    task=new taskQ;
    do
    {
        //?????????
        this->m_minNum=min;
        this->m_maxNum=max;
        busy_Num=0;
        live_Num=min;

        //?????????????????????????????
        threadIds=new pthread_t[m_maxNum];
        if(threadIds==nullptr)
        {
            //cout<<"??????????"<<endl;
            break;
        }
        //?????
        memset(threadIds,0,sizeof(pthread_t)*m_maxNum);
        //???????????????????????????????????
        if(pthread_mutex_init(&this->mutexPool,NULL)!=0||
            pthread_cond_init(&this->notEmpty,NULL)!=0)
        {
            //cout<<"????????????????????????"<<endl;
            break;
        }

        //???????
        //??????��???????????????????
        int count=0;
        for(int i=0;i<m_minNum;i++)
        {
            pthread_create(&threadIds[i],NULL,worker,this);
            //pthread_mutex_lock(&IOMutex);
            //cout<<"??????????id="<<to_string(threadIds[i])<<endl;
            //pthread_mutex_unlock(&IOMutex);
            
        }

        //?????????????
        pthread_create(&ManagerId,NULL,manager,this);
    }while (0);
    


}

ThreadPool::~ThreadPool()
{
    shutdown = 1;
    // ????????????
    pthread_join(ManagerId, NULL);
    // ?????????????????
    for (int i = 0; i < live_Num; ++i)
    {
        pthread_cond_signal(&notEmpty);
    }

    if (task) delete task;
    if (threadIds) delete[]threadIds;
    pthread_mutex_destroy(&mutexPool);
    pthread_cond_destroy(&notEmpty);




}

void ThreadPool::addTask(Task a)
{

    if (shutdown)
    {
        return;
    }
    // ???????????????????????????????
    task->inputTask(a);
    ///cout<<"addOK~"<<endl;
    // ????????????
    pthread_cond_signal(&notEmpty);

    cout<<"释放！！！！！！！！！！"<<"           条件变量地址："<<&notEmpty<<endl;
}

void *ThreadPool::worker(void *arg)
{
    //cout<<"into worker"<<endl;
    ThreadPool * pool=static_cast<ThreadPool *>(arg);
    while (1)//??????????��????????????
    {
        //?????
        //sleep(10);
        //cout<<"enter worker 1st cycle"<<endl;
        pthread_mutex_lock(&pool->mutexPool);
        //cout<<"enter worker 1st cycle mutex lock"<<endl;
        cout<<"                     getTaskNum="<<pool->task->getTaskNum()<<endl;
        while(pool->task->getTaskNum()==0&&!pool->shutdown)
        {
            


            //pthread_mutex_lock(&IOMutex);
            //cout << "thread " << to_string(pthread_self()) << " waiting..." << endl;
            ///pthread_mutex_unlock(&IOMutex);
            //???????
            //cout<<"等着.........."<<endl;
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);
            //cout<<"干活了............"<<endl;
            //???????????��????????????
            if(pool->exit_Num>0)
            {
                pool->exit_Num--;
                if (pool->live_Num>pool->m_minNum)
                {
                    pool->live_Num--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    pool->threadExit();
                }
                
            }
            
        }
        //cout<<"出来了"<<endl;
        //?��????????????????
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pool->threadExit();
        }

        //?????????????????????
        Task task= pool->task->getTask();
        pool->busy_Num++;

        pthread_mutex_unlock(&pool->mutexPool);
        //pthread_mutex_lock(&IOMutex);
        //cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
        //pthread_mutex_unlock(&IOMutex);
        //cout<<"function start"<<endl;
        task.function(task.arg);
        //cout<<"function end"<<endl;
        //delete task.arg;
        task.arg=nullptr;

        //pthread_mutex_lock(&IOMutex);
        //cout << "thread " << to_string(pthread_self()) << " end working..." << endl;
        //pthread_mutex_unlock(&IOMutex);


        //??????????
        pthread_mutex_lock(&pool->mutexPool);
        pool->busy_Num--;
        pthread_mutex_unlock(&pool->mutexPool);


    }
    

    return nullptr;
}

void *ThreadPool::manager(void *arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    // ?????????��??, ???????
    while (!pool->shutdown)
    {
        //cout<<"manager is alive"<<endl;
        // ???5s??????
        //sleep(5);
        // ????????��????????????????
        //  ?????????????????
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->task->getTaskNum();
        int liveNum = pool->live_Num;
        int busyNum = pool->busy_Num;
        pthread_mutex_unlock(&pool->mutexPool);

        // ???????
        const int NUMBER = 1;
        // ??????????>????????? && ?????????<?????????
        /*
        cout<<"              queuesize="<<queueSize<<"      liveNum="<<liveNum<<"          busyNum="<<busyNum<<"       pool->m_maxNum="<<pool->m_maxNum<<endl;
        
        if (pthread_mutex_trylock(&pool->mutexPool) == 0) {
            printf("Mutex is not locked\n");
            pthread_mutex_unlock(&pool->mutexPool);
        } else {
            printf("Mutex is locked\n");
        }
        */
        pthread_cond_signal(&pool->notEmpty);
        

        if (queueSize > liveNum-busyNum && liveNum < pool->m_maxNum)
        {
            // ???????
            pthread_mutex_lock(&pool->mutexPool);
            int num = 0;
            for (int i = 0; i < pool->m_maxNum && num < NUMBER
                && pool->live_Num < pool->m_maxNum; ++i)
            {
                if (pool->threadIds[i] == 0)
                {
                    pthread_create(&pool->threadIds[i], NULL, worker, pool);
                    //pthread_mutex_lock(&IOMutex);
                    //cout<<"??????????id="<<to_string(pool->threadIds[i])<<endl;
                    //pthread_mutex_unlock(&IOMutex);
                    cout<<"create a thread!!!"<<endl;
                    num++;
                    pool->live_Num++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        // ???????????
        // ????*2 < ?????????? && ????????? > ??��???????
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exit_Num = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            for (int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }


    return nullptr;
}

void ThreadPool::threadExit()
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < m_maxNum; ++i)
    {
        if (threadIds[i] == tid)
        {
            //pthread_mutex_lock(&IOMutex);
            //cout << "threadExit() function: thread " 
                //<< to_string(pthread_self()) << " exiting..." << endl;
            //pthread_mutex_unlock(&IOMutex);
            threadIds[i]= 0;
            break;
        }
    }
    pthread_exit(NULL);

}
