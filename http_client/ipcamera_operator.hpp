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

#ifndef IPCAMERA_OPERATOR
#define IPCAMERA_OPERATOR

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace gz_ag {
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
    //par: Ip address of one camera that should be used
    std::string _getDeviceCommand(std::string device_info, std::string port="554");

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
    //success or fail to open cam.
    bool _openCamera(cv::VideoCapture capture);



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
    // take a image for trash.
    cv::Mat _getOneImg(cv::VideoCapture capture);

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
    //take a image for trash.
    std::vector<cv::Mat> _getSeveralImg(cv::VideoCapture capture, int pic_num);
}
#endif // IPCAMERA_OPERATOR