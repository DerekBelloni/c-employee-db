#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "parse.h"
#include "file.h"
#include "common.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n - create new database file\n");
    printf("\t -f - (required) path to database file\n");
    return;
}


int main(int argc, char *argv[]) {
    char *filepath = NULL;
    bool newfile = false;
    bool list = false;
    int c;
    int dbfd = -1;
    char *updatestring = NULL;
    char *addstring = NULL;
    char *removestring = NULL;

    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    // The colons after the flag indicate data is expected with the flag
    while ((c = getopt(argc, argv, "nf:a:lr:h:")) != -1) {
        switch (c) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'a':
                addstring = optarg;
                break;
            case 'l': 
                list = true;
                break; 
            case 'r': 
                removestring = optarg;
                break;
            case 'h':
                updatestring = optarg;
                break;
            case '?':
                printf("Unknown option -%c\n", c);
                break;
            default:
                return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is a required agrument\n");
        print_usage(argv);

        return 0;
    }

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }

       if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return -1;
       }
    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return -1;
        }
        if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return -1;
        }
    }
    
    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employees\n");
        return 0;
    } 

    if (addstring) {
        // Now we need to make more room in the employee structure for another employee
        // We can use 'realloc'
        dbhdr->count++;
        employees = realloc(employees, dbhdr->count*sizeof(struct employee_t));
        add_employee(dbhdr, employees, addstring);
    }

    if (list) {
        list_employees(dbhdr, employees);
    }

    if (removestring) {
        if (remove_employee(dbfd, dbhdr, employees, removestring) != STATUS_SUCCESS) {
            printf("Failed to remove employee\n");
            return 0;
        } 
    }

    if (updatestring) {
        if (update_hours(dbfd, dbhdr, employees, updatestring) != STATUS_SUCCESS) {  
            printf("Failed to update employee hours\n");
            return 0;
        }
    }

    output_file(dbfd, dbhdr, employees);
    
    return 0;
}