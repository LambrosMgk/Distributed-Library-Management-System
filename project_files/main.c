#include <mpi.h>
#include <stdio.h>
#include <string.h>

#include "my_funcs.h"
#include "client.h"
#include "server.h"



/*
* Function for the coordinator that executes the "CONNECT" event.
* Sends a "CONNECT" message to the c_id1 and waits for "ACK".
* @param str_array The string array
* @param num_libs The number of libraries.
*/
void event_connect(char **str_array, int num_libs)
{
    char buffer[BUF_SIZE];
    MPI_Status status;
    int id1, id2;


    // Get the ids and increment by 1 to skip the coordinator rank 0. That way the ranks are aligned with the logical c_ids of the pdf.
    id1 = atoi(str_array[1]);
    id1++;
    //id1 += num_libs;
    id2 = atoi(str_array[2]);
    id2++;
    //id2 += num_libs;
    

    // Convert the new id2 to a string
    free(str_array[2]);     // release old malloc
    str_array[2] = int_to_string(id2);  // Get new malloc with the converted integer.


    // Create the message using a buffer.
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    strcat(strcat(strcat(buffer, str_array[0]), " "), str_array[2]);    // create the "CONNECT id2" message
    //print_debug("buffer is: %s", buffer);


    // Send "CONNECT id2" to id1
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, id1, TAG_CONNECT, MPI_COMM_WORLD);
    
    
    // Wait for "ACK"
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, id1, TAG_ACK, MPI_COMM_WORLD, &status);
    if(strcmp(buffer, "ACK") != 0)
    {
        print_error("Didn't get 'ACK' but instead got: %s", buffer);
        exit(-1);
    }
    // else you got "ACK"
    print_debug("Coordinator: Got ACK from client rank %d", id1);
}


/*
* Function for the coordinator that executes the "START_LE_LOANERS" event.
* Sends "START_LE_LOANERS" to every loaner/borrower process and waits for "LE_LOANERS_DONE" from the leader process.
*
* @return The rank of the loaners leader process.
*/
int event_start_le_loaners(int num_lib)
{
    int i, N;
    int num_of_processes;

    char buffer[BUF_SIZE];
    MPI_Status status;


    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);


    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    strcpy(buffer, "START_LE_LOANERS"); // Create the message
    for(i = num_lib + 1; i < num_of_processes; i++)
    {
        // Send "START_LE_LOANERS" to every loaner/borrower/client
        MPI_Send(buffer, strlen(buffer), MPI_CHAR, i, TAG_START_LE_LOANERS, MPI_COMM_WORLD);
        print_info(HCYN"Coordinator: sent '%s' to client rank <%d>"reset, buffer, i);
    }


    // Wait for the elected process to send "LE_LOANERS_DONE"
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, MPI_ANY_SOURCE, TAG_LE_LOANERS_DONE, MPI_COMM_WORLD, &status);

    
    if(strcmp(buffer, "LE_LOANERS_DONE") != 0)
    {
        print_error("Didn't get 'LE_LOANERS_DONE' but instead got <%s>", buffer);
        exit(-1);
    }


    // Return the rank of the loaners leader
    return status.MPI_SOURCE;
}


/*
* Function that starts the leader election for the libraries processes.
*
* @return The rank of the leader library process.
*/
int event_start_le_libraries(int num_libs)
{
    int i, N;
    char buffer[BUF_SIZE];
    MPI_Status status;



    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    strcpy(buffer, "START_LEADER_ELECTION"); // Create the message
    for(i = 1; i <= num_libs; i++)      // Side note: try starting for the biggest id going to the smallest to see a different leader to the LE?
    {
        // Send "START_LEADER_ELECTION" to every library/server
        MPI_Send(buffer, strlen(buffer), MPI_CHAR, i, TAG_START_LE_LIBR, MPI_COMM_WORLD);
        print_info(HCYN"Coordinator: sent '%s' to server rank <%d>"reset, buffer, i);
    }


    // Wait for the elected process to send "LE_LIBR_DONE"
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, MPI_ANY_SOURCE, TAG_LE_LIBRARIES_DONE, MPI_COMM_WORLD, &status);

    
    if(strcmp(buffer, "LE_LIBR_DONE") != 0)
    {
        print_error("Didn't get 'LE_LIBR_DONE' but instead got <%s>", buffer);
        exit(-1);
    }


    // Return the rank of the libraries leader
    return status.MPI_SOURCE;
}


/*
* Handles the 'TAKE_BOOK' event.
*/
void event_takeBook(int c_id, int b_id)
{
    char buffer[BUF_SIZE];
    int client_rank;
    MPI_Status status;


    client_rank = c_id + 1;             // c_id doesn't take the coordinator rank into account


    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    strcpy(buffer, "TAKE_BOOK ");
    strcat_int(buffer, b_id);
    print_info(HCYN"Coordinator: sent '%s' to client rank <%d>"reset, buffer, client_rank);
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, client_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD);

    // Wait for 'DONE_FIND_BOOK'
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client_rank, TAG_DONE_FIND_BOOK, MPI_COMM_WORLD, &status);

    if(strcmp(buffer, "DONE_FIND_BOOK") != 0)
    {
        print_error("Didn't get 'DONE_FIND_BOOK' but instead got: %s", buffer);
        exit(-1);
    }
    print_info(HCYN"Coordinator: got 'DONE_FIND_BOOK' by client rank %d."reset, client_rank);
}


/*
* Handles the 'DONATE_BOOKS' event. Sends 'DONATE_BOOKS <b_id> <n_copies>' to the client rank with the given arguments.
*/
void event_donateBook(int c_id, int b_id, int n_copies)
{
    char buffer[BUF_SIZE];
    int client_rank;
    MPI_Status status;


    client_rank = c_id + 1;             // convert to MPI rank.

    // Send 'DONATE_BOOKS <b_id> <n_copies>' to client rank
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "DONATE_BOOKS ");
    strcat_int(buffer, b_id);
    strcat(buffer, " ");
    strcat_int(buffer, n_copies);
    print_info(HCYN"Coordinator: sent '%s' to client rank <%d>"reset, buffer, client_rank);
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, client_rank, TAG_DONATE_BOOKS, MPI_COMM_WORLD);


    // Wait for 'DONATE_BOOKS_DONE'
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client_rank, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD, &status);

    if(strcmp(buffer, "DONATE_BOOKS_DONE") != 0)
    {
        print_error("Didn't get 'DONATE_BOOKS_DONE' but instead got: %s", buffer);
        exit(-1);
    }
    print_info(HCYN"Coordinator: got '%s' by client rank %d."reset, buffer, client_rank);
}


/*
* Handles the 'GET_MOST_POPULAR_BOOK' event. Send 'GET_MOST_POPULAR_BOOK' to the client leader.
*/
void event_get_most_popular_book(int borrower_leader_rank)
{
    char buffer[BUF_SIZE];
    MPI_Status status;


    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "GET_MOST_POPULAR_BOOK");
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, borrower_leader_rank, TAG_GET_MOST_POPULAR_BOOK, MPI_COMM_WORLD);


    // Wait for 'GET_MOST_POPULAR_BOOK_DONE'
    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, borrower_leader_rank, TAG_GET_MOST_POPULAR_BOOK, MPI_COMM_WORLD, &status);
    if(strcmp(buffer, "GET_MOST_POPULAR_BOOK_DONE") != 0)
    {
        print_error("Coordinator expected 'GET_MOST_POPULAR_BOOK_DONE' from client leader rank %d but instead got: %s", borrower_leader_rank, buffer);
        exit(-1);
    }
}


/*
*
*/
void event_check_num_books_loaned(int borrower_leader_rank, int library_leader_rank)
{
    char buffer[BUF_SIZE];
    char **str_array;
    int total_loaned_lib, total_loaned_bor;
    MPI_Status status;


    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "CHECK_NUM_BOOKS_LOAN");
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, library_leader_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
    MPI_Send(buffer, strlen(buffer), MPI_CHAR, borrower_leader_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);



    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, library_leader_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD, &status);
    print_debug(HCYN"Coordinator got from library leader: %s"reset, buffer);

    str_array = split_string(buffer, strlen(buffer), ' ');
    if(strcmp(str_array[0], "CHECK_NUM_BOOKS_LOAN_DONE") != 0)
    {
        print_error("Expected 'CHECK_NUM_BOOKS_LOAN_DONE' from library leader but got: %s", buffer);
    }

    total_loaned_lib = atoi(str_array[1]);
    // Release memory before proceeding with the borrowers.
    free_string_array(str_array);
    str_array = NULL;
    


    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, borrower_leader_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD, &status);
    print_debug(HCYN"Coordinator got from client leader: %s"reset, buffer);
    
    str_array = split_string(buffer, strlen(buffer), ' ');
    if(strcmp(str_array[0], "CHECK_NUM_BOOKS_LOAN_DONE") != 0)
    {
        print_error("Expected 'CHECK_NUM_BOOKS_LOAN_DONE' from client leader but got: %s", buffer);
    }

    total_loaned_bor = atoi(str_array[1]);
    // Release memory before proceeding with the borrowers.
    free_string_array(str_array);
    str_array = NULL;

    // Compare
    if(total_loaned_lib == total_loaned_bor)
    {
        print_info("CheckNumBooksLoanded   "HGRN"SUCCESS"reset);
    }
    else
    {
        print_info("CheckNumBooksLoanded   "HRED"FAILED"reset);
    }
}


/*
* A shutdown event, sends a 'SHUTDOWN' message to every process.
*/
void event_shutdown(int num_of_processes)
{
    char buffer[BUF_SIZE];
    int i;


    memset(buffer, 0, sizeof(buffer));  // clear the buffer
    strcpy(buffer, "SHUTDOWN");
    print_info(HCYN"Coordinator: sending 'SHUTDOWN' to every process"reset);

    for(i = 1; i < num_of_processes; i++)
    {
        MPI_Send(buffer, strlen(buffer), MPI_CHAR, i, TAG_SHUTDOWN, MPI_COMM_WORLD);
    }
}



int main(int argc, char* argv[]) 
{
    int num_of_processes, process_rank;

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int processor_name_len;

    FILE *test_file_ptr = NULL;
    char buffer[BUF_SIZE];
    int num_libs = -1;


    if(argc != 3)
    {
        fprintf(stderr, "Program usage: ./a.out <NUM_LIBS> <test_file>\n");
        exit(0);
    }
    num_libs = atoi(argv[1]);


    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    // Get the name of the processor
    MPI_Get_processor_name(processor_name, &processor_name_len);

    // Print off a hello world message
    //printf("Hello world from processor %s, rank %d out of %d processors\n", processor_name, process_rank, num_of_processes);


    // Wait for all processes to reach this point, in case a process didn't start.
    MPI_Barrier(MPI_COMM_WORLD);

    if(process_rank == 0)     // MPI rank 0 == Leader/Coordinator
    {
        int loaner_leader_rank, libraries_leader_rank;
        //print_all_colors();
        
        print_info(HCYN"Coordinator: NUM_LIBS: %d, testfile: %s"reset, num_libs, argv[2]);
        print_info("Coordinator: total processes = %d", num_of_processes);

        // Open the testfile given as a cla
        test_file_ptr = fopen(argv[2], "r");
        if(test_file_ptr == NULL)
        {
            print_error("Couldn't open file %s", argv[2]);
            exit(-1);
        }


        // Read the testfile
        while(fgets(buffer, sizeof(buffer), test_file_ptr))
        {
            char** str_array = split_string(buffer, strlen(buffer), ' ');
            //print_string_array(str_array);

            if(strcmp(str_array[0], "CONNECT") == 0)
            {
                event_connect(str_array, num_libs);
            }
            else if(strcmp(str_array[0], "TAKE_BOOK") == 0)
            {
                int c_id = atoi(str_array[1]);
                int b_id = atoi(str_array[2]);
                event_takeBook(c_id, b_id);
                print_barrier();
            }
            else if(strcmp(str_array[0], "DONATE_BOOK") == 0)
            {
                int c_id = atoi(str_array[1]);
                int b_id = atoi(str_array[2]);
                int n_copies = atoi(str_array[3]);
                event_donateBook(c_id, b_id, n_copies);
                print_barrier2();
            }
            else if(strcmp(str_array[0], "GET_MOST_POPULAR_BOOK") == 0)
            {
                print_barrier3();
                event_get_most_popular_book(loaner_leader_rank);
                print_barrier3();
            }
            else if(strcmp(str_array[0], "CHECK_NUM_BOOKS_LOANED") == 0)
            {
                print_barrier4();
                event_check_num_books_loaned(loaner_leader_rank, libraries_leader_rank);
                print_barrier4();
            }
            else if(strcmp(str_array[0], "START_LE_LIBR") == 0)
            {
                print_barrier();
                print_info(HCYN"Coordinator: Starting LE for libraries."reset);

                libraries_leader_rank = event_start_le_libraries(num_libs);
                
                print_info("Coordinator: LE for libraries is done, elected rank is %d", libraries_leader_rank);
                print_barrier();
            }
            else if(strcmp(str_array[0], "START_LE_LOANERS") == 0)
            {
                print_barrier();
                print_info(HCYN"Coordinator: Starting LE for loaners."reset);

                loaner_leader_rank = event_start_le_loaners(num_libs);
                
                print_info(HCYN"Coordinator: LE for loaners is done, elected rank is %d"reset, loaner_leader_rank);
                print_barrier();
            }
            else if(strcmp(str_array[0], "SHUTDOWN") == 0)
            {
                event_shutdown(num_of_processes);
            }


            //print_debug("Releasing memory...");
            free_string_array(str_array);
            str_array = NULL;
        }

        print_info(HCYN"Coordinator: End of test file."reset);
        // Send 'SHUTDOWN' to all other processes?
    }
    else
    {
        if(process_rank <= num_libs)     // Processes with rank in range of 1 to num_libs (N*N) are library processes (servers)
        {
            print_info("Process rank %d starts as a "UBLU"Server."reset, process_rank);
            start_server(process_rank, num_libs);
        }
        else                            // The rest should be from num_libs + 1 to num_of_processes. These would be the clients
        {
            print_info("Process rank %d starts as a "URED"Client."reset, process_rank);
            start_client(process_rank, num_libs);
        }
    }


    // Finalize - clean up the MPI environment. No more MPI calls can be made after this one.
    MPI_Finalize();
}