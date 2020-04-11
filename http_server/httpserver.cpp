#include"httpserver.hpp"

/* standard library */
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/* For socket */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>  //对应于windows下的winsock2
#include <arpa/inet.h>

/*3rd party library*/
#include "3rdparty/nlohmann/json.hpp"
#include "3rdparty/littlstar/b64.h"
#include <opencv2/opencv.hpp>

myHttpServer::HttpServer::HttpServer()
{
    this->port = 8520;     
}

myHttpServer::HttpServer::HttpServer(const unsigned int port)
{
    this->port = port;     
}

myHttpServer::HttpServer::~HttpServer(){
    close_socket();
}

void myHttpServer::HttpServer::close_socket(){
    close(this->clifd);
    close(this->socket_fd);
}

int myHttpServer::HttpServer::HttpInit()
{ 
    //Create Socket
    this->socket_fd = socket(AF_INET, SOCK_STREAM,0);
    if(this->socket_fd < 0){
        std::cerr << TAG_ERROR << "ERROR ID:" << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    

    struct sockaddr_in serveraddr;
    serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
    serveraddr.sin_port = htons(this->port);
    serveraddr.sin_family = AF_INET;

    /*
    return 0 : bind success
    return -1: bind failed;
    */
    int iRet_bind = bind(this->socket_fd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr));
    if(iRet_bind == -1){
        std::cerr << TAG_ERROR << "ERROR ID:" << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    this->isBind = true;

    //Start To Listen
    int iRet_listen = listen(this->socket_fd, MAX_LISTEN);
    if(iRet_listen == -1){
        std::cerr << TAG_ERROR << "ERROR ID:" << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    this->isListen = true;
        
    //HttpInit Finish
    if(this->isBind && this->isListen) {
        this->isInit = true;
        return 0;
    }

    return -1;
}

int myHttpServer::HttpServer::sock_accept(const unsigned int lisfd)
{
    struct sockaddr clientAddr;
    socklen_t size = sizeof(struct sockaddr);
    memset((char *)&clientAddr, 0, sizeof(struct sockaddr));

    int iRet_clifd = accept(lisfd, &clientAddr, &size);
    // int iRet_clifd = accept(this->socket_fd, &clientAddr, &size);
    if (iRet_clifd == -1)
    {
        std::cerr << TAG_ERROR << "ERROR ID:" << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    return iRet_clifd;
}

void myHttpServer::HttpServer::ExtRoutAndContFromReqst()
{
    if(this->isInit == false){
        std::cerr << TAG_ERROR << "Status: HttpServer socket initialization failed" << std::endl;
        exit(SOCKET_INIALIZE_ERROR);
    }
    while (1)
    {
        this->clifd = sock_accept(this->socket_fd); 
        // char recv_buffer[HEADER_RECV_BUFFER_SIZE]={0};
        recv(this->clifd, this->httpHeader, HEADER_RECV_BUFFER_SIZE, 0);

        // 看看是GET还是POST
        char method[4]={0};
        int pos = httpMethod(this->httpHeader, method);                
        this->requestMethod = method;


        std:: cout << this->httpHeader <<std::endl;
        
        if(strcmp(this->requestMethod, "POST") == 0) //看是否是POST请求
        { 
            //parse httpheader
            /*
                char router[NORMAL_BUFFER_SIZE] = {0};
                char http_version[NORMAL_BUFFER_SIZE] = {0};
                int content_length;
                char connection[NORMAL_BUFFER_SIZE] = {0};
            */
            parseHeader(this->httpHeader, pos);
            char tmp_content_buffer[CONTENT_RECV_BUFFER_SIZE]={0};
            char* temp_content = (char*)malloc(MULTI_ADD_TO_CONTENT);
            memset(temp_content, 0, MULTI_ADD_TO_CONTENT);

            if(content_length <= CONTENT_RECV_BUFFER_SIZE){
                recv(this->clifd, temp_content, HEADER_RECV_BUFFER_SIZE, 0);
            }else
            {
                int total_times = (this->content_length / CONTENT_RECV_BUFFER_SIZE) + ((this->content_length  % CONTENT_RECV_BUFFER_SIZE > 0) ? 1 : 0);
                recv(this->clifd, tmp_content_buffer, CONTENT_RECV_BUFFER_SIZE, 0);
                strcat(temp_content, tmp_content_buffer);
                size_t last_buffer_size = (this->content_length  % CONTENT_RECV_BUFFER_SIZE);
                char last_recv_buffer[last_buffer_size]={0};
                for (int i = 2; i <= total_times; i++)
                {
                    if(i == 11) 
                    {
                        
                        recv(this->clifd, last_recv_buffer, last_buffer_size, 0);
                        strcat(temp_content, last_recv_buffer);
                    }else
                    {
                        recv(this->clifd, tmp_content_buffer, CONTENT_RECV_BUFFER_SIZE, 0);
                        strcat(temp_content, tmp_content_buffer);
                    }                    
                }                                
            }
            strncpy(this->content, temp_content,MULTI_ADD_TO_CONTENT);            

            // std::cout<< "Content:\r\n" << this->content <<std::endl;
            SetTask(this->router, this->content);
        }            
        // std::cout<< recv_buffer <<std::endl;
        if(strcmp(this->requestMethod, "GET") == 0)//看是否是GET请求
        { 
            /**/
            // SetTask()
        }
    }
}


inline void myHttpServer::HttpServer::SetTask(const char* router, const char* content)
{
    if(strcmp(this->requestMethod, "POST") == 0)
    {
        if(strcmp(router , "/upload_image")==0)
        {
            upload_image(content);
            if(strcmp(this->connection, "Close")==0 || strcmp(this->connection, "close")==0) close(this->clifd);
        }
        else
        {
            std::cerr << TAG_ERROR << "The request router isn't exit!" <<std::endl;
        }
    }
    else if(strcmp(this->requestMethod,"GET")==0)
    {
         std::cout << "call func() for GET func" <<std::endl;
    }
    else
    {
        std::cerr << TAG_ERROR << "Unkown request!" <<std::endl;
        return ;
    }
    return ;    
}

//看看是GET还是POST方式
int myHttpServer::HttpServer::httpMethod(const char* recvMessage, char* method)
{
    int pos=0;
    for (;;pos++) 
    {
        if(*(recvMessage+pos) == ' ') break;                
    }
    
    strncpy(method, recvMessage, pos);
    return pos;
}

int myHttpServer::HttpServer::parseHeader(const char* recvMessage, int pos)
{
    /*
        提取router
    */
    const char* c_router = recvMessage+pos+1;
    for (;;pos++)
    {
        if(*(recvMessage+pos+1) == ' ') break;                
    }            
    
    strncpy(this->router, c_router, pos-4); //把router提取出来                
    // std::cout<< this->router <<std::endl;


    /*
        提取httpversion
    */
    int current_pos = pos+1;
    for (;;)
    {
        if(*(recvMessage+pos-2) == '\r' && *(recvMessage+pos-1) == '\n') break;
        pos++;
    }
    strncpy(this->http_version, recvMessage+current_pos+1, pos-current_pos-3);
    // std::cout << this->http_version << std::endl;

    
    /*
        提取 Content_Length
    */
    int pos_contLength = count(recvMessage, "Content-Length: ") + strlen("Content-Length: ");
    int size_contLength=0;
    for (;; )
    {
        if(*(recvMessage+pos_contLength+size_contLength+2) == '\n' && *(recvMessage+pos_contLength+size_contLength+1) == '\r') break;
        size_contLength++;
    }    
    char contentLength[128] = {0};
    strncpy(contentLength, recvMessage+pos_contLength, size_contLength+1);
    this->content_length = atoi(contentLength);


    /*
        提取connection状态
    */
    int pos_connection = count(recvMessage, "Connection: ") + strlen("Connection: ");
    int size_connection = 0;
    for (;; )
    {
        if(*(recvMessage+pos_connection+size_connection+2) == '\n' && *(recvMessage+pos_connection+size_connection+1) == '\r') break;
        size_connection++;
    }   
    strncpy(this->connection, recvMessage+pos_connection, size_connection+1);


    /*
        提取 content
    */
    if(strcmp(this->requestMethod, "GET")==0){
        const std::string message = recvMessage;
        int message_size = message.size();      
        current_pos = pos;          
        for(;;) { //直到遇到连续的四个字符 为 \r\n\r\n
            if(message_size >= current_pos && 
                *(recvMessage+pos-4) == '\r' && *(recvMessage+pos-3) == '\n' &&
                *(recvMessage+pos-2) == '\r' && *(recvMessage+pos-1) == '\n') {
                
                break;
            }
            pos++;

            if (pos >= message_size) break;
        }
        strncpy(this->content, recvMessage+pos, message_size-pos);
    }

    return pos;
}

void myHttpServer::HttpServer::parseContent(const char* recvMessage){
    if(strcmp(this->requestMethod, "POST") == 0){

    }
}

void myHttpServer::HttpServer::upload_image(const char* content)
{
    nlohmann::json j = nlohmann::json::parse(content);  
    
    //Extract base64 type of img from json
    std::string b64_img1 = j["0_img_base64"]; 
    std:: cout<< b64_img1 <<std::endl;
    /*
        B64 3rd party lib
        param1: const char *
        param2: size
    */     
    std::string s((char*)(b64_decode(b64_img1.c_str(), b64_img1.size())));
    std::vector<uchar>data(s.begin(), s.end());
    cv::Mat result_img = cv::imdecode(data, CV_LOAD_IMAGE_COLOR);

    cv::imwrite("./UploadImg/1.jpg",result_img);
    /*
        还要return数据回去
    */
    return ;
    
}


int myHttpServer::HttpServer::count(const char *p1,const char *p2)
{
	int count=0;
	const char *pa = p1;
	const char *pb = p2;
	int i;
	int len=lenth(pb);
	while(*(pa+len-1) != '\0')
	{
		for(i = 0;i < len; i++)
		{
			if(*(pa+i) != *(pb+i))
			{
				break;
			}
			if(i == len-1)
			{
				return count;
			}
		}
		count++;
		pa++;
	}
	return 0;
}

int myHttpServer::HttpServer::lenth(const char *pstr)
{
	int len=0;
	while(*pstr++ != 0)
	{
		len++;
	}
	return len;
}