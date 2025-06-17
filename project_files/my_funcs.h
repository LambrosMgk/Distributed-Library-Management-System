#ifndef MY_FUNCS_H
#define MY_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>                         // For variable number of arguments.
#include <time.h>
#include "ansi-color-codes.h"


#define DEBUG_ENABLED           // Comment this out to stop getting the debug messages
#define COORDINATOR_RANK 0
#define BUF_SIZE 256            // A buffer size to hold the MPI messages

#define TAG_ACK 0

#define TAG_CONNECT 1
#define TAG_TAKE_BOOK 2
#define TAG_DONATE_BOOKS 3
#define TAG_GET_MOST_POPULAR_BOOK 4
#define TAG_CHECK_NUM_BOOKS_LOANED 5

#define TAG_START_LE_LIBR 6
#define TAG_START_LE_LOANERS 7

#define TAG_NEIGHBOR 8      // Used by clients/borrowers
#define TAG_CLIENT_ELECT 9
#define TAG_CLIENT_LEADER_SELECTED 10
#define TAG_LE_LOANERS_DONE 11

#define TAG_LE_LIBRARIES_DONE 12    // Used by libraries
#define TAG_LIB_LEADER 13
#define TAG_LIB_PARENT 14
#define TAG_LIB_ALREADY 15
#define TAG_FIND_BOOK 16
#define TAG_BOOK_REQUEST 17
#define TAG_ACK_TB 18

#define TAG_DONE_FIND_BOOK 19
#define TAG_DONATE_BOOKS_DONE 20
#define TAG_GET_POPULAR_BK_INFO 21
#define TAG_NUM_BOOKS_LOANED 22

#define TAG_SHUTDOWN 23


#define print_error(format, ...) _print_error_internal(__FILE__, __LINE__, format, ##__VA_ARGS__)

// Macro wrappers to capture file & line automatically
#define MyMalloc(size)          _MyMalloc_internal(size, __FILE__, __LINE__)
#define MyCalloc(count, size)   _MyCalloc_internal(count, size, __FILE__, __LINE__)
#define MyRealloc(ptr, size)    _MyRealloc_internal(ptr, size, __FILE__, __LINE__)


void print_all_colors();
void print_info(const char *format, ...);
void print_debug(const char *format, ...);
void print_warn(const char *format, ...);
void _print_error_internal(const char *file, int line, const char *format, ...);

void print_barrier();
void print_barrier2();
void print_barrier3();
void print_barrier4();

int get_random_in_range(int lower, int upper);

void *_MyMalloc_internal(size_t size, const char *file, int line);
void *_MyCalloc_internal(size_t count, size_t size, const char *file, int line);
void *_MyRealloc_internal(void *ptr, size_t size, const char *file, int line);

char **split_string(const char *buff, int buff_len, const char delim);

int get_string_array_size(char **str_array);
void print_string_array(char **str_array);
void free_string_array(char **str_array);

char *int_to_string(int x);

void strcat_int(char *buffer, int x);

#endif