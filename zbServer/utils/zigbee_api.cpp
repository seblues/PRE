#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include "zigbee_api.h"

Buffer* zigbee_read(int fd){
    char header[3] = {0x00, 0x00, 0x00};
    unsigned int nb = read(fd, header, 3);
    unsigned short bufferSize = (unsigned short)((header[1]<<8) + header[2]);
    Buffer* buffer = newBuffer(bufferSize + 1);// +1 ->checksum
    nb = 0;
    while(nb != buffer->size){
         nb += read(fd, (buffer->ptr) + nb, (buffer->size) - nb);
	}
	return buffer;
}

int sendData(int fd, const char* macAddr, const char* netAddr, const Buffer* dataBuffer){
    unsigned int i = 0;
    Buffer* buffer = newBuffer(18 + dataBuffer->size);
    unsigned int length = 14 + dataBuffer->size; //14=17-3 -> header
    buffer->ptr[0] = 0x7E;
    buffer->ptr[1] = (char)(length>>8);
    buffer->ptr[2] = (char)(length);
    buffer->ptr[3] = 0x10;
    buffer->ptr[4] = 0x01;// ?
    for(i = 0; i<8; i++){
        buffer->ptr[5+i] = macAddr[i];
	}
    buffer->ptr[13] = netAddr[0];
    buffer->ptr[14] = netAddr[1];
    buffer->ptr[15] = 0x00;
	//data copy
    for(i=0; i<dataBuffer->size; i++){
        buffer->ptr[16+i] = dataBuffer->ptr[i];
	}
	//checksum
    buffer->ptr[17 + dataBuffer->size] = 0xFF;
    for(i=3; i< 17 + dataBuffer->size; i++){
        buffer->ptr[17 + dataBuffer->size] -= buffer->ptr[i];
	}
    //printBuffer(buffer);
	write(fd, buffer->ptr, buffer->size);
	freeBuffer(buffer);
	return 0;
}

int sendCmd(int fd, char* cmd, Buffer* data){
    unsigned int i = 0;
    unsigned int dataSize = 0;
	if(data){
        dataSize = data->size;
	}
    Buffer* buffer = newBuffer(8 + dataSize);
    unsigned int length = 4 + dataSize; //14=17-3 -> header
    buffer->ptr[0] = 0x7E;
    buffer->ptr[1] = (char)(length>>8);
    buffer->ptr[2] = (char)(length);
    buffer->ptr[3] = 0x08;
    buffer->ptr[4] = 0x52;// ?
    buffer->ptr[5] = cmd[0];
    buffer->ptr[6] = cmd[1];
	//data copy
    for(i=0; i<dataSize; i++){
        buffer->ptr[7 + i] = data->ptr[i];
	}
	//checksum
    buffer->ptr[7 + dataSize] = 0xFF;
    for(i=3; i< 7 + dataSize; i++){
        buffer->ptr[7+dataSize] -= buffer->ptr[i];
	}
	write(fd, buffer->ptr, buffer->size);
	freeBuffer(buffer);
	return 0;
}

int initSerial(char* path){
	int fd;
	struct termios options;

	/* open the port */
	fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
	fcntl(fd, F_SETFL, 0);
	
	/* get the current options */
	tcgetattr(fd, &options);
	
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	
	/* set raw input, 1 second timeout */
	options.c_cflag     |= (CLOCAL | CREAD);
	options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag     &= ~OPOST;
	options.c_cc[VMIN]  = 3;
	options.c_cc[VTIME] = 0;
	
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CRTSCTS;
	options.c_iflag &= ~(IXOFF | IXON);
	
	/* set the options */
	tcsetattr(fd, TCSANOW, &options);
	return fd;
}

int zigbee_close(int fd){
	close(fd);
}

int verifyData(Buffer* buff){
	unsigned int i=0;
	char checksum=0;
	for(i=0; i<(buff->size)-1; i++){ //sum without checksum
		checksum+=buff->ptr[i];
	}
	checksum=0xFF-checksum;
	if(checksum==buff->ptr[(buff->size)-1]){return 0;}
	return -1;
}
