#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

Buffer* newBuffer(unsigned int bufferSize){
    Buffer* newBuffer = (Buffer*)malloc(sizeof(Buffer));
    newBuffer->size = bufferSize;
    newBuffer->ptr = (unsigned char*)malloc(bufferSize);
	return newBuffer;
}

void freeBuffer(Buffer* buffer){
	free(buffer->ptr);
	free(buffer);
}

void printBuffer(Buffer* buffer){
	unsigned int i=0;
	for(i=0;i<buffer->size;i++){
		printf("%02X",buffer->ptr[i]);
	}
	printf("\n");
}

void buffToStr(unsigned char* strDest,unsigned char* bufferSrc, unsigned int size){
	unsigned int i=0;
    for(i = 0; i<size; i++){
        sprintf((char*)(strDest + 2*i), "%02X", bufferSrc[i]); // sizeof(0x11)=1  and sizeof("11")=2 so the str is 2x bigger...
	}
}

void strToBuff(unsigned char* bufferDest, unsigned char* strSrc, unsigned int size){
	unsigned int i=0;
	unsigned int val=0;
	unsigned int j=0;
	for(i=0;i<size;i+=2){
		switch(strSrc[i]){
			case '0': val=0; break;
			case '1': val=1; break;
			case '2': val=2; break;
			case '3': val=3; break;
			case '4': val=4; break;
			case '5': val=5; break;
			case '6': val=6; break;
			case '7': val=7; break;
			case '8': val=8; break;
			case '9': val=9; break;
			case 'A': val=10; break;
			case 'B': val=11; break;
			case 'C': val=12; break;
			case 'D': val=13; break;
			case 'E': val=14; break;
			case 'F': val=15; break;
		}
		bufferDest[j]=16*val;
		switch(strSrc[i+1]){
			case '0': val=0; break;
			case '1': val=1; break;
			case '2': val=2; break;
			case '3': val=3; break;
			case '4': val=4; break;
			case '5': val=5; break;
			case '6': val=6; break;
			case '7': val=7; break;
			case '8': val=8; break;
			case '9': val=9; break;
			case 'A': val=10; break;
			case 'B': val=11; break;
			case 'C': val=12; break;
			case 'D': val=13; break;
			case 'E': val=14; break;
			case 'F': val=15; break;
		}
		bufferDest[j]+=val;
		j++;
	}
}
			
