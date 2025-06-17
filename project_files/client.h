#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#include "my_funcs.h"
#include "book.h"


typedef struct borrower_book_t {

    book_t book;
    int loan_num;               // How many times have i loaned this book from a library.

    struct borrower_book_t *next;

} borrower_book_t;

typedef struct {

    int c_id;                   // Logical id based on the assignment pdf.
    int rank;                   // Rank of the process (real id).
    char *str_rank;             // String representation of the rank.

    int *neighbors;             // Dynamic array that holds the ranks of my neighbors/connections.
    int neightbors_size;        // The size of the neighbors array.

    int *voters;                // Dynamic array that holds the ranks of the neighbors that "voted" for me (send "ELECT" for the LE).
    int votes;                  // The number of voters (how many neighbors have sent "ELECT" to me).

    int leader_rank;            // The real id of the elected leader.
    int sent_elect_to;          // Is 0 if i haven't sent an "ELECT" message, otherwise containts the c_id that i sent a message to.
    
    borrower_book_t *book_list;  // List that holds information about what books i've borrowed

} borrower_t;


void start_client(int c_id, int num_libs);

#endif