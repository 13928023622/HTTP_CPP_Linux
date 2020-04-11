#include "image_decoder_and_encoder.hpp"
/* Standard Library */
#include <vector>
#include <string>

// Third Library
/* OpenCV Library */
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

std::vector<std::vector<uchar>> _matToImageByte(std::vector<cv::Mat> mat_buffer) {
    /*
    函数名： _matToImageByte
    功能： 把opencv拍摄的Mat格式图片编码为字符串类型的数据，加快的数据传输速度（如需要把图片传输至服务器时可以使用该函数，加快传输速度）
    输入参数：
    ----------------
        mat_buffer： vector<Mat>
            由opencv拍摄的mat格式的图片数据
    返回值：
    ----------------
        imgs_strEncode_buffer： vector<string>
            将该摄像头拍摄的图片以string格式数据存储（因为可能拍摄了多张，所以使用了vector）,每一个string类型元素表示一张图片。
    */    
    try
    {        
        if(!mat_buffer.empty()){            
            int size = mat_buffer.size();
            std::vector<std::vector<uchar>> imgs_strEncode_buffer;        
            for (int i = 0; i < size; i++)
            {
                std::vector<uchar> data_encode; //单张图片的buffer        
                cv::imencode(".jpg", mat_buffer[i], data_encode);
                std::cout << data_encode.size() << std::endl;
                imgs_strEncode_buffer.push_back(data_encode);        
            }    
            return imgs_strEncode_buffer;
        }else{
            throw std::string("ERROR 107:mat is none");
        }        
    }
    catch(std::string s)
    {
       std::cout << s << std::endl;
    }	
}


std::vector<cv::Mat> _imageByteToMat(std::vector<std::vector<uchar>> imgs_strEncode_buffer){
    /*
    函数名： _imageByteToMat
    功能： 解析从客户端传输而来的数据流，在服务端将数据流转化为Mat形式
    输入参数：
    ----------------
        imgs_strEncode_buffer: std::vector<std::vector<uchar>>
            由_matToImageByte方法在客户端把Mat格式数据以字符的形式存储，并传输至服务器
    返回值：
    ----------------
        mat_buffer： std::vector<Mat>
            在服务器中将传输流中的字符格式转化为Mat格式，以供opencv方法进行操作，其中每一个Mat表示一张图片。
    */
    try
    {
        if(!imgs_strEncode_buffer.empty()){
            int size = imgs_strEncode_buffer.size();
            std::vector<cv::Mat> mat_buffer;
            cv::Mat img_mat;
            for (int i = 0; i < size; i++)
            {
                img_mat = cv::imdecode(imgs_strEncode_buffer[i], cv::IMREAD_ANYCOLOR);
                mat_buffer.push_back(img_mat.clone());        
            }
            std::cout << mat_buffer.size()<< std::endl;
            return mat_buffer;
        }else{
            throw std::string("ERROR 108: can't get the data streams from client!!");
        }        
    }
    catch(std::string s)
    {
       std::cout << s << std::endl;
    }   
}