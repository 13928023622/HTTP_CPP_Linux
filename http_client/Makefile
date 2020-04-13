# target = test
target = tes2
FLAGS = -Wall -std=c++11

OPENCV_VERSION = 3

ifeq ($(OPENCV_VERSION), 2)
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgproc -lopencv_highgui
else
	OPENCV2_DEPENDENCY = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_videoio
endif
OPENCV2_HEADER = -I/usr/include/opencv2


# OPENCV_HEADER = -I/usr/local/opencv
# OPENCV_DEPENDENCY = 

$(target): $(target).o http_connector.o ipcamera_operator.o base64.o
	g++ -o $(target) $(target).o http_connector.o ipcamera_operator.o base64.o $(B64_LIB) $(FLAGS) $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY)

$(target).o: $(target).cpp
	g++ -c $(target).cpp $(FLAGS)

include 3rdparty/littlstar/b64.make

http_connector.o: $(B64_LIB) http_connector.cpp http_connector.hpp base64.o
	g++ -c http_connector.cpp http_connector.hpp base64.o -l$(B64_NAME) -I$(OPENCV2_PATH) $(FLAGS)


ipcamera_operator.o: ipcamera_operator.cpp ipcamera_operator.hpp 
	g++ -c ipcamera_operator.cpp ipcamera_operator.hpp $(FLAGS) $(OPENCV2_HEADER) $(OPENCV2_DEPENDENCY)

base64.o: base64.cpp
	g++ -c base64.cpp $(FLAGS)

OUTPUT_OBJ = $(target) libb64.a $(target).o decode.o encode.o http_connector.o ipcamera_operator.o http_connector.hpp.gch ipcamera_operator.hpp.gch
D_OBJ = $(foreach obj, $(OUTPUT_OBJ), $(strip $(wildcard $(obj))))

.PHONY : clean
clean:
# ifeq ($(wildcard $(target)), $(target))
# 	rm $(target)
# endif
ifneq ($(D_OBJ),"")
	-rm -f $(D_OBJ)
else
	echo 'All outputing objects has been cleaned.'
endif