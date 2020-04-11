#ifndef GZ_HTTP_CONNECTOR
#define GZ_HTTP_CONNECTOR

/* Normal */
#include <string>
/* 3rd-party header */
#include <opencv2/opencv.hpp>
#include "3rdparty/nlohmann/json.hpp"

namespace gz_ag {

    #define RETRY_NUM 10
    // 1KB
    #define NORMAL_BUFFER_SIZE 1024
    // 2KB
    #define RECV_BUFFER_SIZE 2048
    // 8KB
    #define SEND_BUFFER_SIZE (1024*8)
    /* Outputing tag */
    #define TAG_ERROR "[ERROR] "
    #define TAG_INFO "[INFO] "
    #define TAG_WARNING "[WARNING] "

    /* ERROR CODE */
    #define CONNECTION_ERROR 4001
    #define SEND_BEFORE_CONNECTED 4002
    #define SEND_DATA_BEFORE_SEND_REQUEST_HEADER 4003
    #define SEND_REQUEST_HEADER_FAILED 4004
    #define IMAGE_EMPTY 5001
    #define MALLOC_ERROR 5002

    class HttpConnector {
        public:
            HttpConnector();
            HttpConnector(const std::string& server_ip, const uint port=8522);
            ~HttpConnector();

            int init(); // Initialize socket, and return socket status code
            void close_socket(); // Close connection

            void send_json(const std::string& encode_str, char** path_on_server,const std::string& router="/upload_image");
            void send_json_v2(const std::string& encode_str, nlohmann::json& result,const std::string& router="/upload_image");
            void send_cvMat(const std::string& img_path, char** path_on_server,const std::string& router);
            void send_cvMat(const cv::Mat& img, char** path_on_server, const std::string& router);
            
            // Return whether socket is closed.
            bool isClosed() {return this->bIsClosed;}
            bool isInit() { return this->bIsInit;}
            bool isConnected() { return this->bIsConnected;}
            char* getResponseHeader() { if(this->bIsSendRequest) return response_header; }
            void imageToBase64(const std::string& image_path, std::string& encode_str);
            void imageToBase64(const cv::Mat& img, std::string& encode_str, const std::string& img_type=".JPEG");
            void base64ToImage(const std::string &b64_str, cv::Mat &result_img);

        private:
            std::string mServerIP;
            uint mPort;
            int socket_fd, socket_init_status;
            bool bIsConnected = false;
            bool bIsInit = false;
            bool bIsSendRequest = false;
            bool bIsClosed = false;
            char response_header[NORMAL_BUFFER_SIZE] = { 0 };
            const char* process_label = "-/|\\";

            inline int sendHTTPRequestHeader(const std::string& router, const int data_size);
            void parse_response(const std::string& response_message, char** result);
            void parse_response_v2(const std::string& response_message, nlohmann::json& path_on_server);
    };
};

#endif /* GZ_HTTP_CONNECTOR */