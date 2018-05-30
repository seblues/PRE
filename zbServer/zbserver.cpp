#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <time.h>

#include "utils.h"
#include "zigbee_api.h"
#include "database.h"

//type of message
#define AT_ANS 0x88
#define NODE_ID 0x95
#define RECV_MSG 0x90
#define TRANS_STAT 0x8b

#define SIZE_OF_ACCEL 2
#define SIZE_OF_LUM 4
#define SIZE_OF_TEMP 2
#define SIZE_OF_DIGIT 3

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

int waitVal = 1;

struct sharedData{
    unsigned char run;
    int fd;
    std::condition_variable cv;
    unsigned char currentMacAddr[8];
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

    static sharedData shData;

    //init dataBase
    int rc = sqlite3_open("./prgm.db", &shData.db);
    initDb(shData.db);

    //init serial communication
    char path[] = "/dev/ttyUSB0";
    shData.fd = initSerial(path);
    shData.run = 1;
    std::thread th_rcv(rcv, &shData);
    std::thread th_snd(snd, &shData);


    while(true){
        std::cin >> waitVal;
        std::cout << "command received" << std::endl;
        if(waitVal == 0){break;}
    }
    shData.run = 0;
    th_rcv.join();
    th_snd.join();
    zigbee_close(shData.fd);

}

int rcv(sharedData* pData){
    Buffer* buffer = nullptr;
    struct timespec ts = {0};
    unsigned char netAddr[2] = {0};
    unsigned char macAddr[8] = {0};
    unsigned int dataNb = 0;
    unsigned int offset = 0;

    unsigned short val_accel = 0;
    float val_lum = 0;
    short val_temp = 0;

    unsigned int type = 0;
    unsigned int id = 0;
    float value = 0;

    while(true){
        buffer = zigbee_read(pData->fd);

        //ignore message if checksum wrong
        if(verifyData(buffer) == -1){
            std::cout << "error on data" << std::endl;
            freeBuffer(buffer);
            continue;
        }

        switch(buffer->ptr[0]){

            case TRANS_STAT:
                switch(buffer->ptr[5]){ //buffer->ptr[5] status field

                    case SUCCESS:
                        //do send ack to snd thread
                        pData->cv.notify_all();
                        std::cout << "success. " << std::endl;
                        break;

                    case ADDR_N_FOUND:
                        deleteModule(pData->db, pData->currentMacAddr);
                        pData->cv.notify_all();
                        break;
                }
                break;

            case RECV_MSG:
                std::cout << "data received." << std::endl;

                //PERSONAL PROTOCOL PART
                offset = 0;
                dataNb = buffer->ptr[12];
                for(unsigned int i=0; i<dataNb; i++){
                    type = buffer->ptr[13 + offset];
                    id = buffer->ptr[14 + offset];
                    switch(type){
                        case ACCEL_X:
                            //DATA TYPE 0 TO COMPLETE
                            val_accel = buffer->ptr[15 + offset] << 8 + buffer->ptr[16 + offset];
                            value = (float)val_accel;
                            offset += SIZE_OF_ACCEL;
                            break;
                        case ACCEL_Y:
                            //DATA TYPE 1 TO COMPLETE
                            val_accel = buffer->ptr[15 + offset] << 8 + buffer->ptr[16 + offset];
                            value = (float)val_accel;
                            offset += SIZE_OF_ACCEL;
                            break;
                        case ACCEL_Z:
                            //DATA TYPE 2 TO COMPLETE
                            val_accel = buffer->ptr[15 + offset] << 8 + buffer->ptr[16 + offset];
                            value = (float)val_accel;
                            offset += SIZE_OF_ACCEL;
                            break;
                        case LUM:
                            //DATA TYPE 3 TO COMPLETE
                            val_lum = (buffer->ptr[15 + offset] << 24 | buffer->ptr[16 + offset] << 16 | buffer->ptr[17 + offset] << 8 | buffer->ptr[18 + offset]);
                            value = (float)val_lum;
                            offset += SIZE_OF_LUM;
                            break;
                        case TEMP:
                            //DATA TYPE 3 TO COMPLETE
                            val_temp = buffer->ptr[15 + offset] << 8 + buffer->ptr[15 + offset];
                            value = (float)val_temp;
                            offset += SIZE_OF_TEMP;
                            break;
                        case DIGIT:
                            //DATA TYPE 3 TO COMPLETE
                            value = buffer->ptr[15 + offset];
                            offset += SIZE_OF_DIGIT;
                            break;
                    }
                    clock_gettime(CLOCK_REALTIME, &ts);
                    insertValue(pData->db, type, pData->currentMacAddr, id, value, ts.tv_sec);
                }
                break;

            case NODE_ID:
                netAddr[0]=buffer->ptr[9];
                netAddr[1]=buffer->ptr[10];
                for (unsigned int i=0; i<8; i++){
                    macAddr[i] = buffer->ptr[1+i];
                }
                insertModule(pData->db, macAddr, netAddr);
                break;
        }
        freeBuffer(buffer);
    }
    std::cout << "quit rcv thread" << std::endl;
    return 0;
}

int snd(sharedData* pData){

    List moduleList = {0};
    Buffer* buffer = newBuffer(10);
    unsigned char netAddr[2] = {0};
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);

    while(true){

        getAllMacAddr(pData->db, &moduleList);
        for (unsigned int i=0; i<moduleList.index; i++){
            memcpy(pData->currentMacAddr, moduleList.values[i], 8);
            getNetAddr(pData->db, pData->currentMacAddr, netAddr);

            std::cout << "sending... " << std::endl;
            sendData(pData->fd, pData->currentMacAddr, netAddr, buffer);
            pData->cv.wait(lock);
        }
        sleep(waitVal);
    }
    return 0;
}



