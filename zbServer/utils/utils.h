#ifndef UTILS_H
#define UTILS_H

#define STR_SIZE_MAX 50

struct Buffer{
	unsigned int size;
    unsigned char* ptr;
} typedef Buffer;

struct List{
    unsigned char values[10][STR_SIZE_MAX];
    unsigned char index;
};

//alloc memory and return a pointer to this memory
Buffer* newBuffer(unsigned int bufferSize);

//free the memory pointed by buffer
void freeBuffer(Buffer* buffer);

//print the content of the buffer
void printBuffer(Buffer* buff);

//convert a buffer to str, size param must be the size of source buffer   
void buffToStr(unsigned char* dest, unsigned char* src, unsigned int size);

//convert a str to buffer, size param must be the size of source string
void strToBuff(unsigned char* bufferDest, unsigned char* strSrc, unsigned int size);
#endif
