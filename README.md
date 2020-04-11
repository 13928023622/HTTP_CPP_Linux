# HTTP_server_client_c-

使用常规的网络编程函数写了个简单的Server端和Client端，用到的网络编程函数有：
socket函数
int socket(int protofamily, int type, int protocol) //返回的是sockfd，socket()用于创建sock描述符，唯一表示这个socket。
` protofamily：协议域，常用的有AF_INET(ipv4)，AF_INET6(ipv6)，AF_LOCAL（或称AF_UNIX，Unix域socket）、AF_ROUTE等等。协议域决定了socket的地址类型，在通信中必须采用对应的地址。如AF_INET必须使用ipv4地址（32位）和端口号（16位）的组合，AF_UNIX决定了要用一个决定路径作为地址。
` type: 指定Socket的类型，常用类型有SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_PACKET, SOCK_SEQPACK等等。
` protocol: 协议，常用协议有，IPPROTO_TCP, IPPROTO_UDP, IPPROTO_SCTP, IPPROTO_TIPC等，分别对应tcp协议，udp协议，sctp协议，tipc协议。

注意：当调用socket函数时，是创建了一个没有具体地址的Socket。如果要给地址赋值，则必须要使用bind函数去分配一个端口。否则当调用listen和connect的时候系统会自动分配窗口。

bind函数
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen); //将一串地址赋给sockfd对应的socket
` sockfd: socket描述符， 通过socket()创建，唯一标识了socket。
` *addr：一个const struct sockaddr 的指针，指向要绑定给该sockfd的地址，该地址根据创建socket时协议的不同而改变。
如ipv4如下：
    struct sockaddr_in{
        sa_family_t sin_family; /* address family: AF_INET */
        in_port_t   sin_port;   /* port in network byte order */
        struct in_addr sin_addr;/* internet address */
    };
    /* internet address */
    struct in_addr{
        uint32_t s_addr; /* address in network byte order */
    };

如ipv6如下：
    struct sockaddr_in6{
        sa_family_t sin6_family; /* AF_INET6 */
        in_port_t   sin6_port;   /* port number */
        uint32_t    sin6_flowinfo; /* IPv6 flow information */
        struct      in6_addr;    /* IPv6 address */
        uint32_t    sin6_scope_id; /* Scope ID */
    };
    struct in6_addr{
        unsigned char s6_addr[16]; /* IPv6 address */
    }

Unix域（AF_LOCAL）
    #define UNIX_PATH_MAX 108
    
    struct sockaddr_un{
        sa_family_t sun_family; /* AF_UNIX */  
        char        sun_path[UNIX_PATH_MAX]; /* pathname */
    };

` addrlen: 对应的是地址的长度。

注意：通常服务器启动的时候会绑定一个总所周知的地址（ip地址+端口号），用于提供服务，客户就可以通过它来连接服务器；而客户端就不需要使用bind来绑定了，因为使用connect连接服务器的时候，系统会自动分配一个端口号，并且和自身的ip地址进行组合。这就是为什么通过服务器在listen之前会使用一个bind函数，而客户端就不需要调用，而是用connect()在系统中自动生成一个。

！！重点
主机字节序
！主机字节序就是我们常说的大端和小端模式：不同的CPU有不同的字节序类型，这些字节是指整数在内存中保存的顺序。这个叫主机序。
        a） (Little-Endian)小端模式：低位字节排放在内存的低地址端，高位字节排放在内存的高地址端。
        b） (Big-Endian)   大端模式：高位字节排放在内存的低地址端，低位字节排放在内存的高地址端。


上图的（1）中，低位地址78,放在了高地址空间，所以（1）是大端
而上图的（2）中，低位地78放在了低地址空间，所以（2）是小端
！！ 重点
网络字节序
！4个字节的32bit以下面的次序传输，首先是0-7bit，其次8-15bit，然后16-24bit，最后是24-31bit。这种传输次序称作大端字节序。由于TCP/IP首部中所有的二进制
整数在网络中传输时都要求以这种次序，因此它又被称作网络字节序。

注意：在将一个地址绑定到socket的时候，请先将主机字节序转换成网络字节序，而不要假定主机字节序和网络字节序一样都是大端模式。务必！bind的时候要将主机字节序转换为网络字节序。

listen(),connet()函数
作为一个服务器，在调用socket()，bind()之后会调用listen()来监听这个Socket，如果这个客户端这时调用connect()发出连接请求，服务端就会收到这个请求。
int listen(int sockfd, int backlog);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

listen()函数中，第一个参数为要监听的socket描述符，第二个参数为相应socket可以排队的最大连接个数。socket()函数创建的socket默认是一个主动类型，listen()函数将socket变为被动类型，等待客户端的连接请求。
connect()函数的第一个参数为客户端的socket对象的fd，第二个参数为准备请求的服务器的socket地址，第三个参数为socket地址的长度。注：客户端通过connect函数来建立与服务器的TCP连接。连接成功则返回0，失败返回-1


accept()函数
1. 服务器端以此执行了socket(), bind(), listen()之后，就会监听指定的socket地址。
2. 客户端依次调用socket()，connect()之后就向服务器发送连接请求。
3. 服务端监听到客户端的请求后，会调用accept()接收请求，这样连接就建立好了。之后就可以开始网络I/O操作，类同于普通文件的I/O操作。
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); //返回连接connect_fd,该套接字是连接套接字
注意：这里有监听套接字和连接套接字
（1）监听套接字：监听套接字正如accept的参数sockfd，这是监听套接字，是服务器开始调用socket()函数时创建的。
（2）连接套接字：一个套接字会从主动连接的套接字变为一个监听套接字；而accept函数返回的是已连接的Socket描述字，它代表着一个网络已经存在的点点连接。
一个服务器通常只创建一个监听socket套接字，在服务器的生命周期中一直存在。同时内核为每一个连接的客户进程提供了已连接Socket的文件描述符，当服务器完成对某个客户的服务时，会关闭这个已连接的Socket，包括其socket的fd。


read(), write()函数
网络的IO操作主要有以下几组：
` read()/write()
` recv()/send()
` readv()/writev()
` recvmsg()/sendmsg()
` recvfrom()/sendto()


       #include <unistd.h>

       ssize_t read(int fd, void *buf, size_t count);
       ssize_t write(int fd, const void *buf, size_t count);

       #include <sys/types.h>
       #include <sys/socket.h>

         ssize_t send(int sockfd, const void *buf, size_t len, int flags);
        ssize_t recv(int sockfd, void *buf, size_t len, int flags);
        
        ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
        ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
        
        ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
        ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);





close函数
在服务端和客户端建立连接之后，会进行一些读写操作，读写结束后需要关闭相应的已连接套接字。
int close(int fd);
注意：close操作只是使相应的Socket描述字的引用计数-1，只有当引用计数为0的时候，才会触发客户端向服务器发送终止连接的请求。
