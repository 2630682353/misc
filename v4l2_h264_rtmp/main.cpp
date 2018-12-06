#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "librtmp_send264.h"  

extern void capture_init();
int pipefd[2];
  
  
FILE *fp_send1;  
  
//读文件的回调函数  
//we use this callback function to read data from buffer  
int read_buffer1(unsigned char *buf, int buf_size ){  
    if(1){  
        int true_size=read(pipefd[0],buf,buf_size);  
        return true_size;  
    }else{  
        return -1;  
    }  
}  

static void *thread_func(void *arg)
{
     capture_init();
}
  
int main(int argc, char* argv[])  
{  
	 if (pipe(pipefd) < 0) {  
        printf("pipe error\n");  
    }  
	pthread_t pth1;

   	
//    fp_send1 = fopen("test.h264", "rb");  
  
    //初始化并连接到服务器  
    RTMP264_Connect("rtmp://192.168.20.120/live/test");  
	pthread_create(&pth1, NULL, thread_func, NULL);
      printf("befor rtmp send\n");
    //发送  
    RTMP264_Send(read_buffer1);  
  
    //断开连接并释放相关资源  
    RTMP264_Close();  
  
    return 0;  
}  