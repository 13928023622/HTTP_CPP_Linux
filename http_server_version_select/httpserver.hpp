#ifndef HTTPSERVER_H
#define HTTPSERVER_H

/* Normal */
#include <string>
#include <opencv2/opencv.hpp>

/* For socket */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>  //对应于windows下的winsock2
#include <arpa/inet.h>

/* thread */
#include<pthread.h>

/* thread pool */
// #include "threadpool/myThreadPool.h"

namespace myHttpServer{

    #define MAX_LISTEN 20

    #define RETRY_NUM 10

    // MINIMUM SIZE
    #define MINIMUM_SIZE 64
    // 1KB
    #define NORMAL_BUFFER_SIZE 1024
    // 2KB
    #define HEADER_RECV_BUFFER_SIZE 1024
    // 8KB
    #define SEND_BUFFER_SIZE (1024*8)
    //8KB
    #define CONTENT_RECV_BUFFER_SIZE (1024*8)
    //128KB
    // #define MULTI_ADD_TO_CONTENT (1024*128)
    #define MULTI_ADD_TO_CONTENT (1024*512)


    /* Outputing tag */
    #define TAG_ERROR "[ERROR] "
    #define TAG_INFO "[INFO] "
    #define TAG_WARNING "[WARNING] "

    /* ERROR CODE */

    #define SOCKET_INIALIZE_ERROR 5401
    #define SEND_BEFORE_CONNECTED 5402
    #define SEND_DATA_BEFORE_SEND_REQUEST_HEADER 5403

    #define THE_PAGE_IS_NOT_FOUND 5001
    #define JSON_DATA_IS_NULL 5002
    #define IMG_NUM_IS_ZERO 5003
    #define SEND_YOUR_CONTENT 5100

    class HttpServer{
    public:
        // HttpServer();
        HttpServer(const unsigned int port=8522);
        ~HttpServer();

        void ServerRun();

        

    private:
        int socket_fd, clifd;
        int clifds[FD_SETSIZE];//客户端池
        int cli_nums=0;
        fd_set allSet;//fd_set对象
        // pthread_t conn_thread_id; //线程指针 ;

        // char router[NORMAL_BUFFER_SIZE] = {0};        
        // char http_version[NORMAL_BUFFER_SIZE] = {0};
        // int content_length;
        // char connection[NORMAL_BUFFER_SIZE] = {0};

        unsigned int port;

        /*status*/
        bool isInit = false;
        bool isBind = false;
        bool isListen = false;
        bool isConByCli = false;

        /*http*/
        int HttpInit();    
        // void HttpRequest();
        void ExecReqst();
        void RunTask(int connectfd);

        void sock_accept();
        static void* call_sock_accept(void* pThis);

        void close_socket();
        inline void SetTask(const char* router, const char* content, int cur_clifd, const char* requestMethod, const char* connection);
        inline void retData(std::string return_data_dump, int cur_clifd);
        int httpMethod(const char* recvMessage, char* method);
        // int parseHeader(const char* recvMessage, int pos, char* getMethod_content);
        int parseHeader(const char* recvMessage, int pos, char* router, char* http_version, int& content_length, char* connection);
        void parseContent(const char* recvMessage);

        /*tasks*/
        void upload_image(const char* content, int cur_clifd);

        /*tool*/
        void base64ToImage(const std::string &b64_str, cv::Mat &result_img);
        int lenth(const char *pstr);
        int count(const char *p1,const char *p2);
        char *random_uuid(char buf[37]);
    };
};


#endif 