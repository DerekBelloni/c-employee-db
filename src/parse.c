#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    int i = 0;
    for(; i < dbhdr->count; i++) {
        printf("Employee %d\n", i);
        printf("\tName: %s\n ", employees[i].name);
        printf("\tAddress: %s\n ", employees[i].address);
        printf("\tHours Worked: %d\n ", employees[i].hours);
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    printf("%s\n", addstring);

    char *name = strtok(addstring, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    printf("%s %s %s\n", name, addr, hours);

    strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
    strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));
    
    employees[dbhdr->count-1].hours = atoi(hours);

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Received bad file descriptor from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == -1) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count*sizeof(struct employee_t));

    int i = 0;
    for(; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int update_hours(int fd, struct dbheader_t *dbhdr, struct employee_t *employees, char *updatestring) {
    if (fd < 0) {
        printf("Received bad file descriptor from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;
    int updated = 0;

    char *name = strtok(updatestring, ",");
    int hours = atoi(strtok(NULL, ","));

    int i = 0;
    for(; i < count; i++) {
        if (strcmp(employees[i].name, name) == 0) {
            employees[i].hours = hours;
            updated = 1;
            break;
        }   
    }

    if (updated) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_ERROR;
    }
}

int remove_employee(int fd, struct dbheader_t *dbhdr, struct employee_t *employees, char *removestring) {
    int removed = 0;
    int i, j;

    for (i = 0; i < dbhdr->count; i++) {
        if (strcmp(employees[i].name, removestring) == 0) {
            for (j = i; j < dbhdr->count - 1; j++) {
                employees[j] = employees[j + 1];
            }
            removed = 1;
            dbhdr->count--; 
            dbhdr->filesize = sizeof(struct dbheader_t) + (sizeof(struct employee_t) * dbhdr->count);
            break; 
        }
    }

    if (removed) {
        if (ftruncate(fd, dbhdr->filesize) == -1) {
            perror("Failed to truncate file");
            return STATUS_ERROR;
        }
        return STATUS_SUCCESS;
    } else {
        return STATUS_ERROR;
    }
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Received bad file descriptor from the user\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->version = htons(dbhdr->version);
    dbhdr->count = htons(dbhdr->count);
    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));

    lseek(fd, 0, SEEK_SET);

    write(fd, dbhdr, sizeof(struct dbheader_t));

    int i = 0;
    for(; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        write(fd, &employees[i], sizeof(struct employee_t));
    }

    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Received bad file descriptor from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == -1) {
        printf("Malloc failed to create a database header");
        return STATUS_ERROR;
    }
    
    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database\n");
        free(header);
        return -1;
    }

    *headerOut = header;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == -1) {
        printf("Calloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
    return STATUS_SUCCESS;
}