TOP_DIR := $(shell pwd)
#APP = $(TOP_DIR)x264_test
APP = x264_test

#CC = arm-none-linux-gnueabi-gcc
CC = gcc
CFLAGS = -g 
LIBS = -lpthread -lx264 -lm -lrtmp
#LIBS = -lpthread 
#DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = main.o h264encoder.o 

all:
	g++ -g -o $(APP) camera_capture.cpp h264encoder.c main.cpp librtmp_send264.cpp  $(LIBS) -L/usr/local/lib

clean:
	rm -f *.o a.out $(APP) core *~ ./out/*


