// file.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef FILE_H
#define FILE_H

// std incl
#include <stdint.h>
#include <stdio.h>

// read a file. allocates memory for the buffer.
// filename: the absolute path to the file.
// bytesRead: if not NULL, will store the number of bytes read.
// expectedSize: if not 0, will check the file size against this value and return NULL if they don't match.
// returns the buffer if successful, NULL otherwise.
uint8_t* readFileAllocBuffer(const char* filename, uint32_t* bytesRead, const uint32_t expectedSize);

// read a file into an already allocated buffer
// filename: the absolute path to the file.
// bytesRead: if not NULL, will store the number of bytes read.
// expectedSize: if not 0, will check the file size against this value and return NULL if they don't match.
// returns the buffer if successful, NULL otherwise.
int readFileIntoBuffer(const char* filename, uint8_t* buff, uint32_t buffSize, uint32_t* bytesRead, const uint32_t expectedSize);

// write to a file.
// filename: the absolute path to the file.
// ptr: the buffer to write from.
// bytesToWrite: the number of bytes to write to the file.
// returns 0 if successful, 1 otherwise.
int writeFile(const char* filename, void* ptr, const uint32_t bytesToWrite);

// deletes a file.
// filename: the absolute path to the file.
// returns 0 if successful, 1 otherwise.
int deleteFile(const char* filename);

// gets the size of a file.
// file: the file to get the size of.
// fileSize: if not NULL, will store the file size.
// returns 0 if successful, 1 otherwise.
int getFileSize(FILE* file, uint32_t* fileSize);

#endif