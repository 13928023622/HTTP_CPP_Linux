// 创建者：陈ziliu
// 创建时间：2019-10-25
// ------------------------------------------
// 修改者：肖yao
// 修改时间：2019-10-29
// 修改内容：
//    添加注释
// ------------------------------------------
// 检测传入的网络摄像头的URL地址是否正确，补全摄像
// 头的RTSP地址，通过RTSP地址打开摄像头进行拍照。
// 本段代码需要c++11以及opencv相关的库文件支持


#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<regex>

#include "ipcamera_operator.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

std::string gz_ag::_getDeviceCommand(std::string device_info, std::string port)//par: Ip address of one camera that should be used 
{
    /*
    函数名： _getDeviceCommand
    功能： 检测URL地址是否合法，并补全RTSP地址。（注意：不同品牌网络摄像头的地址可能不同，需查询）
    输入参数：
    ----------------
        device_info： string
            网络摄像头的URL地址，例如：192.168.1.10
    返回值：
    ----------------
        rtsp： string
            当URL地址合法时，返回网络摄像头的RTSP地址。
    */
    try{
        if(!device_info.empty())
        {
            // 正则化表达式判断URL地址是否合法
            std::regex ip_regex("((25[0-5]|2[0-4]\\d|((1\\d{2})|([1-9]?\\d)))\\.){3}(25[0-5]|2[0-4]\\d|((1\\d{2})|([1-9]?\\d)))");
            if(std::regex_match(device_info,ip_regex)){     //input match or not IP formation by using regex
                // 补全RTSP地址
                std::string title = "rtsp://admin:@";
                std::string tail  = "/h264/ch1/main/av_stream";
                // std::string rtsp = title.append(device_info);
                // rtsp.append(tail);
                std::string rtsp = title + device_info + ":" + port + tail;
                sleep(3); //waiting x sec the device to stabilize
                return rtsp;  //return rtsp address 
            }else{
                throw std::string("ERROR 101:input is not match ip formation!");
            }
            
        }else{
            throw std::string("ERROR 102:can't get the correct IP address!");
        }
    }
    // 打印异常信息
    catch(std::string s){
        std::cout<<s<<std::endl;
        return NULL;
    }
}

bool gz_ag::_openCamera(cv::VideoCapture capture) //success or fail to open cam.
{   
    /*
    函数名： _openCamera
    功能： 判断摄像头是否存在异常。
    输入参数：
    ----------------
        capture： VideoCapture
            摄像头相关参数
    返回值：
    ----------------
        bool
            真/假，当摄像头打开成功时返回真，否则假。
    */ 

    // capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    // capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    // sleep(3); //sleep 5 sec 
    try{
        if ( !capture.isOpened()){ //Exit when failing to open camera.
            // cout << "fail to open!" << endl;
            throw std::string("ERROR 103:fail to open camera!");
        }
        // 如果能获取到图像的分辨率信息则表明摄像头打开成功。
        // this device is ok if we can get the relate pars of video.
        long video_h = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
        long video_w = capture.get(CV_CAP_PROP_FRAME_WIDTH);
        if (video_h==0|| video_w==0){
            throw std::string("ERROR 104:camera has some situation!");
        }
    }
    // 打印异常信息
    catch(std::string s){
        std::cout<< s << std::endl;
        return false;
    }
    return true;
}

cv::Mat gz_ag::_getOneImg(cv::VideoCapture capture) // take a image for trash.
{
    /*
    函数名： _getOneImg
    功能： 获取单张图像
    输入参数：
    ----------------
        capture： VideoCapture
            摄像头相关参数
    返回值：
    ----------------
        frame: Mat
            图像
    */
    
    cv::Mat frame, dstImg;
    try{
        // 判断摄像头是否存在异常
        bool flag = _openCamera(capture);
        if (flag == true){
                // namedWindow("wind", CV_WINDOW_NORMAL);
                // setWindowProperty("wind", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
                // capture.set(CV_CAP_PROP_FRAME_WIDTH, 1024);  //set the size of output image
                // capture.set(CV_CAP_PROP_FRAME_WIDTH, 768);
                
                // 修改视频编码格式为MJPG
                capture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
                capture >> frame;
                
                if (frame.empty()){
                    throw std::string("ERROR 105:can't get the frame information from camera!");
                }else{
                    // #pragma region 
                    // cv::imshow("Video0", frame);     //here, the type of parameter should be Mat.
                    // imshow("Screen",frame);
                    // waitKey(5000);
                    // imwrite("1.jpg", frame);     //save mat to img.jpg 
                    // #pragma endregion

                    // 由于摄像机一次会传回多帧图像，这里多获取几次尽量读取最新的那一帧
                    capture>>frame; 
                    capture>>frame; 
                    capture>>frame;
                    capture>>frame;  //read latest frame
                    resize(frame, dstImg, cv::Size(1024, 768));// resize current frame and save to dstImg
                    frame=dstImg;
                    // imshow("wind",dstImg);  //show this frame
                    // waitKey(30);
                }
        }
    }
    // 打印异常信息
    catch(std::string s){
        std::cout<< s << std::endl;
        return frame;
    }
    return frame;
}

std::vector<cv::Mat> gz_ag::_getSeveralImg(cv::VideoCapture capture, int pic_num) //take a image for trash.
{
    /*
    函数名： _getThreeImg
    功能： 获取多张图像，图像的数量由输入参数控制
    输入参数：
    ----------------
        capture： VideoCapture
            摄像头相关参数
        pic_num： int
            定义要读取的图像的数量
    返回值：
    ----------------
        frame: Mat
            图像
    */
    std::vector<cv::Mat> mat_buffer;
    cv::Mat frame, dstImg;
    try{
        // 判断摄像头是否存在异常
        bool flag = gz_ag::_openCamera(capture);
        if (flag == true){
                // namedWindow("wind", CV_WINDOW_NORMAL);
                // setWindowProperty("wind", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
                // capture.set(CV_CAP_PROP_FRAME_WIDTH, 1024);  //set the size of output image
                // capture.set(CV_CAP_PROP_FRAME_WIDTH, 768);
                std::cout << "[INFO]: Camera has opened" << std::endl;
                // 修改视频编码格式为MJPG
                capture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
                capture >> frame;
                if (frame.empty()){
                    throw std::string("ERROR 106:can't get the frame information from camera!");
                }else{
                    // #pragma useless 
                    // cv::imshow("Video0", frame);     //here, the type of parameter should be Mat.
                    // imshow("Screen",frame);
                    // waitKey(5000);

                    // imwrite("1.jpg", frame);     //save mat to img.jpg                                                        
                    // #pragma endregion
                    for (int i=0;i<pic_num;i++){
                        
                        //===========================试图不同帧数的图片
                        // 由于摄像机一次会传回多帧图像，这里多获取几次尽量读取最新的那一帧
                        capture>>frame; 
                        capture>>frame; 
                        capture>>frame; 
                        capture>>frame; 
                        capture>>frame; //read the latest frame
                        cv::resize(frame, dstImg, cv::Size(1024, 768));// resize current frame and save to dstImg                        
                        // Mat tmp = Mat::zeros(dstImg);
                        mat_buffer.push_back(dstImg.clone());//deep copy
                    }
                    // capture>>frame; 
                    // resize(frame, dstImg, Size(1024, 768));// resize current frame and save to dstImg
                    // frame=dstImg;
                }
        }
    }
    // 打印异常信息
    catch(std::string s){
        std::cout<< s << std::endl;
        // return frame;
    }
    return mat_buffer;
}