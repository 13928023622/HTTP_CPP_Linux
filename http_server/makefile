TARGET = server
FLAG = -Wall -std=c++11


OPENCV_VERSION = 3

ifeq ($(OPENCV_VERSION), 2)
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgproc -lopencv_highgui
else
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_videoio
endif
OPENCV2_HEADER = -I/usr/include/opencv2



include 3rdparty/littlstar/b64.make
$(TARGET): $(TARGET).o httpserver.o
	g++ -o $(TARGET) $(TARGET).o httpserver.o $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) $(B64_LIB)

$(TARGET).o:$(TARGET).cpp 
	g++ -c $(TARGET).cpp  $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) $(B64_LIB)

httpserver.o:httpserver.cpp
	g++ -c httpserver.cpp $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) $(B64_LIB)

clean:
	rm *.o