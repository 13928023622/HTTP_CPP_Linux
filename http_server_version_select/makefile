TARGET = server
FLAG = -Wall -std=c++11 -lpthread
BASE64 = base64


OPENCV_VERSION = 3

ifeq ($(OPENCV_VERSION), 2)
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgproc -lopencv_highgui
else
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_videoio
endif
OPENCV2_HEADER = -I/usr/include/opencv2


$(TARGET): $(TARGET).o httpserver.o $(BASE64).o 
	g++ -o $(TARGET) $(TARGET).o httpserver.o $(BASE64).o $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) 

$(TARGET).o:$(TARGET).cpp 
	g++ -c $(TARGET).cpp  $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) 

httpserver.o:httpserver.cpp
	g++ -c httpserver.cpp $(FLAG)  $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY) 

$(BASE64).o: $(BASE64).cpp
	g++ -c $(BASE64).cpp $(FLAG)

# export FLAG
# myThreadPool.o: $(SUB_DIRS)/myThreadPool.cpp
# 	g++ -c $(SUB_DIRS)/myThreadPool.cpp $(FLAG)


.PHONY : clean
clean:
	-rm *.o $(TARGET)