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
#define AT_ANS '\x88'
#define NODE_ID '\x95'
#define RECV_MSG '\x90'
#define TRANS_STAT '\x8B'

#define SIZE_OF_ACCEL 3
#define SIZE_OF_LUM 3
#define SIZE_OF_TEMP 3
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

    //init dataBase
    int rc = sqlite3_open("./prgm.db", &shData.db);
    initDb(shData.db);

    //init serial communication
    char path[] = "/dev/ttyUSB0";
    shData.fd = initSerial(path);

    std::thread th_rcv(rcv, &shData);
    std::thread th_snd(snd, &shData);

    shData.run = true;
    //std::cin >> rc;
    th_rcv.join();

    shData.run = false;
    zigbee_close(shData.fd);

}

int rcv(sharedData* pData){
    Buffer* buffer = nullptr;
    struct timespec ts = {0};
    char netAddr[2] = {0};
    char macAddr[8] = {0};
    unsigned int dataNb = 0;
    unsigned int offset = 0;

    unsigned int type = 0;
    unsigned int id = 0;
    unsigned int timestamp = 0;
    float value = 0;

    while(pData->run){
        buffer = zigbee_read(pData->fd);

        //ignore message if checksum wrong
        if(verifyData(buffer) == -1){
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
                dataNb = buffer->ptr[12];
                for(unsigned int i=0; i<dataNb; i++){
                    type = buffer->ptr[13 + offset];
                    switch(type){
                        case ACCEL_X:
                            //DATA TYPE 0 TO COMPLETE
                            offset += SIZE_OF_ACCEL;
                            break;
                        case ACCEL_Y:
                            //DATA TYPE 1 TO COMPLETE
                            offset += SIZE_OF_ACCEL;
                            break;
                        case ACCEL_Z:
                            //DATA TYPE 2 TO COMPLETE
                            offset += SIZE_OF_ACCEL;
                            break;
                        case LUM:
                            //DATA TYPE 3 TO COMPLETE
                            offset += SIZE_OF_LUM;
                            break;
                        case TEMP:
                            //DATA TYPE 3 TO COMPLETE
                            offset += SIZE_OF_TEMP;
                            break;
                        case DIGIT:
                            //DATA TYPE 3 TO COMPLETE
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
    return 0;
}

int snd(sharedData* pData){

    List moduleList = {0};
    Buffer* buffer = newBuffer(10);
    char netAddr[2] = {0};
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);

    while(pData->run){

        getAllMacAddr(pData->db, &moduleList);
        for (unsigned int i=0; i<moduleList.index; i++){
            memcpy(pData->currentMacAddr, moduleList.values[i], 8);
            getNetAddr(pData->db, pData->currentMacAddr, netAddr);

            std::cout << "sending... " << std::endl;
            sendData(pData->fd, pData->currentMacAddr, netAddr, buffer);
            pData->cv.wait(lock);
        }
        sleep(5);
    }
    return 0;
}



