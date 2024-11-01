// file.c: File handling functions

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>

#include "file.h"
#include "mem_tracking.h"

uint8_t* readFileAllocBuffer(const char* filename, uint32_t* bytesRead, const uint32_t expectedSize)
{
	FILE* file = NULL;
	uint32_t size = 0;

	if (filename == NULL)
		return NULL;

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		printf("Error: could not open file: %s\n", filename);
		return NULL;
	}

	getFileSize(file, &size);

	if (expectedSize != 0 && size != expectedSize)
	{
		printf("Error: invalid file size. Expected %u bytes. Got %u bytes\n", expectedSize, size);
		fclose(file);
		return NULL;
	}

	uint8_t* data = (uint8_t*)malloc(size);
	if (data != NULL)
	{
		fread(data, 1, size, file);
		if (bytesRead != NULL)
		{
			*bytesRead = size;
		}
	}
	fclose(file);

	return data;
}

int readFileIntoBuffer(const char* filename, uint8_t* buff, uint32_t buffSize, uint32_t* bytesRead, const uint32_t expectedSize)
{
	FILE* file = NULL;
	uint32_t size = 0;

	if (filename == NULL)
		return 1;

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		printf("Error: could not open file: %s\n", filename);
		return 1;
	}

	getFileSize(file, &size);

	if (buffSize < size)
	{
		printf("Error: file too big for buffer. Buffer size: %u bytes. Got %u bytes\n", buffSize, size);
		fclose(file);
		return 1;
	}

	if (expectedSize != 0 && size != expectedSize)
	{
		printf("Error: invalid file size. Expected %u bytes. Got %u bytes\n", expectedSize, size);
		fclose(file);
		return 1;
	}

	fread(buff, 1, size, file);
	if (bytesRead != NULL)
	{
		*bytesRead = size;
	}
	fclose(file);
	return 0;
}
int writeFile(const char* filename, void* ptr, const uint32_t bytesToWrite)
{
	FILE* file = NULL;
	uint32_t bytesWritten = 0;

	if (filename == NULL)
		return 1;

	file = fopen(filename, "wb");
	if (file == NULL)
	{
		printf("Error: Could not open file: %s\n", filename);
		return 1;
	}

	bytesWritten = fwrite(ptr, 1, bytesToWrite, file);
	fclose(file);

	return 0;
}
int getFileSize(FILE* file, uint32_t* fileSize)
{
	if (file == NULL)
		return 1;

	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	return 0;
}
int fileExists(const char* filename)
{
	FILE* file = NULL;

	if (filename == NULL)
		return 1;

	file = fopen(filename, "rb");
	if (file == NULL)
		return 1;
	fclose(file);
	return 0;
}
int deleteFile(const char* filename)
{
	if (filename == NULL)
		return 1;

	if (!fileExists(filename))
		return 0;

	if (remove(filename) != 0)
		return 1;

	return 0;
}
