/*
  Author: Mephis Chen
  Date: 2020-04-17
*/
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
// #include "3rdparty/littlstar/b64.h"
#include "base64.h"
#include <opencv2/opencv.hpp>


// myHttpServer::HttpServer::HttpServer()
// {
//     this->port = 8522;     
// }

myHttpServer::HttpServer::HttpServer(const unsigned int port)
{
    this->port = port;     
}

myHttpServer::HttpServer::~HttpServer(){
    close_socket();
}

void myHttpServer::HttpServer::close_socket(){
    if(this -> isInit)
    {
        std::cout<< TAG_INFO << "Close server" << std::endl;
        close(this->socket_fd);
    }
}

void myHttpServer::HttpServer::ServerRun()
{
    HttpInit();
    ExecReqst();
    return ;
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

    int iRet_connetct_fd = accept(lisfd, &clientAddr, &size);
    // int iRet_clifd = accept(this->socket_fd, &clientAddr, &size);
    if (iRet_connetct_fd == -1)
    {
        std::cerr << TAG_ERROR << "ERROR ID:" << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    this->isConByCli = true;
    return iRet_connetct_fd;
}

void myHttpServer::HttpServer::ExecReqst()
{
    if(this->isInit == false){
        std::cerr << TAG_ERROR << "Status: HttpServer socket initialization failed" << std::endl;
        exit(SOCKET_INIALIZE_ERROR);
    }

    /*创建线程池*/
    CThreadPool Pool(3);
    while (1)
    {
        int connectfd = sock_accept(this->socket_fd);         
        CTask* task = new HttpServer;
        task->SetConnFd(connectfd);
        Pool.AddTask(task);
    }
}

/*
    1.查看是post还是get
    2.解析httpheader其中的信息
    3.若是post，则server端发送100响应给client端，要求client端发送接下来的数据content
    4.recv得到数据主体content，使用nlohmann进行解析，进一步得到json对象
    5.执行httpheader中对应的router的函数
*/
void myHttpServer::HttpServer::Run(){
    int connectfd = GetConnFd();

    char httpHeader[HEADER_RECV_BUFFER_SIZE] ={0};
    recv(connectfd, httpHeader, HEADER_RECV_BUFFER_SIZE, 0);

    // 看看是GET还是POST
    char method[4]={0};
    int pos = httpMethod(httpHeader, method);                
    const char* requestMethod = method;
    

    std:: cout << httpHeader <<std::endl;
    
    if(strcmp(requestMethod, "POST") == 0) //看是否是POST请求
    {                         
        //parse httpheader
        char router[NORMAL_BUFFER_SIZE] = {0};
        char http_version[NORMAL_BUFFER_SIZE] = {0};
        int content_length;
        char connection[NORMAL_BUFFER_SIZE] = {0};
        parseHeader(httpHeader, pos, router, http_version, content_length, connection);
        std::cout << "CL:" << content_length <<std::endl;


        /*发送100响应*/
        try{
            nlohmann::json reponse_100;
            reponse_100["status_code"]  = SEND_YOUR_CONTENT; 
            reponse_100["message"]  = "'Start to send content!'";
            send(connectfd, reponse_100.dump().c_str(), reponse_100.dump().size(),0);
        }
        catch (nlohmann::json::exception err) {
            std::cerr << "[ERROR] exception id: " << err.id <<
                        " message: " << err.what() << std::endl;
        }            

        sleep(1);

        /*Start to receive content*/            
        char tmp_content_buffer[CONTENT_RECV_BUFFER_SIZE]={0};
        char* content = (char*)malloc(MULTI_ADD_TO_CONTENT);
        memset(content, 0, MULTI_ADD_TO_CONTENT);
        
        // char content[MULTI_ADD_TO_CONTENT]={0};

        if(content_length <= CONTENT_RECV_BUFFER_SIZE){
            recv(connectfd, content, HEADER_RECV_BUFFER_SIZE, 0);
        }else
        {                
            int total_times = (content_length / CONTENT_RECV_BUFFER_SIZE) + ((content_length  % CONTENT_RECV_BUFFER_SIZE > 0) ? 1 : 0);

            for (int i = 1; i <= total_times; i++)
            {
                int recv_size;
                if(i == total_times) 
                {
                    int last_buffer_size = (content_length  % CONTENT_RECV_BUFFER_SIZE);
                    char last_recv_buffer[last_buffer_size]={0};
                    recv_size = recv(connectfd, last_recv_buffer, last_buffer_size, 0);  
                    
                    strncat(content, last_recv_buffer,last_buffer_size);
                    // std::cout<< last_recv_buffer;
                }else
                {
                    recv_size = recv(connectfd, tmp_content_buffer, CONTENT_RECV_BUFFER_SIZE, 0);

                    strncat(content, tmp_content_buffer,CONTENT_RECV_BUFFER_SIZE);
                    // std::cout<< tmp_content_buffer;
                }          
                std::cout << TAG_INFO << "Current / Total: " << i << " / " << total_times 
                    << ", Recv Size: " << recv_size << " Byte(s)." << std::endl;                 
            }      
                                                
        }
        std::cout<< "---------------------"<<std::endl;
        sleep(15);
        // std::cout << content <<std::endl;
        std::cout<< "---------------------"<<std::endl;            
        SetTask(router, content, connectfd, requestMethod, connection);
        free(content);
    }            
    // std::cout<< recv_buffer <<std::endl;
    if(strcmp(requestMethod, "GET") == 0)//看是否是GET请求
    { 
        // char* getContent;
        // parseHeader(this->httpHeader, pos,getContent);
        // SetTask()
    }
}


inline void myHttpServer::HttpServer::SetTask(const char* router, const char* content, int cur_clifd, const char* requestMethod, const char* connection)
{
    if(strcmp(requestMethod, "POST") == 0)
    {
        if(strcmp(router , "/upload_image")==0)
        {            
            std::cout << TAG_INFO<< "Request router:" << router << std::endl;
            upload_image(content,cur_clifd);
            if(strcmp(connection, "Close")==0 || strcmp(connection, "close")==0) 
            {
                std::cout << TAG_INFO << "Close client socketfd:" << cur_clifd <<std::endl;
                close(cur_clifd);
            }
        }
        else
        {
            std::cerr << TAG_ERROR << "The request router isn't exit!" <<std::endl;
            nlohmann::json return_j;
            return_j["status_code"]  = THE_PAGE_IS_NOT_FOUND; 
            return_j["message"]  = "'The request router isn't found!'";
            retData(return_j.dump(),cur_clifd);
        }
    }
    else if(strcmp(requestMethod,"GET")==0)
    {
         std::cout << "call func() for GET func" <<std::endl;
    }
    else
    {
        std::cerr << TAG_ERROR << "Unkown request method!" <<std::endl;
        nlohmann::json return_j;
        return_j["status_code"]  = THE_PAGE_IS_NOT_FOUND; 
        return_j["message"]  = "'Unkown request method!'";
        retData(return_j.dump(),cur_clifd);
        
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


int myHttpServer::HttpServer::parseHeader
    (const char* recvMessage, int pos, char* router, char* http_version, int& content_length, char* connection)
{
    /*
        提取router
    */
    const char* c_router = recvMessage+pos+1;
    for (;;pos++)
    {
        if(*(recvMessage+pos+1) == ' ') break;                
    }            
    
    strncpy(router, c_router, pos-4); //把router提取出来                
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
    strncpy(http_version, recvMessage+current_pos+1, pos-current_pos-3);
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
    content_length = atoi(contentLength);


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
    strncpy(connection, recvMessage+pos_connection, size_connection+1);


    return pos;
}

void myHttpServer::HttpServer::parseContent(const char* recvMessage){
    // if(strcmp(this->requestMethod, "POST") == 0){

    // }
}

void myHttpServer::HttpServer::base64ToImage(const std::string &b64_str, cv::Mat &result_img)
{
    std::string base64_dec_str = base64_decode(b64_str);
    std::vector<uchar>data(base64_dec_str.begin(),base64_dec_str.end());
    result_img = cv::imdecode(data, CV_LOAD_IMAGE_COLOR);
}


void myHttpServer::HttpServer::upload_image(const char* content, int cur_clifd)
{
    //Create return json object
    nlohmann::json return_j;
    if(strlen(content)==0)
    {
        return_j["status_code"]  = JSON_DATA_IS_NULL; 
        return_j["message"]  = "'Json data is null.'";
        retData(return_j.dump(),cur_clifd);
        return ;
    }
    std::cout << content << std::endl;           
    //Get the request json object
    nlohmann::json request_j = nlohmann::json::parse(content);  
    int nums = request_j["img_num"];
    if(nums <= 0)
    {            
        return_j["status_code"]  = IMG_NUM_IS_ZERO; 
        return_j["message"]  = "'Argument \'img_num\' of Json date is zero.'";
        std:: cout << TAG_ERROR << return_j["message"] <<std::endl;
        retData(return_j.dump(),cur_clifd);
        return ;
    }

    for (int i = 0; i < nums; i++)
    {
        //transfer base64 to img mat
        char tmp_img_name[MINIMUM_SIZE]={0};
        sprintf(tmp_img_name, "%d_img_base64", i);
        std::string b64_img = request_j[tmp_img_name];
        cv::Mat res_img;
        base64ToImage(b64_img, res_img);

        //save img mat
        char uuid[37]={0};
        char uuid_name[MINIMUM_SIZE*4] = {0};
        sprintf(uuid_name, "./UploadImg/%s.jpg", random_uuid(uuid));
        cv::imwrite(uuid_name,res_img);
    }
    
    return_j["status_code"] = 200; 
    return_j["message"] = "Upload Success!";
    std::cout<< TAG_INFO << "Return content size:" << return_j.dump().size() << std::endl;;
    std::cout << TAG_INFO << return_j["message"] << std::endl;
    
    retData(return_j.dump(),cur_clifd);


    return ;
    
}


inline void myHttpServer::HttpServer::retData(std::string return_data_dump, int cur_clifd)
{
    if(return_data_dump.size() == 0){
        return ;
    }
    const char* header_template = 
        // Request Line
        "HTTP/1.1 200 OK\r\n" 
        "Server: HttpServer 1.0\r\n"
        "Connection: Close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s";
    char header[NORMAL_BUFFER_SIZE] = {0};
    sprintf(header,header_template, return_data_dump.size(), return_data_dump.c_str());
    
    int send_size = send(cur_clifd, header, NORMAL_BUFFER_SIZE,0);
    if(send_size < 0){
        std::cerr << TAG_ERROR << "Send size:" << send_size << std::endl;
    }
    
}



// tool
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


/**
 * Create random UUID
 *
 * param buf - buffer to be filled with the uuid string
 */

char* myHttpServer::HttpServer::random_uuid(char buf[37])
{
    const char *c = "89ab";
    char *p = buf;
    int n;
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;
        switch( n )
        {
            case 6:
                sprintf(p, "4%x", b%15 );
                break;
            case 8:
                sprintf(p, "%c%x", c[rand()%strlen(c)], b%15 );
                break;
            default:
                sprintf(p, "%02x", b);
            break;
        }
        p += 2;
        switch( n )
        {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    *p = 0;
    return buf;
}


// int myHttpServer::HttpServer::parseHeader(const char* recvMessage, int pos, char* getMethod_content)
// {
//     /*
//         提取router
//     */
//     const char* c_router = recvMessage+pos+1;
//     for (;;pos++)
//     {
//         if(*(recvMessage+pos+1) == ' ') break;                
//     }            
    
//     strncpy(this->router, c_router, pos-4); //把router提取出来                
//     // std::cout<< this->router <<std::endl;


//     /*
//         提取httpversion
//     */
//     int current_pos = pos+1;
//     for (;;)
//     {
//         if(*(recvMessage+pos-2) == '\r' && *(recvMessage+pos-1) == '\n') break;
//         pos++;
//     }
//     strncpy(this->http_version, recvMessage+current_pos+1, pos-current_pos-3);
//     // std::cout << this->http_version << std::endl;

    
//     /*
//         提取 Content_Length
//     */
//     int pos_contLength = count(recvMessage, "Content-Length: ") + strlen("Content-Length: ");
//     int size_contLength=0;
//     for (;; )
//     {
//         if(*(recvMessage+pos_contLength+size_contLength+2) == '\n' && *(recvMessage+pos_contLength+size_contLength+1) == '\r') break;
//         size_contLength++;
//     }    
//     char contentLength[128] = {0};
//     strncpy(contentLength, recvMessage+pos_contLength, size_contLength+1);
//     this->content_length = atoi(contentLength);


//     /*
//         提取connection状态
//     */
//     int pos_connection = count(recvMessage, "Connection: ") + strlen("Connection: ");
//     int size_connection = 0;
//     for (;; )
//     {
//         if(*(recvMessage+pos_connection+size_connection+2) == '\n' && *(recvMessage+pos_connection+size_connection+1) == '\r') break;
//         size_connection++;
//     }   
//     strncpy(this->connection, recvMessage+pos_connection, size_connection+1);


//     /*
//         提取 content
//     */
//     if(strcmp(this->requestMethod, "GET")==0){
//         const std::string message = recvMessage;
//         int message_size = message.size();      
//         current_pos = pos;          
//         for(;;) { //直到遇到连续的四个字符 为 \r\n\r\n
//             if(message_size >= current_pos && 
//                 *(recvMessage+pos-4) == '\r' && *(recvMessage+pos-3) == '\n' &&
//                 *(recvMessage+pos-2) == '\r' && *(recvMessage+pos-1) == '\n') {
                
//                 break;
//             }
//             pos++;

//             if (pos >= message_size) break;
//         }        
//         strncpy(getMethod_content, recvMessage+pos, message_size-pos);
//     }

//     return pos;
// }
