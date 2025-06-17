#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#include "my_funcs.h"
#include "book.h"


/*
* Multiple copies of the same book share the same id, so instead of making a lot of copies i'm having a counter variable.
*/
typedef struct book_library_t{

    book_t book;                        // An instance of a book.
    int currently_available;            // How many copies of this book are in the library
    int loaned_num;                     // How many copies of this book were loaned.
    int donated_num;                    // How many copies of this book were donated.
    struct book_library_t *next;        // Pointer to the next (unique/different) book in the list.

} book_library_t;

typedef struct {

    int l_id;                           // Logical id based on the assignment pdf.
    int rank;                           // Rank of the process (real id).
    char *str_rank;                     // String representation of the rank.

    int x,y;                            // Position on the grid.
    int up, down, left, right;          //  If not 0 they hold the rank for your neighbors on the grid

    int leader_rank;                    // For the DFS SP.
    int parent_rank;                    // For the DFS SP.
    int unexplored[4];
    int *children;
    int children_num;

    book_library_t *book_list;               // linked list for the books

} library_t;

void start_server(int l_id, int num_libs);

#endif