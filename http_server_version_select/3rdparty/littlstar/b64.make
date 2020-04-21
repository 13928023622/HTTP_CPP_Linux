CFLAGS =  -Wall
TEST_CFLAGS = -Ideps
CUSTOM_MALLOC_FLAGS = -Db64_malloc=custom_malloc -Db64_realloc=custom_realloc

B64_NAME = b64
B64_LIB = libb64.a

ifeq ($(USE_CUSTOM_MALLOC), true)
CFLAGS += $(CUSTOM_MALLOC_FLAGS)
endif

$(B64_LIB): encode.o decode.o
	ar -rcs lib$(B64_NAME).a encode.o decode.o

encode.o: 3rdparty/littlstar/encode.cpp
	g++ -c 3rdparty/littlstar/encode.cpp $(CFLAGS)

decode.o: 3rdparty/littlstar/decode.cpp
	g++ -c 3rdparty/littlstar/decode.cpp $(CFLAGS)