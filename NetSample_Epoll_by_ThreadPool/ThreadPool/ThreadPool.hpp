#include "taskQueue.hpp"
#include "pthread.h"


class ThreadPool
{
public:
    //??????
    ThreadPool(int min,int max);

    //????????
    ~ThreadPool();

    //????????
    void addTask(Task a);

    //???????????
    inline int getBusyNumber()
    {
        return this->busy_Num;
    }

    //??????????????
    inline int getLiveNumber()
    {
        return this->live_Num;
    }

private:
    //??????????
    static void* worker(void *arg);

    //????????????
    static void* manager(void *arg);

    //??????????
    void threadExit();

private:
    taskQ *task;
    int m_minNum;
    int m_maxNum;
    int live_Num;
    int busy_Num;
    int exit_Num;
    pthread_mutex_t mutexPool;
    pthread_cond_t notEmpty;
    pthread_t ManagerId;
    pthread_t* threadIds;
    bool shutdown=false;
    static const int NUMBER=2;
};