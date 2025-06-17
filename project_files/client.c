#include "client.h"


/*
* Initializes a borrower_t struct.
*/
void init_client(borrower_t *client, int rank, int num_libs)
{
    // Init all fields to 0
    memset(client, 0, sizeof(borrower_t));

    client->c_id = rank - 1;
    client->rank = rank;
    client->str_rank = int_to_string(rank);
}


/*
* Clears a borrower_t struct, releasing the allocated memory of some fields and setting all the values to 0.
*/
void clear_client(borrower_t *client)
{
    if(client == NULL)
        return;

    if(client->neighbors != NULL)
        free(client->neighbors);

    if(client->str_rank != NULL)
        free(client->str_rank);

    if(client->voters != NULL)
        free(client->voters);

    memset(client, 0, sizeof(borrower_t));
}


/*
* Adds the given rank to the given client's neighbors array. If the array was empty it allocates memory and inserts the given rank.
*/
void add_rank_to_neighbor_array(int neighbor_rank, borrower_t *client)
{
    // Check if it's the first connection or realloc to add the new connection
    if(client->neighbors == NULL)    // Malloc...
    {
        client->neightbors_size++;
        client->neighbors = (int *) malloc(1 * sizeof(int));
        if(client->neighbors == NULL)
        {
            print_error("When allocating memory for client (rank %d) neighbors.", client->rank);
            exit(-1);
        }
        client->neighbors[0] = neighbor_rank;  
    }
    else                            // Realloc...
    {
        client->neightbors_size++;
        int *tmp = (int *) realloc(client->neighbors, client->neightbors_size * sizeof(int));
        if(tmp == NULL)
        {
            print_error("When allocating memory for client (rank %d) neighbors.", client->rank);
            exit(-1);
        }
        client->neighbors = tmp;
        client->neighbors[client->neightbors_size-1] = neighbor_rank;  
    }

    print_debug("Client rank %d (c_id %d) added rank %d to its neighbors", client->rank, client->c_id, client->neighbors[client->neightbors_size-1]);
}


/*
* Handle the "CONNECT" event.
* A connect message was sent (by the coordinator). The client will add the given rank (not c_id) to its neighbors array/list, send
* a "NEIGHBOR" message to that rank and after receiving "ACK" for that rank then send "ACK" to the coordinator.
* Note: Check if you are already connected with that rank
*/
void event_client_connect(int neighbor_rank, borrower_t *client)
{
    char buffer_recv[BUF_SIZE], buffer_send[BUF_SIZE];
    MPI_Status status;
    int i;


    for(i = 0; i < client->neightbors_size; i++)
    {
        if(client->neighbors[i] == neighbor_rank)
        {
            print_info("Client rank %d is already connected with rank %d", client->rank, neighbor_rank);

            // Send "ACK" to the coordinator
            memset(buffer_send, 0, sizeof(buffer_send));
            strcpy(buffer_send, "ACK");
            MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_ACK, MPI_COMM_WORLD);
            
            return;
        }
    }

    add_rank_to_neighbor_array(neighbor_rank, client);


    // Send "NEIGHBOR" to inform the other client about the connection.
    memset(buffer_send, 0, sizeof(buffer_send));
    strcpy(buffer_send, "NEIGHBOR ");
    strcat(buffer_send, client->str_rank);
    MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, neighbor_rank, TAG_NEIGHBOR, MPI_COMM_WORLD);     // +1 so that the '\0' is included in the sent message


    // Wait for "ACK" from the neighbor.
    MPI_Recv(buffer_recv, sizeof(buffer_recv), MPI_CHAR, neighbor_rank, TAG_ACK, MPI_COMM_WORLD, &status);
    print_debug("Client rank %d got '%s' from rank %d for the 'NEIGHBOR' event", client->rank, buffer_recv, neighbor_rank);


    // Send "ACK" to the coordinator
    memset(buffer_send, 0, sizeof(buffer_send));
    strcpy(buffer_send, "ACK");
    MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_ACK, MPI_COMM_WORLD);     // +1 so that the '\0' is included in the sent message
}


/*
* Handle the "NEIGHBOR" event.
* Case: the client received a "NEIGHBOR" message, indicating that the given rank is its neighbor.
* It then proceeds to add that rank to its neighbors array/list and send an "ACK" back to it.
*/
void event_client_neighbor(int neighbor_rank, borrower_t *client)
{
    char buffer_send[BUF_SIZE];


    add_rank_to_neighbor_array(neighbor_rank, client);


    // Send "ACK" to the neighbor
    memset(buffer_send, 0, sizeof(buffer_send));
    strcpy(buffer_send, "ACK");
    MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, neighbor_rank, TAG_ACK, MPI_COMM_WORLD);     // +1 so that the '\0' is included in the sent message
}


/*
* Handle the "START_LE_LOANERS" event.
* Send an "ELECT" message to my neighbor
*/
void event_client_start_le_loaners(borrower_t *client)
{
    char buffer_send[BUF_SIZE];


    // You are a leaf node
    if(client->neightbors_size == 1)
    {
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "ELECT");
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, client->neighbors[0], TAG_CLIENT_ELECT, MPI_COMM_WORLD);     // +1 so that the '\0' is included in the sent message
        client->sent_elect_to = client->neighbors[0];

        print_info("Leaf node rank %d sent 'ELECT' to rank %d", client->rank, client->neighbors[0]);
    }
}


/*
* Handle the "ELECT" message. Gather all the "ELECT" messages and then decide what to do.
* Either send to the last neighbor (that didn't send an "ELECT" message) an "ELECT" message, or
* choose youself to be the leader since you received "ELECT" from all your neighbors. In case
* of 2 "ELECT" messages going through the same edge, the process with the highest rank wins.
*/
void event_client_elect(borrower_t *client, int voter_rank)
{
    int i, j;
    char buffer_send[BUF_SIZE];


    
    if(voter_rank <= 0) // Shouldn't get here.
    {
        print_error("Voters are NULL but voter rank is negative.");
        exit(-1);
    }


    // First "ELECT" message
    if(client->voters == NULL)
    {
        client->votes++;
        client->voters = (int *) malloc(client->votes * sizeof(int));
        if(client->voters == NULL)
        {
            print_error("When allocating memory.");
            exit(-1);
        }
        client->voters[0] = voter_rank;

        print_debug("---Client rank %d voters: %d (total neighbors %d)---", client->rank, voter_rank, client->neightbors_size);
    }
    else
    {
        // Realloc and add the neighbor to the "voters" array/list.
        client->votes++;
        int *tmp = (int *) realloc(client->voters, client->votes * sizeof(int));
        if(tmp == NULL)
        {
            print_error("When allocating memory.");
            exit(-1);
        }
        client->voters = tmp;
        client->voters[client->votes - 1] = voter_rank;


        // Use the buffer_send to create a print with all my voters.
        sprintf(buffer_send, "---Client rank %d voters: ", client->rank);
        for(i = 0; i < client->votes; i++)
        {
            strcat_int(buffer_send, client->voters[i]);
            strcat(buffer_send, "-");
        }
        print_debug("%s--\n", buffer_send);
    }
    
    print_debug("Client rank %d: votes=%d, neighbors=%d", client->rank, client->votes, client->neightbors_size);
    // If all neighbors sent "ELECT" i'm probably the leader.
    if(client->votes == client->neightbors_size)
    {
        // if i've sent "ELECT" compare our ranks (with the last neighbor that sent me an "ELECT").
        if(client->sent_elect_to != 0)
        {
            if(client->rank > voter_rank)     // I'm the leader.
            {
                client->leader_rank = client->rank;
            }
            else                            // The other process is the leader.
            {
                client->leader_rank = voter_rank;
            }
        }
        else                                // All neighbors sent me ELECT and i didn't so I'm the leader.
        {
            client->leader_rank = client->rank;
        }
        print_info(HGRN"---------------Client rank %d (c_id %d) elected rank %d as leader.---------------"reset, client->rank, client->c_id, client->leader_rank);


        // Leader must notify the neighbors about the end of the LE and then the coordinator.
        if(client->rank == client->leader_rank)
        {
            MPI_Status status;

            print_info(GRN"Leader client is going to send 'LE_LOANERS <leader_rank>' to it's neighbors (boardcasting to the SP)."reset);
            print_debug(UMAG"Debug is enabled. Clients will also print their neighbors."reset);

            memset(buffer_send, 0, sizeof(buffer_send));
            strcpy(buffer_send, "LE_LOANERS ");
            strcat_int(buffer_send, client->leader_rank);

            // Send the leader rank to the neighbors and they'll propagate it through the SP.
            for(i = 0; i < client->neightbors_size; i++)
            {
                MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, client->neighbors[i], TAG_CLIENT_LEADER_SELECTED, MPI_COMM_WORLD);
                print_info("Client leader %d sent 'LE_LOANERS' to %d", client->leader_rank, client->neighbors[i]);
            }

            // Wait for 'ACK'
            for(i = 0; i < client->neightbors_size; i++)
            {
                memset(buffer_send, 0, sizeof(buffer_send));
                MPI_Recv(buffer_send, sizeof(buffer_send), MPI_CHAR, client->neighbors[i], TAG_ACK, MPI_COMM_WORLD, &status);
                if(strcmp(buffer_send, "ACK") != 0)
                {
                    print_error("Client %d didn't get 'ACK' from neighbor rank %d but instead got %s", client->rank, client->neighbors[i], buffer_send);
                }
                print_info("Client leader %d got 'ACK' from rank %d", client->leader_rank, client->neighbors[i]);
            }

            // Send "LE_LOANERS_DONE" to the coordinator with the leader rank.
            memset(buffer_send, 0, sizeof(buffer_send));
            strcpy(buffer_send, "LE_LOANERS_DONE");
            MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_LE_LOANERS_DONE, MPI_COMM_WORLD);
        }
    }
    else if(client->votes == (client->neightbors_size - 1))     // Send "ELECT" to the neighbor that's left.
    {
        char flag;
        print_debug("Client rank %d is searching for the neighbor that didn't send 'ELECT'", client->rank);
        // Find out who is the neighbor that didn't send an "ELECT" message.
        for(i = 0; i < client->neightbors_size; i++)
        {
            flag = 0;
            for(j = 0; j < client->votes; j++)
            {
                if(client->neighbors[i] == client->voters[j])
                {
                    flag = 1;
                    break;
                }
            }

            if(flag == 0)
            {
                memset(buffer_send, 0, sizeof(buffer_send));
                strcpy(buffer_send, "ELECT");
                MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, client->neighbors[i], TAG_CLIENT_ELECT, MPI_COMM_WORLD);     // +1 so that the '\0' is included in the sent message
                client->sent_elect_to = client->neighbors[i];
                print_debug("-----Rank %d sent 'ELECT' to it's last neighbor rank %d", client->rank, client->neighbors[i]);
                break;
            }
        }
    }
// End of function
}


/*
* This function sets the leader rank in the borrower struct and propagates it to your neighbors.
*/
void event_client_leader_selected(borrower_t *client, char *str_leader_rank, int sender_rank)
{
    int i;
    char buffer[BUF_SIZE];
    MPI_Status status;


    print_info("Client rank %d got 'LE_LOANERS %s' from rank %d", client->rank, str_leader_rank, sender_rank);
#ifdef DEBUG_ENABLED
    // Print my neighbors for debug
    sprintf(buffer, "---Client rank %d neighbors: ", client->rank);
    for(i = 0; i < client->neightbors_size; i++)
    {
        strcat_int(buffer, client->neighbors[i]);
        strcat(buffer, "-");
    }
    print_debug("%s--", buffer);
#endif


    client->leader_rank = atoi(str_leader_rank);
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "LE_LOANERS ");
    strcat(buffer, str_leader_rank);

    // Send it to your neighbors (except the one that sent it to you).
    for(i = 0; i < client->neightbors_size; i++)
    {
        if(client->neighbors[i] != sender_rank)
        {
            print_info("Client rank %d sending 'LE_LOANERS %s' to rank %d", client->rank, str_leader_rank, client->neighbors[i]);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->neighbors[i], TAG_CLIENT_LEADER_SELECTED, MPI_COMM_WORLD);
        }
    }
    

    // Wait for 'ACK' from neighbors. Also helps with a cleaner output in the console
    for(i = 0; i < client->neightbors_size; i++)
    {
        if(client->neighbors[i] != sender_rank)
        {
            memset(buffer, 0, sizeof(buffer));
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client->neighbors[i], TAG_ACK, MPI_COMM_WORLD, &status);
            if(strcmp(buffer, "ACK") != 0)
            {
                print_error("Client %d didn't get 'ACK' from neighbor rank %d but instead got %s", client->rank, client->neighbors[i], buffer);
            }
            print_debug("Client rank %d got 'ACK' from neighbor rank %d", client->rank, client->neighbors[i]);
        }
    }

    // Send 'ACK' to sender. (Side note: if you are a leaf you simply won't execute anything in the "if" in the for loops)
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "ACK");
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, sender_rank, TAG_ACK, MPI_COMM_WORLD);
}


/*
* Adds a new book entry to the (end of the) client's book list. If the client already has an entry
* of this book, it increments a "loaned" counter.
*/
void client_add_book(borrower_t *client, int b_id, int b_cost)
{
    borrower_book_t *tmp, *prev;


    if(client->book_list == NULL)
    {
        tmp = (borrower_book_t *) MyCalloc(1, sizeof(borrower_book_t));
        tmp->book.id = b_id;
        tmp->book.cost = b_cost;
        tmp->loan_num = 1;
        tmp->next = NULL;

        client->book_list = tmp;
        print_debug("Client rank %d added book %d to its list", client->rank, b_id);
    }
    else
    {
        // If a record of the book already exists update the loan counter
        tmp = client->book_list;
        while(tmp != NULL)
        {
            if(tmp->book.id == b_id)
            {
                tmp->loan_num++;
                print_debug("Client rank %d updated loan counter to %d for book %d", client->rank, tmp->loan_num, b_id);
                return;
            }
            prev = tmp;
            tmp = tmp->next;    // step
        }

        // Else add a new record at the end of the list
        tmp = (borrower_book_t *) MyCalloc(1, sizeof(borrower_book_t));
        tmp->book.id = b_id;
        tmp->book.cost = b_cost;
        tmp->loan_num = 1;
        tmp->next = NULL;

        prev->next = tmp;
    }
}


/*
* Handles the 'TAKE_BOOK <b_id>' message from coordinator. Calculates the l_id that's in charge of the
* b_id and sends a 'LEND_BOOK' message to that library.
*
* Library responses:
* - 'GET_BOOK <cost>' : if it has that book
* - 'ACK_TB <b_id> <cost>' but <b_id> will either be -1 indicating the book was not found, or b_id >= 0 meaning success.
*/
void event_client_takeBook(borrower_t *client, int b_id, int num_libs)
{
    int l_id, N;
    int library_rank;
    char buffer[BUF_SIZE];
    MPI_Status status;


    
    N = sqrt(num_libs);
    l_id = (b_id/N);
    library_rank = l_id + 1;

    print_info("Client rank %d send 'LEND_BOOK' to library rank %d", client->rank, library_rank);
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "LEND_BOOK ");
    strcat_int(buffer, b_id);
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD);


    // Get answer from the library
    memset(buffer, 0, sizeof(buffer));
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, library_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD, &status);


    char **string_array = split_string(buffer, strlen(buffer), ' ');
    if(strcmp(string_array[0], "GET_BOOK") == 0)
    {
        int b_cost = atoi(string_array[1]);
        print_info("Client rank %d: Got '%s' from library rank %d ('GET_BOOK')", client->rank, buffer, library_rank);
        client_add_book(client, b_id, b_cost);
    }
    else if(strcmp(string_array[0], "ACK_TB") == 0)
    {
        int b_id = atoi(string_array[1]);
        int b_cost = atoi(string_array[2]);

        if(b_id == -1)
        {
            print_info(HYEL"Client rank %d: book %d was not found in the libraries."reset, client->rank, b_id);
        }
        else
        {
            print_info("Client rank %d: Got book %d (with cost %d) from library rank %d ('ACK_TB')", client->rank, b_id, b_cost, library_rank);
            client_add_book(client, b_id, b_cost);
        }
    }
    else
    {
        print_error("Unknown message from library %d, got %s", library_rank, buffer);
    }


    // Send 'DONE_FIND_BOOK' to coordinator
    print_debug("Client rank %d send 'DONE_FIND_BOOK' to coordinator", client->rank);
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "DONE_FIND_BOOK");
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_DONE_FIND_BOOK, MPI_COMM_WORLD);
}


/*
* Handles the 'DONATE_BOOKS <b_id> <n_copies>' from coordinator. Sends a similar message
* to the leader so he can distribute the book copies.
*/
void event_client_donateBook(borrower_t *client, int b_id, int n_copies)
{
    char buffer[BUF_SIZE];
    MPI_Status status;


    // Sanity check, this should never execute because it's handled in the main client function
    if(client->rank == client->leader_rank)
    {
        print_error("Leader client shouldn't call this function, instead it should call the 'leader' version.");
        return;
    }

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "DONATE_BOOK ");     // Note: there's a difference, the 'S' is missing because that message is meant for the leader.
    strcat_int(buffer, b_id);
    strcat(buffer, " ");
    strcat_int(buffer, n_copies);
    print_info("Client rank %d send '%s' to client leader rank %d", client->rank, buffer, client->leader_rank);
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->leader_rank, TAG_DONATE_BOOKS, MPI_COMM_WORLD);


    // Wait for 'DONATE_BOOKS_DONE' from leader.
    memset(buffer, 0, sizeof(buffer));
    MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client->leader_rank, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD, &status);
    if(strcmp(buffer, "DONATE_BOOKS_DONE") != 0)
    {
        print_error("Client rank %d didn't get 'DONATE_BOOKS_DONE' from leader but instead got: %s", client->rank, buffer);
        // Correct the buffer.   
        print_error("Client rank %d corrected the buffer and is now forwarding it to coordinator.", client->rank);     
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "DONATE_BOOKS_DONE");
    }
    else
    {
        print_debug("Client rank %d got %s from leader, forwarding it to coordinator.", client->rank, buffer);
    }

    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD);
}


/*
* Function that is called by the leader, distributes the book copies among the libraries round robin style.
* Note that the book cost will also be sent in the form of 'DONATE_BOOK <b_id> <cost>'
*/
void event_client_leader_donateBook(borrower_t *client, int b_id, int n_copies, int num_libs, int client_rank)
{
    int donate_next, i, lib_rank, book_cost;
    char buffer_send[BUF_SIZE], buffer_recv[BUF_SIZE];
    MPI_Status status;


    // Sanity check
    if(client->rank != client->leader_rank)
    {
        print_error("Client rank %d is not the leader but got message 'DONATE_BOOK'", client->rank);
        return;
    }
    
    if(client_rank == -1)
        print_info(HMAG"Leader client: coordinator sent 'DONATE_BOOK' to me, no other client is involved."reset);
    print_debug("Leader client (rank %d) is going to send to %d libraries 'DONATE_BOOK <b_id> <cost>' messages.", client->rank, num_libs);
    
    
    book_cost = get_random_in_range(5, 100);        // Set a random cost that all b_id copies will share.
    memset(buffer_send, 0, sizeof(buffer_send));
    strcpy(buffer_send, "DONATE_BOOK ");
    strcat_int(buffer_send, b_id);
    strcat(buffer_send, " ");
    strcat_int(buffer_send, book_cost);

    donate_next = 1;
    for(i = 0; i < n_copies; i++)
    {
        print_info(HGRN"Leader"reset" client (rank %d) send '%s' to library rank %d", client->rank, buffer_send, donate_next);
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, donate_next, TAG_DONATE_BOOKS, MPI_COMM_WORLD);

        // Wait for 'ACK_DB'
        memset(buffer_recv, 0, sizeof(buffer_recv));
        MPI_Recv(buffer_recv, sizeof(buffer_recv), MPI_CHAR, donate_next, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD, &status);
        if(strcmp(buffer_recv, "ACK_DB") != 0)
        {
            print_error("Client leader didn't get 'ACK_DB' from library rank %d but instead got: %s", donate_next, buffer_recv);
            exit(-1);
        }

        donate_next = (donate_next % num_libs) + 1;
    }


    // After distributing the book copies send DONATE_BOOKS_DONE to the client that began this event.
    if(client_rank != -1)
    {
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "DONATE_BOOKS_DONE");
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, client_rank, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD);
    }
    else
    {
        // Send 'DONATE_BOOKS_DONE' to coordinator because there are no other clients involved.
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "DONATE_BOOKS_DONE");
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD);
    }
}


/*
* @return The most loaned book from your book list.
*/
borrower_book_t* get_most_popular_book(borrower_t *client)
{
    borrower_book_t *best_book;
    borrower_book_t *book;


    best_book = client->book_list;
    if(best_book == NULL)
        return best_book;

    book = client->book_list->next;
    while(book != NULL)
    {
        // Get most loaned book from your list
        if(book->loan_num > best_book->loan_num)
        {
            best_book = book;
        }

        book = book->next;
    }

    return best_book;
}


/*
*   Handles the 'GET_MOST_POPULAR_BOOK' event. First do a broadcast and then a convergecast
*/
void event_client_get_mostPopBook(borrower_t *client, int N, int sender_rank)
{
    MPI_Status status;
    char buffer[BUF_SIZE];
    borrower_book_t *best_book;
    int i, my_l_id, my_bookid, my_cost, my_loan_num;


    // Leaf node, might also be the leader but that won't matter because sender_rank would be 0 (coordinator)
    if(client->neightbors_size == 1)
    {
        if(client->rank != client->leader_rank)
            print_info("Client rank %d is a leaf node, end of broadcast from this side.", client->rank);
        else
            print_info("Client leader rank %d (leaf node) starting a broadcast to get the most popular book.", client->rank);
    }
    else
        print_info("Client rank %d starting a broadcast to get the most popular book.", client->rank);


    // Send message to all of my neighbors (note1: we don't have a SP here, only libraries) (note2: leaf nodes shouldn't send any messages because they only have 1 neighbor (the sender))
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "GET_MOST_POPULAR_BOOK");
    for(i = 0; i < client->neightbors_size; i++)
    {
        if(client->neighbors[i] != sender_rank)
        {
            print_debug("Client rank %d send %s to client rank %d", client->rank, buffer, client->neighbors[i]);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->neighbors[i], TAG_GET_MOST_POPULAR_BOOK, MPI_COMM_WORLD);
        }
    }


    best_book = get_most_popular_book(client);  // Leader also needs to check his book list for the best book so this line executes here (before the if-else).
    if(best_book == NULL)
    {
        print_info("Client rank %d has an empty book list", client->rank);
        my_l_id = -1;
        my_bookid = -1;
        my_cost = -1;
        my_loan_num = -1;
    }
    else
    {
        my_l_id = best_book->book.id / N;   // Calculate the l_id.
        my_bookid = best_book->book.id;
        my_loan_num = best_book->loan_num;
        my_cost = best_book->book.cost;
    }


    // After broadcasting send my most loaned book to leader.
    if(client->rank != client->leader_rank)
    {
        print_info("Client rank %d is sending my most popular book (id:%d,loaned:%d,cost:%d,l_id:%d) to leader %d", client->rank, my_bookid, my_loan_num, my_cost, my_l_id, client->leader_rank);
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "GET_POPULAR_BK_INFO ");
        strcat_int(buffer, my_bookid);
        strcat(buffer, " ");
        strcat_int(buffer, my_loan_num);
        strcat(buffer, " ");
        strcat_int(buffer, my_cost);
        strcat(buffer, " ");
        strcat_int(buffer, my_l_id);
        
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->leader_rank, TAG_GET_POPULAR_BK_INFO, MPI_COMM_WORLD);
        
        // Wait for 'ACK_BK_INFO' from leader.
        memset(buffer, 0, sizeof(buffer));
        MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client->leader_rank, TAG_ACK, MPI_COMM_WORLD, &status);
        if(strcmp(buffer, "ACK_BK_INFO") != 0)
        {
            print_error("Client rank %d expected 'ACK_BK_INFO' from client leader rank %d but instead got: %s", client->rank, client->leader_rank, buffer);
            exit(-1);
        }
        print_debug("Client rank %d got 'ACK_BK_INFO' from client leader.", client->rank);
    }
    else    // Leader section
    {
        // Receive messages from every client and do a comparison
        char **str_array;
        int clients = (N*N*N)/2;
        int num_libs = N*N;
        int bookid, loan_num, cost, best_l_id;
        borrower_book_t *best_books;            // Array that holds the most pop book for each library (indexed using l_id)


        // Init array
        best_books = (borrower_book_t *) MyCalloc(num_libs, sizeof(borrower_book_t));
        for(i = 0; i < num_libs; i++)
            best_books[i].loan_num = -1;

        // If my book list wasn't empty.
        if(my_l_id != -1)
        {
            best_books[my_l_id].book.id = my_bookid;
            best_books[my_l_id].book.cost = my_cost;
            best_books[my_l_id].loan_num = my_loan_num;
        }

        //print_debug(HGRN"Leader"reset" client rank %d expects message from client ranks %d - %d", client->rank, num_libs + 1, num_libs + clients);
        print_debug(HGRN"Leader"reset" client rank %d expects %d messages", client->rank, clients);
        for(i = num_libs + 1; i < (num_libs + clients); i++)
        {
            //print_debug(UGRN"LEADER"reset" currently expects message from rank %d", i);
            memset(buffer, 0, sizeof(buffer));
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, MPI_ANY_SOURCE, TAG_GET_POPULAR_BK_INFO, MPI_COMM_WORLD, &status);
            print_debug(HGRN"Leader"reset" got from rank %d: %s", status.MPI_SOURCE, buffer);

            str_array = split_string(buffer, strlen(buffer), ' ');
            if(strcmp(str_array[0], "GET_POPULAR_BK_INFO") != 0)
            {
                print_error("Client leader expected 'GET_POPULAR_BK_INFO' from client rank %d but instead got: %s", status.MPI_SOURCE, buffer);
                exit(-1);
            }
            bookid = atoi(str_array[1]);
            loan_num = atoi(str_array[2]);
            cost = atoi(str_array[3]);
            best_l_id = atoi(str_array[4]);


            // If book list was not empty and Empty spot for this library or more loaned book
            if(best_l_id != -1 && (best_books[best_l_id].loan_num == -1 || best_books[best_l_id].loan_num < loan_num
            // or same number of loans, compare with cost
            || (best_books[best_l_id].loan_num == loan_num && (best_books[best_l_id].book.cost < cost))))
            {
                best_books[best_l_id].book.id = bookid;
                best_books[best_l_id].book.cost = cost;
                best_books[best_l_id].loan_num = loan_num;
                print_debug("Client leader: best book now for l_id=%d is id=%d with times_loaned=%d", best_l_id, best_books[best_l_id].book.id, loan_num);
            }


            // Send 'ACK_BK_INFO' to client.
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "ACK_BK_INFO");
            print_debug("Client "HGRN"leader"reset" is sending 'ACK_BK_INFO' to client rank %d", status.MPI_SOURCE);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, status.MPI_SOURCE, TAG_ACK, MPI_COMM_WORLD);

            free_string_array(str_array);
            str_array = NULL;
        }


        // Print most popular books
        print_info(HMAG"Client leader will now print the most popular books for each library:"reset);
        for(i = 0; i < num_libs; i++)
        {
            print_info("Popular book b_id=%d times_loaned=%d for library l_id=%d", best_books[i].book.id, best_books[i].loan_num, i);
        }


        // Send 'GET_MOST_POPULAR_BOOK_DONE' to coordinator.
        print_debug("Client "HGRN"leader"reset" is sending 'GET_MOST_POPULAR_BOOK_DONE' to coordinator");
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "GET_MOST_POPULAR_BOOK_DONE");
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_GET_MOST_POPULAR_BOOK, MPI_COMM_WORLD);
    }

} // End of func


/*
* Goes through the book list and adds all the loan counters and returns the final value.
*/
int get_loaned_books(borrower_t *client)
{
    int total_loaned;
    borrower_book_t *tmp_book;


    // Integrity check
    if(client == NULL)
        return 0;

    total_loaned = 0;
    tmp_book = client->book_list;
    while(tmp_book != NULL)
    {
        total_loaned += tmp_book->loan_num;
        tmp_book = tmp_book->next;
    }

    return total_loaned;
}


/*
* Handles the 'CHECK_NUM_BOOKS_LOAN' event.
*/
void event_client_check_numBooksLoan(borrower_t *client, int sender_rank)
{
    char buffer[BUF_SIZE];
    int i, total_loans;
    char **str_array;
    MPI_Status status;


    // Leaf node, might also be the leader but that won't matter because sender_rank would be 0 (coordinator)
    if(client->neightbors_size == 1)
    {
        if(client->rank != client->leader_rank)
            print_info("Client rank %d is a leaf node, end of broadcast from this side.", client->rank);
        else
            print_info("Client leader rank %d (leaf node) starting a broadcast to get all the loan counters.", client->rank);
    }
    else
        print_info("Client rank %d starting a broadcast to get all the loan counters.", client->rank);



    // Broadcast: Send 'CHECK_NUM_BOOKS_LOAN' to my neighbors.
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "CHECK_NUM_BOOKS_LOAN");
    for(i = 0; i < client->neightbors_size; i++)
    {
        // Skip the sender.
        if(client->neighbors[i] == sender_rank)
            continue;

        print_debug("Client rank %d is sending to rank %d: %s", client->rank, client->neighbors[i], buffer);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->neighbors[i], TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
    }

    // Set my total loans
    total_loans = get_loaned_books(client);
    print_debug("Client rank %d has %d times loans", client->rank, total_loans);

    // Wait to get their counts
    for(i = 0; i < client->neightbors_size; i++)
    {
        // Skip the sender.
        if(client->neighbors[i] == sender_rank)
            continue;

        memset(buffer, 0, sizeof(buffer));
        MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, client->neighbors[i], TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD, &status);
        print_debug("Client rank %d got from %d: %s", client->rank, client->neighbors[i], buffer);
        str_array = split_string(buffer, strlen(buffer), ' ');
        if(strcmp(str_array[0], "NUM_BOOKS_LOANED") != 0)
        {
            print_error("Client %d expected 'NUM_BOOKS_LOANED <times_loaned>' from client rank %d but instead got: %s", client->rank, client->neighbors[i], buffer);
            exit(-1);
        }

        total_loans += atoi(str_array[1]);

        free_string_array(str_array);
        str_array = NULL;


        // Send 'ACK_NBL'
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "ACK_NBL");
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client->neighbors[i], TAG_ACK, MPI_COMM_WORLD);
    }


    
    // Leader section
    if(client->rank == client->leader_rank)
    {
        // Send results to coordinator
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "CHECK_NUM_BOOKS_LOAN_DONE ");
        strcat_int(buffer, total_loans);
        print_info("Client leader rank %d is sending to %d (coordinator): %s", client->rank, sender_rank, buffer);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
    }
    else    // Send loan num to sender and it'll eventually reach leader
    {
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "NUM_BOOKS_LOANED ");
        strcat_int(buffer, total_loans);
        print_info("Client rank %d is sending to %d: %s", client->rank, sender_rank, buffer);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, sender_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);

        // Wait for 'ACK_NBL'
        memset(buffer, 0, sizeof(buffer));
        MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, sender_rank, TAG_ACK, MPI_COMM_WORLD, &status);
        if(strcmp(buffer, "ACK_NBL") != 0)
        {
            print_error("Client %d expected 'ACK_NBL' from client rank %d but instead got: %s", client->rank, sender_rank, buffer);
            exit(-1);
        }
        print_debug("Client rank %d got 'ACK_NBL' from sender rank %d", client->rank, sender_rank);
    }
}



/*
* Function to start a client process. (The process is started from MPI and then calls this function)
*/
void start_client(int rank, int num_libs)
{
    char buffer_recv[BUF_SIZE], buffer_send[BUF_SIZE];
    MPI_Status status;
    borrower_t client;
    char **strings_array = NULL;
    int N;
    

    N = sqrt(num_libs);

    // initialize borrower struct
    init_client(&client, rank, num_libs);
    // Initialize buffer just in case
    memset(buffer_recv, 0, sizeof(buffer_recv));
    

    
    while(1)
    {
        //Block on receive and examine the message when it arrives (or use MPi_Probe for that)
        MPI_Recv(buffer_recv, sizeof(buffer_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        

        strings_array = split_string(buffer_recv, strlen(buffer_recv), ' ');

        if(strcmp(strings_array[0], "CONNECT") == 0)
        {
            int neighbor_id = atoi(strings_array[1]);   // Reminder: the neighbor rank is incremented by 1 to align with the process ranks

            event_client_connect(neighbor_id, &client);
        }
        else if(strcmp(strings_array[0], "NEIGHBOR") == 0)
        {
            int neighbor_id = atoi(strings_array[1]);

            event_client_neighbor(neighbor_id, &client);
        }


        else if(strcmp(strings_array[0], "START_LE_LOANERS") == 0)
        {
            event_client_start_le_loaners(&client);
        }
        else if(strcmp(strings_array[0], "ELECT") == 0)
        {
            event_client_elect(&client, status.MPI_SOURCE);            
        }
        else if(strcmp(strings_array[0], "LE_LOANERS") == 0)        // Leader elected.
        {
            event_client_leader_selected(&client, strings_array[1], status.MPI_SOURCE);
        }


        else if(strcmp(strings_array[0], "TAKE_BOOK") == 0)
        {
            int b_id = atoi(strings_array[1]);
            event_client_takeBook(&client, b_id, num_libs);
        }
        else if(strcmp(strings_array[0], "DONATE_BOOKS") == 0)  // Coordinator sends the msg
        {
            int b_id = atoi(strings_array[1]);
            int n_copies = atoi(strings_array[2]); 
            if(client.rank != client.leader_rank)
                event_client_donateBook(&client, b_id, n_copies);
            else
                event_client_leader_donateBook(&client, b_id, n_copies, num_libs, -1);
        }
        else if(strcmp(strings_array[0], "DONATE_BOOK") == 0)   // Leader donates books
        {
            int b_id = atoi(strings_array[1]);
            int n_copies = atoi(strings_array[2]); 
            event_client_leader_donateBook(&client, b_id, n_copies, num_libs, status.MPI_SOURCE);
        }
        else if(strcmp(strings_array[0], "GET_MOST_POPULAR_BOOK") == 0)
        {
            event_client_get_mostPopBook(&client, N, status.MPI_SOURCE);
        }
        else if(strcmp(strings_array[0], "CHECK_NUM_BOOKS_LOAN") == 0)
        {
            event_client_check_numBooksLoan(&client, status.MPI_SOURCE);
        }
        else if(strcmp(strings_array[0], "SHUTDOWN") == 0)
        {
            break;
        }
        else
        {
            print_error("Client got unknown message from %d: %s", status.MPI_SOURCE, buffer_recv);
        }


        // Release memory
        memset(buffer_recv, 0, sizeof(buffer_recv));    // Clear the memory just in case.
        free_string_array(strings_array);
        strings_array = NULL;
    }

    print_info(URED"Client"reset" rank %d got message from coordinator, shutting down...", client.rank);

    // Release used memory of the struct fields.
    clear_client(&client);
}