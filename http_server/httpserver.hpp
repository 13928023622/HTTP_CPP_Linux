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

namespace myHttpServer{

    #define MAX_LISTEN 20

    #define RETRY_NUM 10

    // MINIMUM SIZE
    #define MINIMUM_SIZE 64
    // 1KB
    #define NORMAL_BUFFER_SIZE 1024
    // 2KB
    #define HEADER_RECV_BUFFER_SIZE 2048
    // 8KB
    #define SEND_BUFFER_SIZE (1024*8)
    //64KB
    #define CONTENT_RECV_BUFFER_SIZE (1024*8)
    //128KB
    // #define MULTI_ADD_TO_CONTENT (1024*128)
    #define MULTI_ADD_TO_CONTENT (1024*1024*5)


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

    class HttpServer{
    public:
        HttpServer();
        HttpServer(const unsigned int port=8522);
        ~HttpServer();

        int HttpInit();    
        // void HttpRequest();
        void ExtRoutAndContFromReqst();
        

    private:
        int socket_fd, clifd;

        char httpHeader[HEADER_RECV_BUFFER_SIZE] ={0};
        const char* requestMethod;
        char router[NORMAL_BUFFER_SIZE] = {0};
        char content[MULTI_ADD_TO_CONTENT] = {0};
        char http_version[NORMAL_BUFFER_SIZE] = {0};
        int content_length;
        char connection[NORMAL_BUFFER_SIZE] = {0};

        unsigned int port;

        bool isInit = false;
        bool isBind = false;
        bool isListen = false;
        bool isConByCli = false;

        int sock_accept(const unsigned int lisfd);
        void close_socket();
        inline void SetTask(const char* router, const char* content);
        inline void retData(std::string return_data_dump);
        int httpMethod(const char* recvMessage, char* method);
        int parseHeader(const char* recvMessage, int pos);
        void parseContent(const char* recvMessage);
        void upload_image(const char* content);
        void base64ToImage(const std::string &b64_str, cv::Mat &result_img);

        int lenth(const char *pstr);
        int count(const char *p1,const char *p2);
        char *random_uuid(char buf[37]);
    };
};


#endif 