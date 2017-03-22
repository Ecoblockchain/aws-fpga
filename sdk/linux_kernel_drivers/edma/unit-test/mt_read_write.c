#include<pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (1024 *4)
#endif

#ifndef SIZE_OF_DATA
#error "Please define SIZE_OF_DATA"
#endif

#ifndef CHUNK_SIZE
#error "Please define CHUNK_SIZE"
#endif
/*
#define  SIZE_OF_DATA (PAGE_SIZE * 100000)
#define CHUNK_SIZE (PAGE_SIZE)
 */

char* write_buf;
char* read_buf;
long long int write_index;
long long int read_index;

off_t written_no_fsync;
off_t can_read;


pthread_t write_tid;
pthread_t read_tid;
pthread_t fsync_tid;

bool read_done = false;

static char *rand_string(char *str, size_t size)
{
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRTSUVWXYZ1234567890";
	int i;

	for(i = 0; i < size; i++){
		int key = rand() % (int) (sizeof charset - 1);
		str[i] = charset[key];
	}

	str[size-1] = '\0';

	return str;
}

pthread_mutex_t lock;


void* doWrite(void *arg)
{

	int fd = *((int*)arg);
	unsigned long long int size_of_data = SIZE_OF_DATA;
	off_t offset = 0x010000000;
	int ret;

	char *srcBuf = (char*)malloc(sizeof(char) * CHUNK_SIZE);

	while(size_of_data > 0)
	{
		/*write a page to 0x100000000*/
		int sleep_time = rand() % 200;
		int write_size = size_of_data < CHUNK_SIZE ? size_of_data : rand() % CHUNK_SIZE;

		rand_string(srcBuf, CHUNK_SIZE);

		pthread_mutex_lock(&lock);

		ret = lseek(fd, offset, SEEK_SET);
		if(ret < 0)
			exit(3);

		printf("----> %s\nwriting %u bytes\n", __func__, write_size);
		ret = write(fd, srcBuf, write_size);
		printf("wrote %u bytes\n", ret);

		if(ret != write_size)
			exit(1);

		offset += ret;
		written_no_fsync += ret;

		memcpy(write_buf + write_index, srcBuf, ret);
		write_index += ret;

		pthread_mutex_unlock(&lock);

		size_of_data -= ret;

		/*        printf("%s going to sleep for %d\n", __func__, sleep_time);*/

		usleep(sleep_time);
	}

	free(srcBuf);

}

void* doRead(void *arg)
{
	int fd = *((int*)arg);
	unsigned long long int size_of_data = SIZE_OF_DATA;
	int ret;
	off_t offset = 0x010000000;

	char* dstBuf = (char*)malloc(sizeof(char) * CHUNK_SIZE);

	while(size_of_data > 0)
	{
		int sleep_time = rand() % 200;
		/*int read_size = size_of_data < CHUNK_SIZE ? size_of_data : rand() % CHUNK_SIZE;*/
		int read_size;
		int i;
		while(!can_read)
			usleep(200);

		pthread_mutex_lock(&lock);

		read_size = can_read == 1 ? 1: rand() % can_read;

		ret = lseek(fd, offset, SEEK_SET);
		if(ret < 0)
			exit(3);

		printf("\n---> %s\nTryting to read %d bytes out of can_read %lld \n", __func__, read_size, can_read);
		ret = read(fd, dstBuf, read_size);
		printf("\nRead %d bytes \n", ret, size_of_data);

		if(ret != read_size)
			exit(2);

		offset += ret;

		can_read -= ret;

		memcpy(read_buf + read_index, dstBuf, ret);

		printf("reading to buf at offsed %lld size is %d\n", read_index, ret);
		printf("\nData read is:\n");
		for(i = 0; i < ret; i++)
		{
			printf("%c", dstBuf[i]);
		}
		printf("\n");

		read_index += ret;

		size_of_data -= ret;

		if(memcmp(write_buf, read_buf, SIZE_OF_DATA - size_of_data)){
			int i = 0;

			printf("\nData written is:\n");
			for(i = 0; i < SIZE_OF_DATA - size_of_data; i++)
			{
				if(i % 90 == 0)
					printf("\n");

				printf("%c", write_buf[i]);
			}
			printf("\n");

			printf("\nData read is:\n");
			for(i = 0; i < SIZE_OF_DATA - size_of_data; i++)
			{
				if(i % 90 == 0)
					printf("\n");

				/*make it easier for debug*/
				printf("%c", read_buf[i]);
			}
			printf("\n");

			exit(4);
		}

		/*        printf("%s going to sleep for %d\n", __func__, sleep_time);*/
		pthread_mutex_unlock(&lock);

		usleep(sleep_time);
	}

	pthread_mutex_lock(&lock);
	read_done = true;
	pthread_mutex_unlock(&lock);
}


void* doFsync(void *arg)
{
	int fd = *((int*)arg);
	int sleep_time = rand() % 200;
	bool read_lock = false;

	while(!read_lock)
	{

		pthread_mutex_lock(&lock);
		read_lock = read_done;
		fsync(fd);
		printf("\n---> %s\nFsyncing %lld can read was %lld and now it is", __func__, written_no_fsync, can_read);
		can_read += written_no_fsync;
		printf("%lld\n", can_read);
		written_no_fsync = 0;
		pthread_mutex_unlock(&lock);

		usleep(sleep_time);
	}
}


int main()
{

	int fd, i;
	int ret = 0;
	srand ( time(NULL) );

	write_buf = (char*)malloc(sizeof(char) * SIZE_OF_DATA);
	if(!write_buf) {
		ret = -ENOMEM;
		goto done;
	}

	read_buf = (char*)malloc(sizeof(char) * SIZE_OF_DATA);
	if(!read_buf) {
		free(write_buf);
		ret = -ENOMEM;
		goto done;
	}


	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		goto done;;
	}

	if((fd = open("/dev/edma0_queue_0",O_RDWR)) == -1){
		perror("open failed");
		return -1;
	}

	printf("create thread :[%s]\n", "write");
	ret = pthread_create(&write_tid, NULL, &doWrite, &fd);
	if (ret != 0)
	{
		printf("\ncan't create thread :[%s]", strerror(ret));
		goto done;
	}

	printf("create thread :[%s]\n", "read");
	ret = pthread_create(&fsync_tid, NULL, &doFsync, &fd);
	if (ret != 0)
	{
		printf("\ncan't create thread :[%s]", strerror(ret));
		goto done;
	}

	printf("create thread :[%s]\n", "fsync");
	ret = pthread_create(&read_tid, NULL, &doRead, &fd);
	if (ret != 0)
	{
		printf("\ncan't create thread :[%s]", strerror(ret));
		goto done;
	}

	pthread_join(write_tid, NULL);
	pthread_join(fsync_tid, NULL);
	pthread_join(read_tid, NULL);

	if(!memcmp(write_buf, read_buf, SIZE_OF_DATA)){
		printf("The string written and the string read are identicle!\n");
	} else {
		int i = 0;

		printf("\nData written is:\n");
		for(i = 0; i < write_index; i++)
		{
			if(i % 90 == 0)
				printf("\n");

			printf("%c", write_buf[i]);
		}
		printf("\n");

		printf("\nData read is:\n");
		for(i = 0; i < read_index; i++)
		{
			if(i % 90 == 0)
				printf("\n");

			/*make it easier for debug*/
			printf("%c", read_buf[i]);
		}
		printf("\n");

		ret = -1;
	}

done:
	free(write_buf);
	free(read_buf);

	return ret;
}

