#ifndef ZIGBEE_API_H
#define ZIGBEE_API_H

#include "utils.h"

int sendData(int fd,const char* macAddr, const char* netAddr, const Buffer* dataBuffer);
int sendCmd(int fd, char* cmd, Buffer* data);
int initSerial(char* path);
int zigbee_close(int fd);
int verifyData(Buffer* buff);
Buffer* zigbee_read(int fd);

#endif
