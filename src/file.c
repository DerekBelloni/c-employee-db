#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "common.h"

int create_db_file(char *filename) {
    // Check first to see if the file exists, read only (dont create it)
    int fd = open(filename, O_RDWR);
    // If the file exists, close the file descriptor
    // Call open_db_file
    if (fd != -1) {
        close(fd);
        printf("File already exists\n");
        return STATUS_ERROR;
    } 
    
    fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return STATUS_ERROR;
    }
    return fd;
}

int open_db_file(char *filename) {
    int fd = open(filename, O_RDWR, 0644);

    if (fd == -1) {
        perror("open");
        return STATUS_ERROR;
    }
    return fd;
}

