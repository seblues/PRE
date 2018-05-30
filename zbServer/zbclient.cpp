#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <time.h>
#include <math.h>

#include "utils.h"
#include "zigbee_api.h"
#include "database.h"

//type of message
#define AT_ANS 0x88
#define NODE_ID 0x95
#define RECV_MSG 0x90
#define TRANS_STAT 0x8b

#define SIZE_OF_ACCEL 8
#define SIZE_OF_LUM 8
#define SIZE_OF_TEMP 8
#define SIZE_OF_DIGIT 8

#define OK 0
#define ERROR 1
#define INVALID_CMD 2
#define INVALID_PARAM 3
#define TX_FAIL 4

//broadcast address
#define BC_ADDR {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}

//delivery status for transmit status message
#define SUCCESS 0x00
#define ROUTE_N_FOUND 0x25
#define ADDR_N_FOUND 0x24
#define PAYLOAD_TOO_LARGE 0x74

struct sharedData{
    bool run;
    int fd;
    std::condition_variable cv;
    char currentMacAddr[8];
    sqlite3* db;
};

enum SensorType{
    ACCEL_X = 0,
    ACCEL_Y = 1,
    ACCEL_Z = 2,
    LUM = 3,
    TEMP = 4,
    DIGIT = 5
};

int rcv(sharedData *pData);
int snd(sharedData *pData);

int main(){

    sharedData shData;


    //init serial communication
    char path[] = "/dev/ttyUSB1";
    shData.fd = initSerial(path);

    std::cout << "start rcv thread" << std::endl;
    std::thread th_rcv(rcv, &shData);

    shData.run = true;
    //std::cin >> rc;
    th_rcv.join();

    shData.run = false;
    zigbee_close(shData.fd);

}

int rcv(sharedData* pData){
    Buffer* buffer = nullptr;
    unsigned char netAddr[2] = {0x00, 0x00};
    unsigned char macAddr[8] = {0x00, 0x13, 0xa2, 0x00, 0x40, 0x89, 0xec, 0x1a};
    unsigned int dataNb = 0;
    unsigned int offset = 0;

    unsigned int type = 0;
    unsigned int id = 0;
    unsigned int timestamp = 0;
    float value = 0;

    unsigned int x = 0;

    while(pData->run){
        buffer = zigbee_read(pData->fd);

        //ignore message if checksum wrong
        if(verifyData(buffer) == -1){
            std::cout << "error in msg" << std::endl;
            freeBuffer(buffer);
            continue;
        }

        switch(buffer->ptr[0]){

            case TRANS_STAT:
                switch(buffer->ptr[5]){ //buffer->ptr[5] status field

                    case SUCCESS:
                        std::cout << "success. " << std::endl;
                        break;

                    case ADDR_N_FOUND:
                        std::cout << "error." << std::endl;
                        break;
                }
                break;

            case RECV_MSG:
                std::cout << "msg received" << std::endl;
                Buffer* sendingBuffer = newBuffer(10);

                //number of values to send
                sendingBuffer->ptr[0] = 3;

                sendingBuffer->ptr[1] = 0; //type
                sendingBuffer->ptr[2] = 0; //id
                sendingBuffer->ptr[3] = cos(x++ + 1); //data

                sendingBuffer->ptr[4] = 1;
                sendingBuffer->ptr[5] = 0;
                sendingBuffer->ptr[6] = 2* cos(x++);

                sendingBuffer->ptr[7] = 2;
                sendingBuffer->ptr[8] = 0;
                sendingBuffer->ptr[9] = 3* cos(x++ + 2);

                sendData(pData->fd, macAddr, netAddr, sendingBuffer);
                break;
        }
        freeBuffer(buffer);
    }
    return 0;
}




