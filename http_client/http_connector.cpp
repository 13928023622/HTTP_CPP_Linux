#include "http_connector.hpp"

/* standard library */
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string.h>
/* For socket */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>  //对应于windows下的winsock2
#include <arpa/inet.h>
/* 3rd party library */
#include "3rdparty/nlohmann/json.hpp"
#include "3rdparty/littlstar/b64.h"
#include "base64.h"

gz_ag::HttpConnector::HttpConnector()
{
    this->mServerIP = "127.0.0.1";
    this->mPort = 8522;
}

gz_ag::HttpConnector::HttpConnector(const std::string& server_ip, const uint port)
{
    this->mServerIP = server_ip;
    this->mPort = port;
}

gz_ag::HttpConnector::~HttpConnector()
{
    close_socket();
}

int gz_ag::HttpConnector::init()
{
    // Set socket options
    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << TAG_ERROR << "ERROR ID: " << errno << ", MSG: " << strerror(errno) << std::endl;
        return errno;
    }
    this->bIsInit = true;

    sockaddr_in servaddr;
    servaddr.sin_addr.s_addr = inet_addr((this->mServerIP).c_str());//将主机字节序转为网络字节序
    servaddr.sin_port = htons(this->mPort); 
    std::cout<<htons(this->mPort)<<std::endl;
    servaddr.sin_family = AF_INET;

    // CONNCT TO SERVER
    bool isFirst = false;
    for (int i = 0; i < RETRY_NUM+1; i++) {
        this->socket_init_status = connect(socket_fd, (sockaddr*)&servaddr, sizeof(sockaddr));
        std::string _statue = this->socket_init_status<0?" connect unsuccess ":" connect success ";
        std::cout << TAG_INFO << " socket fd: " << this->socket_fd << std::endl;
        std::cout << TAG_INFO << " client id: " << this->socket_init_status <<std::endl;
        std::cout << TAG_INFO << _statue << std::endl;

        if (this->socket_init_status < 0) {
            std::cerr << TAG_ERROR << "Error id: " << errno << ", Error Message: " << strerror(errno);
            if (!isFirst) {
                // std::cerr << TAG_ERROR << "Error id: " << errno << ", Error Message: " << strerror(errno);
                isFirst = true;
                std::cerr << std::endl;
                continue;
            }
            // printf("\e[33m%s Error id: %d, Error message: %s, Retry (%d / %d)\r\e[0m", TAG_ERROR, errno, strerror(errno), i, RETRY_NUM);
            // fflush(stdout);
            std::cerr << "Retry ( " << i << " / " << RETRY_NUM << " )" << std::endl; 
            sleep(1);
        } else {
            this->bIsConnected = true;
            return this->socket_init_status; //代表连接成功了，return 退出循环
        }
    }
    std::cout << TAG_ERROR << " Connecting Error! Going to exit." << std::endl;
    exit(CONNECTION_ERROR);
}

inline int gz_ag::HttpConnector::sendHTTPRequestHeader(const std::string& router, const int data_size)
{
    const char* header_template = 
        // Request Line
        "POST %s HTTP/1.1\r\n" 
        "User-Agent: MatrixRecycle Device 1.0\r\n"
        "Connection: Close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %lu\r\n"
        "\r\n";
    char header[NORMAL_BUFFER_SIZE] = { 0 };
    if (router.empty()) {
        /*
        sprint作用于赋值，如下例子中
        char　buffer[50];
        sprintf(buffer,"%d plus %d is %d",a,b,a+b);赋予数值
        ==================================================
        所以下面的sprintf(header, header_template, "/", data_size);
        作用是将header_template赋值给header，并且使用“/”和data_size替代header_template中的%s和%lu
        */
        sprintf(header, header_template, "/", data_size);
    }
    else {
        sprintf(header, header_template, router.c_str(), data_size);
    }
    
    size_t send_size = send(this->socket_fd, header, strlen(header), 0);
    std::cout << "Request Header: \n" << header;
    // When send_size = -1
    // How to deal with this error.
    if (send_size < 0) {
        std::cerr << TAG_ERROR << "Send size: " << send_size << std::endl;
    }
    this->bIsSendRequest = true;

    return send_size;
}

void gz_ag::HttpConnector::send_json(const std::string& json_data, char** path_on_server, const std::string& router)
{
    if (!this->bIsConnected) {
        std::cerr << TAG_ERROR << "Status: Connected, Send before connected." << std::endl;
        exit(SEND_BEFORE_CONNECTED);
    }

    // nlohmann::json j;
    // j["img_base64"] = encode_str;
    // std::string json_data= j.dump();

    size_t data_size = json_data.size();
    size_t send_size = this->sendHTTPRequestHeader(router, data_size);//先发送httpheader
    if (send_size < 0) {
        std::cerr << TAG_ERROR << "Send request header failed." << std::endl;
        exit(SEND_REQUEST_HEADER_FAILED);
    }

    if (!this->bIsSendRequest) {
        std::cerr << TAG_ERROR << "Send data before sending request." << std::endl;
        exit(SEND_DATA_BEFORE_SEND_REQUEST_HEADER);
    }

    if ( data_size < SEND_BUFFER_SIZE) { //在发送data,size小于send_buffer_size
        send_size = send(this->socket_fd, json_data.c_str(), data_size, 0);
        std::cout << TAG_INFO << "Send Size: " << send_size << " Byte(s)." << std::endl;
    } else {
        /*
        函数c_str()就是将C++的string转化为C的字符串数组,c_str()生成一个constchar*指针,指向字符串的首地址
        */
        int total_times = data_size / SEND_BUFFER_SIZE + ((data_size % SEND_BUFFER_SIZE > 0) ? 1 : 0);//计算send次数
        for (int i = 0; i < total_times; i++) {
            if (i == (total_times-1))
                send_size = send(this->socket_fd, json_data.c_str()+i*SEND_BUFFER_SIZE, (data_size-i*SEND_BUFFER_SIZE), 0);    
            else
                send_size = send(this->socket_fd, json_data.c_str()+i*SEND_BUFFER_SIZE, SEND_BUFFER_SIZE, 0);
            // printf("\e[33m%s[%d/%d][%c] Send Bytes: %lu\r\e[0m", TAG_INFO, i+1, total_times, process_label[i%4], send_size);
            // fflush(stdout);
            std::cout << TAG_INFO << "Current / Total: " << i + 1 << " / " << total_times 
                << ", Send Size: " << send_size << " Byte(s)." << std::endl;
        }
    }

    // Waiting for http server response
    std::cout << "[INFO] Wait for 1000 microseconds..." << std::endl;
    sleep(2);
    std::cout << "[INFO] Timer stop, go on" << std::endl;
    // Receive http code
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    size_t recv_size = recv(this->socket_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
    std::cout << "[INFO] Size of recv: " << recv_size << std::endl;
    std::cout << "[INFO] Message of recv: " << recv_buffer << std::endl;
    
    this->parse_response(recv_buffer, path_on_server);

    this->close_socket();
}


void gz_ag::HttpConnector::parse_response(const std::string& response_message, char** path_on_server)
{
    std::cout << TAG_INFO << "In func 'void gz_ag::HttpConnector::parse_response(const std::string& response_message, char* path_in_server)'" << std::endl;
    size_t message_size = response_message.size();
    // Parse response message
    // ****** Parse buffer data
    int pos = 4;
    const char* message_ptr = response_message.data();
    std::cout << "Current pos:" << pos << " Going to parse\n";
    for(;;) { //直到遇到连续的四个字符 为 \r\n\r\n
        if(message_size >=4 && 
            *(message_ptr+pos-4) == '\r' && *(message_ptr+pos-3) == '\n' &&
            *(message_ptr+pos-2) == '\r' && *(message_ptr+pos-1) == '\n') {
            
            break;
        }
        pos++;

        if (pos >= message_size) break;
    }
    std::cout << "Current pos:" << pos << " Going to parse\n";
    if (pos > 4) {
        // Response header
        strncpy(this->response_header, message_ptr, pos-4);
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "[INFO] What receives from server(response header length: " << pos << "): \n" 
                << this->response_header << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        float http_version = .0f;
        int status_code = 0;
        /*
        常见用法。
　　     char buf[512];
　　     sscanf("abc123456", "abc%s", buf);//此处buf是数组名，它的意思是将123456以%s的形式存入buf中！

        以下用法是，将response_header中HTTP/后面的http版本和返回的状态码提取出来
        */
        sscanf(response_header, "HTTP/%f %d", &http_version, &status_code);

        int start_pos = 5;
        for(;;) {
            if ('A' <= message_ptr[start_pos] && 'Z' >= message_ptr[start_pos]) break;
            start_pos++;
        }
        int slash_r_pos = start_pos;
        for(;;) {
            if (message_ptr[slash_r_pos] == '\r') break;
            slash_r_pos++;
        }

        char http_status[128] = {0};
        strncpy(http_status, message_ptr+start_pos, slash_r_pos - start_pos);
        std::cout << "[INFO] http version: " << http_version << " status code: " << status_code 
            << ", http status: " << http_status << std::endl;

        if (status_code != 200) {
            std::cerr << TAG_ERROR << "Status code: " << status_code << ", Status: " << http_status << std::endl;
            return ;
            // throw std::string("Bad request: "+status_code);
        }
        /*-------------------------------------------------------------------------------------------------------*/
        // Data
        int content_length = 0;
        char content_buffer[RECV_BUFFER_SIZE] = {0};

        /*strncpy参数：
            param1: dataBuffer
            param2：取数据地址起始
            param3：取数据长度
        */
        strncpy(content_buffer, message_ptr+pos, message_size-pos); 
        std::cout << "[INFO] Content :" << content_buffer << std::endl;

        std::cout << "[INFO] Content length: " << message_size - pos << std::endl;

        // Parse json data, get image path
        std::cout << "[INFO] In json parser" << std::endl;
        bool find_200 = false;
        try {
            nlohmann::json json_obj = nlohmann::json::parse(content_buffer);
            if (200 == json_obj["status_code"].get<int>()) {
                find_200 = true;
            }
            else {
                std::cout << "[HTTP ERROR] status_code: " << json_obj["status_code"].get<int>() << 
                    ", message: "<<json_obj["message"].get<std::string>()<< std::endl;
            }

            //Request success and Receive success
            if (find_200) {
                if (*path_on_server == NULL){
                    *path_on_server = (char*)malloc(sizeof(char) * NORMAL_BUFFER_SIZE);
                    /*
                        void *memset(void *s, int v, size_t n);  
                        这里s可以是数组名，也可以是指向某一内在空间的指针；
                        v为要填充的值；
                        n为要填充的字节数；
                    */
                    memset(*path_on_server, 0, sizeof(char) * NORMAL_BUFFER_SIZE);

                    if (path_on_server == NULL) {
                        std::cerr << TAG_ERROR << "Malloc error" << std::endl;
                        exit(MALLOC_ERROR);
                    }
                }
                
                strncpy(*path_on_server, 
                    json_obj["image_path"].get<std::string>().c_str(), 
                    json_obj["image_path"].get<std::string>().size());
            }
        }
        catch (nlohmann::json::exception err) {
            std::cerr << "[ERROR] exception id: " << err.id <<
                        " message: " << err.what() << std::endl;
        }
    }
}

void gz_ag::HttpConnector::send_json_v2(const std::string& json_data, nlohmann::json& result, const std::string& router)
{
    if (!this->bIsConnected) {
        std::cerr << TAG_ERROR << "Status: Connected, Send before connected." << std::endl;
        exit(SEND_BEFORE_CONNECTED);
    }

    // nlohmann::json j;
    // j["img_base64"] = encode_str;
    // std::string json_data= j.dump();

    size_t data_size = json_data.size();
    size_t send_size = this->sendHTTPRequestHeader(router, data_size);//先发送httpheader
    if (send_size < 0) {
        std::cerr << TAG_ERROR << "Send request header failed." << std::endl;
        exit(SEND_REQUEST_HEADER_FAILED);
    }

    if (!this->bIsSendRequest) {
        std::cerr << TAG_ERROR << "Send data before sending request." << std::endl;
        exit(SEND_DATA_BEFORE_SEND_REQUEST_HEADER);
    }

    std::cout<< json_data.c_str() << std::endl;
    
    if ( data_size < SEND_BUFFER_SIZE) { //在发送data,size小于send_buffer_size
        send_size = send(this->socket_fd, json_data.c_str(), data_size, 0);
        std::cout << TAG_INFO << "Send Size: " << send_size << " Byte(s)." << std::endl;
    } else {
        /*
        函数c_str()就是将C++的string转化为C的字符串数组,c_str()生成一个constchar*指针,指向字符串的首地址
        */
        int total_times = data_size / SEND_BUFFER_SIZE + ((data_size % SEND_BUFFER_SIZE > 0) ? 1 : 0);//计算send次数
        for (int i = 0; i < total_times; i++) {
            if (i == (total_times-1))
                send_size = send(this->socket_fd, json_data.c_str()+i*SEND_BUFFER_SIZE, (data_size-i*SEND_BUFFER_SIZE), 0);    
            else
                send_size = send(this->socket_fd, json_data.c_str()+i*SEND_BUFFER_SIZE, SEND_BUFFER_SIZE, 0);
            // printf("\e[33m%s[%d/%d][%c] Send Bytes: %lu\r\e[0m", TAG_INFO, i+1, total_times, process_label[i%4], send_size);
            // fflush(stdout);
            std::cout << TAG_INFO << "Current / Total: " << i + 1 << " / " << total_times 
                << ", Send Size: " << send_size << " Byte(s)." << std::endl;
        }
    }

    

    // Waiting for http server response
    std::cout << "[INFO] Wait for 1000 microseconds..." << std::endl;
    sleep(2);
    std::cout << "[INFO] Timer stop, go on" << std::endl;
    // Receive http code
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    size_t recv_size = recv(this->socket_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
    std::cout << "[INFO] Size of recv: " << recv_size << std::endl;
    std::cout << "[INFO] Message of recv: " << recv_buffer << std::endl;
    
    this->parse_response_v2(recv_buffer, result);

    this->close_socket();
}


void gz_ag::HttpConnector::parse_response_v2(const std::string& response_message, nlohmann::json& result)
{
    std::cout << TAG_INFO << "In func 'void gz_ag::HttpConnector::parse_response(const std::string& response_message, char* path_in_server)'" << std::endl;
    size_t message_size = response_message.size();
    // Parse response message
    // ****** Parse buffer data
    int pos = 4;
    const char* message_ptr = response_message.data();
    std::cout << "Current pos:" << pos << " Going to parse\n";
    for(;;) { //直到遇到连续的四个字符 为 \r\n\r\n
        if(message_size >=4 && 
            *(message_ptr+pos-4) == '\r' && *(message_ptr+pos-3) == '\n' &&
            *(message_ptr+pos-2) == '\r' && *(message_ptr+pos-1) == '\n') {
            
            break;
        }
        pos++;

        if (pos >= message_size) break;
    }
    std::cout << "Current pos:" << pos << " Going to parse\n";
    if (pos > 4) {
        // Response header
        strncpy(this->response_header, message_ptr, pos-4);
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "[INFO] What receives from server(response header length: " << pos << "): \n" 
                << this->response_header << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        float http_version = .0f;
        int status_code = 0;
        /*
        常见用法。
　　     char buf[512];
　　     sscanf("abc123456", "abc%s", buf);//此处buf是数组名，它的意思是将123456以%s的形式存入buf中！

        以下用法是，将response_header中HTTP/后面的http版本和返回的状态码提取出来
        */
        sscanf(response_header, "HTTP/%f %d", &http_version, &status_code);

        int start_pos = 5;
        for(;;) {
            if ('A' <= message_ptr[start_pos] && 'Z' >= message_ptr[start_pos]) break;
            start_pos++;
        }
        int slash_r_pos = start_pos;
        for(;;) {
            if (message_ptr[slash_r_pos] == '\r') break;
            slash_r_pos++;
        }

        char http_status[128] = {0};
        strncpy(http_status, message_ptr+start_pos, slash_r_pos - start_pos);
        std::cout << "[INFO] http version: " << http_version << " status code: " << status_code 
            << ", http status: " << http_status << std::endl;

        if (status_code != 200) {
            std::cerr << TAG_ERROR << "Status code: " << status_code << ", Status: " << http_status << std::endl;
            return ;
            // throw std::string("Bad request: "+status_code);
        }
        /*-------------------------------------------------------------------------------------------------------*/
        // Data
        int content_length = 0;
        char content_buffer[RECV_BUFFER_SIZE] = {0};

        /*strncpy参数：
            param1: dataBuffer
            param2：取数据地址起始
            param3：取数据长度
        */
        strncpy(content_buffer, message_ptr+pos, message_size-pos); 
        std::cout << "[INFO] Content :" << content_buffer << std::endl;

        std::cout << "[INFO] Content length: " << message_size - pos << std::endl;

        // Parse json data, get image path
        std::cout << "[INFO] In json parser" << std::endl;
        bool find_200 = false;
        try {
            nlohmann::json json_obj = nlohmann::json::parse(content_buffer);
            if (200 == json_obj["status_code"].get<int>()) {
                find_200 = true;
            }
            else {
                std::cout << "[HTTP ERROR] status_code: " << json_obj["status_code"].get<int>() << 
                    ", message: "<<json_obj["message"].get<std::string>()<< std::endl;
            }

            //Request success and Receive success
            if (find_200) {
                result = json_obj;
                
            }
        }
        catch (nlohmann::json::exception err) {
            std::cerr << "[ERROR] exception id: " << err.id <<
                        " message: " << err.what() << std::endl;
        }
    }
}


void gz_ag::HttpConnector::send_cvMat(const std::string& img_path, char** path_on_server, const std::string& router)
{
    std::string encode_str;
    this->imageToBase64(img_path, encode_str);
    this->send_json(encode_str, path_on_server, router);
}


void gz_ag::HttpConnector::send_cvMat(const cv::Mat& img, char** path_on_server, const std::string& router)
{
    std::string encode_str;
    this->imageToBase64(img, encode_str);
    this->send_json(encode_str, path_on_server, router);
    // std::cout << TAG_INFO << "path_in_server: " << path_on_server << std::endl;

}


void gz_ag::HttpConnector::imageToBase64(const std::string& image_path, std::string& encode_str)
{
    std::cout << "[INFO] In func \'void gz_ag::imageToBase64(const std::string& image_path, std::string& encode_str)\' " << std::endl;
   
    // Read image from a given path
    cv::Mat img = cv::imread(image_path);
    if (img.empty()) {
        std::cout << "[ERROR] Image is empty, please check your image path." << std::endl;
        exit(IMAGE_EMPTY);
    }

    // Get image extension
    size_t pos_of_dot = image_path.find_last_of('.');
    char img_type[16] = { 0 };
    image_path.copy(img_type, image_path.size()-pos_of_dot, pos_of_dot);
    std::cout << "[INFO] image type: " << img_type << std::endl;

    this->imageToBase64(img, encode_str, img_type);
    
   return;
}


void gz_ag::HttpConnector::imageToBase64(const cv::Mat& img, std::string& encode_str, const std::string& img_type)
{
    std::vector<uchar> vecImg;
    std::vector<int> vecCompression_params;
    
    if (img_type == ".JPEG") {
        // set compression_params;
        vecCompression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        vecCompression_params.push_back(90);    
    } else if (img_type == ".PNG") {
        vecCompression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        vecCompression_params.push_back(1);    
    }

    // Convert mat to base64
    std::cout << "[INFO] size of image: " << img.size() /*img.cols*img.rows*img.channels()*/ << std::endl;
    cv::imencode(img_type, img, vecImg, vecCompression_params);
    encode_str = b64_encode(vecImg.data(), vecImg.size());
    std::cout << vecImg.data() << std::endl;
    return;
}


void gz_ag::HttpConnector::base64ToImage(const std::string &b64_str, cv::Mat &result_img)
{
    std::string base64_dec_str = base64_decode(b64_str);
    std::vector<uchar>data(base64_dec_str.begin(),base64_dec_str.end());
    result_img = cv::imdecode(data, CV_LOAD_IMAGE_COLOR);
}


void gz_ag::HttpConnector::close_socket()
{
    if (this->bIsConnected && !this->bIsClosed) {
        close(this->socket_fd);
        std::cout << TAG_INFO << "Close socket: " << this->socket_fd << std::endl;
    }
    else{
        std::cout << TAG_INFO << "Connection is not established or socket has been closed" << std::endl;
    }
}