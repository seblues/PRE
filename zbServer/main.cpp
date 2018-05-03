#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"
#include "zigbee_api.h"
#include "database.h"

//type of message
#define AT_ANS '\x88'
#define NODE_ID '\x95'
#define RECV_MSG '\x90'
#define TRANS_STAT '\x8B'


#define OK 0
#define ERROR 1
#define INVALID_CMD 2
#define INVALID_PARAM 3
#define TX_FAIL 4

//broadcast address
#define BC_ADDR {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF}

//delivery status for transmit status message
#define SUCCESS 0x00
#define ROUTE_N_FOUND 0x25
#define ADDR_N_FOUND 0x24
#define PAYLOAD_TOO_LARGE 0x74

//states of state machine
#define SEND 0
#define READ 1
#define SELECT 2

struct CmdList{
    char nbOfCmd;
    char** cmdListPtr;
} typedef CmdList;

int cmdRead(int fd, CmdList* cmdList);


int main(int argc, char *argv[]){
	
	//init dataBase
	sqlite3 *db;
    int rc = sqlite3_open("db/prgm.db", &db);
	initDb(db);

	//init serial communication
    char path[] = "/dev/ttyUSB0";
    int fd = initSerial(path);
	fd_set afds;
	fd_set rfds;
	FD_ZERO(&afds);
	FD_SET(fd,&afds);
	FD_SET(0,&afds);
	
	char state=SELECT;
	unsigned int reqToSend=0;
	Buffer* buffer=NULL;
	char macAddr[8];
	char netAddr[2];
	
	//char mac0[8]={0x00,0x13,0xA2,0x00,0x40,0x89,0xEB,0xE1};
	//char mac0[]="020013A2004089EBE10013A2004089EBE1";
	
	char net[2];
	char destMac[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	char destNet[2]={0x00,0x00};
    Buffer* request = newBuffer(2);
	char dataNb=0;
	unsigned int i=0;
	unsigned int k=0;
	
	CmdList cmdList;
    cmdList.nbOfCmd = 0;

	
	while(1){
		switch(state){
			case SELECT:
				rfds=afds;
				select(fd+1,&rfds,NULL,NULL,NULL);
				//if receive command on STDIN
				if(FD_ISSET(0,&rfds)){
					//PROCESS COMMAND...
					cmdRead(0,&cmdList);
                    state = SEND;
				}
				
				//if receive zigbee message
				if(FD_ISSET(fd,&rfds)){
                    state = READ;
				}
				break;
			
			case SEND:
				if((cmdList.nbOfCmd)==0){state=SELECT;}
				else{
					strToBuff(destMac,cmdList.cmdListPtr[(cmdList.nbOfCmd)-1],16);
                    getNetAddr(db, destMac, destNet);
                    sendData(fd, destMac, destNet, request);
                    state = READ;
				}
				break;
			
			case READ:
                buffer = zigbee_read(fd);
				if(verifyData(buffer)==-1){break;} //ignore message if checksum wrong

                switch(buffer->ptr[0]){
					
					case TRANS_STAT:
						switch(buffer->ptr[5]){ //buffer->ptr[5] status field
							
							case SUCCESS:
								//do nothing
								break;
							
							case ADDR_N_FOUND:
								deleteModule(db,destMac);
								(cmdList.nbOfCmd)--;
								state=SEND;
								break;
						}
						break;
					
					case RECV_MSG:
						//STORE DATA... TO COMPLETE
						
						//PERSONAL PROTOCOL PART
                        dataNb = buffer->ptr[12];
						for(i=0;i<dataNb;i++){
							switch(buffer->ptr[k++]){
								case 0:
									//DATA TYPE 0 TO COMPLETE
									k+=1;
									break;
								case 1: 
									//DATA TYPE 1 TO COMPLETE
									k+=2;
									break;
								case 2: 
									//DATA TYPE 2 TO COMPLETE
									k+=3;
									break;
								case 3:
									//DATA TYPE 3 TO COMPLETE
									k+=4;
									break;
							}
						}
						
						(cmdList.nbOfCmd)--;
                        state = SEND;
						break;
					
					case NODE_ID:
						netAddr[0]=buffer->ptr[9];
						netAddr[1]=buffer->ptr[10];
						for (i=0;i<8;i++){
                            macAddr[i] = buffer->ptr[1+i];
						}
						insertModule(db,macAddr,netAddr);
						if((cmdList.nbOfCmd)==0){
                            state = SELECT;
						}
						else{
                            state = READ;
						}
						break;
					
				}
		}
								
	}
	freeBuffer(buffer);
	zigbee_close(fd);
	sqlite3_close(db);
	return 0;
}

int cmdRead(int fd, CmdList* cmdList){
	int nbRead=0;
	char nbOfCmd[2]={0x00,0x00};
	read(0,nbOfCmd,2);
	strToBuff(&(cmdList->nbOfCmd),nbOfCmd,2);
	cmdList->cmdListPtr=(char**)malloc((cmdList->nbOfCmd)*sizeof(char*));
	unsigned int i=0;
	for(i=0;i<cmdList->nbOfCmd;i++){
		cmdList->cmdListPtr[i]=(char*)malloc(16*sizeof(char));
		while(nbRead<16){
			nbRead+=read(0,cmdList->cmdListPtr[i],16);
		}
		nbRead=0;
	}
	return 0;
}


