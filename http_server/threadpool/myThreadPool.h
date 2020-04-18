#ifndef _MYTHREAD_POOL_H
#define _MYTHREAD_POOL_H

#include <string>
#include <queue>
#include <pthread.h>
/* Task Node */
class myTask 
{
protected:
    int connectFd;
public:
    myTask();
    virtual ~myTask();//虚析构
    int GetConnectFd();
    int SetConnectFd(int connFd);
    virtual void RunTask()=0;//纯虚函数，派生类继承的时候实现
};

/* Thread Pool Controller*/
class myThreadPool
{
private:    
    int totalThreadNums; //所有线程数，包括了执行线程和空闲线程        
    static std::queue<myTask*> taskList; //任务等待队列
    static bool shutdown; //线程池的状态：1.开启 2.关闭
    static void* ThreadRoutine(void *arg);
    void CreatePool();

    pthread_t *pthread_id; //线程指针
    static pthread_mutex_t thread_mutex; //线程互斥锁
    static pthread_cond_t  thread_cond; //线程条件变量
public:
    myThreadPool(int threadNums = 5);
    ~myThreadPool();
    void AddTaskList(myTask *task);
    void destroyPool();
    int GetTaskListSize();
};

#endif