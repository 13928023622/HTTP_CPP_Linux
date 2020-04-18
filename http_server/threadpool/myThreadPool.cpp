#include"myThreadPool.h"
#include<queue>
#include<pthread.h> 
#include<iostream>
#include<stdio.h> 
/* _MYTASK_ */

myTask::myTask()
{
    
}
myTask::~myTask()
{
    
}
int myTask::SetConnectFd(int connFd)
{
    this->connectFd = connFd;
    return this->connectFd;
}

int myTask::GetConnectFd()
{
    return this->connectFd;
}



/* _MY_THREAD_POOL_ */
/* 静态变量初始化 */
std::queue<myTask*> myThreadPool::taskList;
bool myThreadPool::shutdown = false;
pthread_cond_t myThreadPool::thread_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t myThreadPool::thread_mutex = PTHREAD_MUTEX_INITIALIZER;

myThreadPool::myThreadPool(int threadNums)
{
    /* Init ThreadPool Attr */
    this->totalThreadNums = threadNums;
    
    /*
        param1: 锁
        param2: 锁属性，NULL默认是互斥锁
    */
    pthread_mutex_init(&thread_mutex, NULL);
    pthread_cond_init(&thread_cond, NULL);
    
    CreatePool();
    std::cout << "[INFO] ThreadPool has "<< this->totalThreadNums << " thread!"<<std::endl;
}

myThreadPool::~myThreadPool()
{   
    destroyPool();
}

void myThreadPool::CreatePool()
{    
    pthread_id = (pthread_t*) malloc(sizeof(pthread_t) * (this->totalThreadNums));
    for (int i = 0; i < (this->totalThreadNums); i++)
    {
        /*
        　　第一个参数为指向线程标识符的指针。
        　　第二个参数用来设置线程属性。
        　　第三个参数是线程运行函数的起始地址。
        　　最后一个参数是运行函数的参数。
        */
        pthread_create(&(pthread_id[i]), NULL, ThreadRoutine, NULL);
    }
    return ;

}

void* myThreadPool::ThreadRoutine(void *arg)
{
    while (1)
    {
        //若要从队列中取任务，则要先加锁
        pthread_mutex_lock(&(thread_mutex));
        /* 若线程池未关闭，但任务等待队列中并没有任务(没有正在等待的任务)) */
        while (!shutdown && taskList.size()==0)
        {   
            /* 等待条件变量的发起 */
            pthread_cond_wait(&thread_cond, &thread_mutex);
        }
        while (shutdown)
        {
            /* 释放当前遍历中的线程,并退出该线程 */
            pthread_mutex_unlock(&thread_mutex);
            pthread_exit(NULL);
        }
        
        
        /* 若有任务等待队列中存在线程，则将其运行 */
        // 1.从taskList中取出
        myTask *task = taskList.front();
        taskList.pop();
        /* 解锁 */
        pthread_mutex_unlock(&thread_mutex);

        /* 执行 */
        task->RunTask();
    }
}

void myThreadPool::AddTaskList(myTask *task)
{
    pthread_mutex_lock(&thread_mutex);
    this->taskList.push(task);
    pthread_mutex_unlock(&thread_mutex);

    /*等待队列中有任务了，条件变量发起, 
    当然如果没有空闲线程，这句没有任何作用*/ 
    pthread_cond_signal(&thread_cond);

}

int myThreadPool::GetTaskListSize()
{
    return taskList.size();
}


void myThreadPool::destroyPool(){
    /* 若已经调用了该函数，则直接返回 */
    if(shutdown) 
        return;
    
    shutdown = true;

    /* 唤醒所有的线程 */
    pthread_cond_broadcast(&thread_cond);

    /* 把所有的线程赶鸭子上架，逐个执行ThreadRoutine,逐个退出 */
    for (int i = 0; i < totalThreadNums; i++)
    {
        pthread_join(pthread_id[i],NULL);
    }
    
    /* 别忘了free，毕竟是malloc的 */
    free(pthread_id);
    pthread_id = NULL;
    
    pthread_mutex_destroy(&thread_mutex);
    pthread_cond_destroy(&thread_cond);

    return ;
}

