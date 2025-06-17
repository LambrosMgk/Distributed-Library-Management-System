#include "server.h"


/*
* This function generates books for each library USING the l_id (not MPI rank).
*/
void init_books(library_t *library, int N)
{
    int i, l_id;
    book_library_t *tmp_book, *last;


    l_id = library->l_id;   // Use the library id

    // Create the first book, so that we also initialize the book list.
    // Note: no need to initialize to 0 'loaned_num' or 'donated_num' because i'm using Calloc.
    library->book_list = (book_library_t *) MyCalloc(1, sizeof(book_library_t));
    library->book_list->currently_available = N;
    library->book_list->next = NULL;
    library->book_list->book.cost = get_random_in_range(5, 100);
    library->book_list->book.id = l_id * N;
    last = library->book_list;

    print_debug(UMAG"Library %d, book %d (cost %d)"reset, library->rank, l_id * N, library->book_list->book.cost);
    

    // Skip the first book.
    for(i = l_id*N + 1; i < (library->l_id + 1)*N; i++)
    {
        // Create a new book
        tmp_book = (book_library_t *) MyCalloc(1, sizeof(book_library_t));
        tmp_book->currently_available = N;
        tmp_book->next = NULL;
        tmp_book->book.cost = get_random_in_range(5, 100);
        tmp_book->book.id = i;

        print_debug(UMAG"Library %d, book %d (cost %d)"reset, library->rank, i, tmp_book->book.cost);

        last->next = tmp_book;
        last = tmp_book;
    }
    print_info(UWHT"Library rank %d has books (based on its lid): %d to %d, each with %d copies."reset"\n", library->rank, l_id*N, (library->l_id + 1)*N - 1, N);
}


/*
* Check if you have neighbors in the grid and set the fields accordingly.
*/
void check_and_set_neighbors(library_t *lib, int N)
{
    // Note: x,y is calculated using the l_id value, so take that into account when calculating the ranks
    // of your neighbors.

    // Calculate l_id neighbors
    lib->up = (lib->y + 1)*N + lib->x;
    lib->down = (lib->y - 1)*N + lib->x;
    lib->left = (lib->y)*N + lib->x - 1;
    lib->right = (lib->y)*N + lib->x + 1;

    // Convert to MPI ranks
    lib->up++;      lib->down++;
    lib->left++;    lib->right++;


    // Check if you are at the left-most side of the grid
    if(lib->x == 0)
    {
        lib->left = 0;
    }
    // Check if you are at the right-most side of the grid
    if(lib->x == N-1)
    {
        lib->right = 0;
    }
    // Check if you are at the bottom side of the grid
    if(lib->y == 0)
    {
        lib->down = 0;
    }
    // Check if you are at the top side of the grid
    if(lib->y == N-1)
    {
        lib->up = 0;
    }
}


void set_unexplored(library_t *library)
{
    library->unexplored[0] = library->up;
    library->unexplored[1] = library->down;
    library->unexplored[2] = library->left;
    library->unexplored[3] = library->right;
}

/*
* Initializes a library_t struct with the given arguments.
*/
void init_library(library_t * library, int library_rank, int num_libs, int N)
{
    // Process 0 is neither a library nor a client so the library processes start at id 1
    // The minus 1 is necessary in order to get correct indexing.
    library->l_id = library_rank - 1;
    library->rank = library_rank;
    library->str_rank = int_to_string(library_rank);

    // N is already calculated (sqrt(num_libs);)
    library->x = library->l_id % N;
    library->y = library->l_id / N;
    check_and_set_neighbors(library, N);

    // For the DFS SP.
    library->leader_rank = library_rank;    // Set myself as the leader
    library->parent_rank = 0;
    
    // If a neighbor is missing the value will be set to 0 anyways from the check n set
    set_unexplored(library);
    library->children = NULL;
    library->children_num = 0;

    init_books(library, N);
}


/*
* Clears a library_t struct, releasing the allocated memory of some fields and setting all the values to 0.
*/
void clear_library(library_t * library)
{
    if(library == NULL)
        return;

    if(library->str_rank != NULL)
        free(library->str_rank);

    if(library->children != NULL)
        free(library->children);


    memset(library, 0, sizeof(library_t));
}


/*
* 
* @return 1 if there was an unexplored neighbor and you sent a "LEADER" message to it
* or 0 if there are no more unexplored neighbors.
*/
int explore(library_t *library)
{
    char buffer_send[BUF_SIZE];
    int i;
    MPI_Status status;


    // Explore()
    for(i = 0; i < 4; i++)
    {
        if(library->unexplored[i] != 0)  // if you have a neighbor
        {
            print_debug("Rank %d is exploring and sent 'LEADER' to neighbor rank %d", library->rank, library->unexplored[i]);
            // Send <leader, leader> to Pk
            memset(buffer_send, 0, sizeof(buffer_send));
            strcpy(buffer_send, "LEADER ");
            strcat_int(buffer_send, library->leader_rank);
            MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, library->unexplored[i], TAG_LIB_LEADER, MPI_COMM_WORLD);
            
            // Remove neighbor rank from unexplored
            library->unexplored[i] = 0;
            return 1;
        }
    }

    // If you reach here it means you don't have any more unexplored neighbors.
    if(library->parent_rank != library->rank)
    {
        // Send <parent,leader> to parent
        print_debug("Rank %d doesn't have any more unexplored neighbors, sending 'PARENT' to rank %d", library->rank, library->parent_rank);
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "PARENT ");
        strcat_int(buffer_send, library->leader_rank);
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, library->parent_rank, TAG_LIB_PARENT, MPI_COMM_WORLD);   // +1 so that the '\0' is included in the sent message
    }
    else
    {
        // Terminate as the root of the SP.
        print_info(GRN"END OF LE"reset UGRN": Rank %d won the elections and is now the leader."reset, library->rank);


        // Send to the other libraries that the election is over. For debug reasons.
    #ifdef DEBUG_ENABLED
        // Use buffer to create a print with all my children.
        sprintf(buffer_send, "---Leader library (rank %d) children: ", library->rank);
        for(i = 0; i < library->children_num; i++)
        {
            strcat_int(buffer_send, library->children[i]);
            strcat(buffer_send, "-");
        }
        print_debug("%s--", buffer_send);

        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "LE_LIBR_DONE");
        for(i = 0; i < library->children_num; i++)
        {
            MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, library->children[i], TAG_LE_LIBRARIES_DONE, MPI_COMM_WORLD);
        }
        // Don't let the coordinator know LE is done yet. 
        // Wait for 'ACK' so that the output is clear and we can see the whole tree in the console.
        for(i = 0; i < library->children_num; i++)
        {
            memset(buffer_send, 0, sizeof(buffer_send));
            MPI_Recv(buffer_send, sizeof(buffer_send), MPI_CHAR, library->children[i], TAG_ACK, MPI_COMM_WORLD, &status);
            if(strcmp(buffer_send, "ACK") != 0)
            {
                print_error("Leader didn't get 'ACK' from child rank %d but instead got %s", library->children[i], buffer_send);
            }
            print_debug("Leader got 'ACK' from child rank %d", library->children[i]);
        }
    #endif


        // Send to coordinator 'LE_LIBR_DONE'
        print_debug("Leader rank %d sending 'LE_LIBR_DONE' to coordinator", library->rank);
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "LE_LIBR_DONE");
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, COORDINATOR_RANK, TAG_LE_LIBRARIES_DONE, MPI_COMM_WORLD);
    }

    return 0;
}


/*
* Handle the event/message "START_LEADER_ELECTION".
*/
void event_lib_start_le(library_t *library)
{
    // "upon receiving no message:"
    if(library->parent_rank == 0)
    {
        // Leader is already set from the init_library function
        library->parent_rank = library->rank;

        // Explore(), send "LEADER" to an unexplored neighbor.
        explore(library);
    }
}


/*
* Handle the event/message "LEADER". "Upon receiving <leader,new-id> from Pj"
*/
void event_recv_leader(library_t *library, int sender_rank, int new_leader_rank)
{
    char buffer_send[BUF_SIZE];
    int i;

    
    // Switch to new tree
    if(library->leader_rank < new_leader_rank)
    {
        library->leader_rank = new_leader_rank;
        library->parent_rank = sender_rank;


        // Children := 0
        library->children_num = 0;
        free(library->children);
        library->children = NULL;


        // Reset neighbors expect Pj (sender_rank)
        set_unexplored(library);
        for(i = 0; i < 4; i++)
        {
            if(library->unexplored[i] == sender_rank)
                library->unexplored[i] = 0;
        }

        explore(library);
    }
    else if(library->leader_rank == new_leader_rank)    // already in the same tree
    {
        // Send "ALREADY", you already have that leader
        memset(buffer_send, 0, sizeof(buffer_send));
        strcpy(buffer_send, "ALREADY ");
        strcat_int(buffer_send, library->leader_rank);
        MPI_Send(buffer_send, strlen(buffer_send) + 1, MPI_CHAR, sender_rank, TAG_LIB_ALREADY, MPI_COMM_WORLD);
    }
    // else leader > new-id and the DFS for new-id is stalled
}


/*
* Handle the event "ALREADY".
*/
void event_recv_already(library_t *library, int new_rank)
{
    if(library->leader_rank == new_rank)
    {
        print_debug("Rank %d got 'ALREADY' and the same leader rank (%d), continuing exploring.", library->rank, library->leader_rank);
        explore(library);
    }
}


/*
* Handle the event "PARENT".
*/
void event_recv_parent(library_t *library, int new_leader_rank, int sender_rank)
{
    // If you don't share the same leader ignore the message, else add to children
    if(library->leader_rank == new_leader_rank)
    {
        print_debug("Rank %d got parent from rank %d, adding as a child", library->rank, sender_rank);

        // First child
        if(library->children == NULL)
        {
            library->children = (int *) malloc(1 * sizeof(int));
            if(library->children == NULL)
            {
                print_error("error when allocating memory");
                exit(-1);
            }
            
            library->children[0] = sender_rank;
            library->children_num++;
        }
        else    // +1 child
        {
            library->children_num++;
            int *tmp = (int *) realloc(library->children, library->children_num * sizeof(int));
            if(tmp == NULL)
            {
                print_error("error when allocating memory");
                exit(-1);
            }
            
            library->children = tmp;
            library->children[library->children_num-1] = sender_rank;
        }
        
        explore(library);
    }
    else 
    {
        print_debug("Rank %d got parent from rank %d, but we have different leaders (mine:%d, theirs:%d) so message is ignored", library->rank, sender_rank, library->leader_rank, new_leader_rank);
    }
}


/*
*
*/
void event_le_done(library_t *library)
{
    char buffer[BUF_SIZE];
    int i;
    MPI_Status status;
    
    
    // Leaf node on the SP.
    if(library->children_num == 0)
    {
        // Send ACK to parent.
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "ACK");
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library->parent_rank, TAG_ACK, MPI_COMM_WORLD);
        return;
    }
    
    
#ifdef DEBUG_ENABLED
    // Use buffer to create a print with all my children.
    sprintf(buffer, "---Library rank %d children: ", library->rank);
    for(i = 0; i < library->children_num; i++)
    {
        strcat_int(buffer, library->children[i]);
        strcat(buffer, "-");
    }
    print_debug("%s--", buffer);
#endif
    

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "LE_LIBR_DONE");
    for(i = 0; i < library->children_num; i++)
    {
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library->children[i], TAG_LE_LIBRARIES_DONE, MPI_COMM_WORLD);
    }
    
    // Wait for 'ACK' so that the output is clear and we can see the whole tree in the console.
    for(i = 0; i < library->children_num; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, library->children[i], TAG_ACK, MPI_COMM_WORLD, &status);
        if(strcmp(buffer, "ACK") != 0)
        {
            print_error("Rank %d didn't get 'ACK' from child rank %d but instead got %s", library->rank, library->children[i], buffer);
        }
        print_debug("Rank %d got 'ACK' from child rank %d", library->rank, library->children[i]);
    }

    // Got ACK from children. Send ACK to parent.
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "ACK");
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library->parent_rank, TAG_ACK, MPI_COMM_WORLD);
}


/*
* Search for a book with b_id in the given library's list.
* @return A pointer to the book struct if it exists (even if there are no available copies), otherwise returns NULL.
*/
book_library_t *search_book(library_t *library, int b_id)
{
    book_library_t *tmp_book;


    tmp_book = library->book_list;
    while(tmp_book != NULL)
    {
        if(tmp_book->book.id == b_id)
            return tmp_book;
        tmp_book = tmp_book->next;  // step
    }

    return NULL;
}


/*
* Handles the 'LEND_BOOK <b_id>' message from a client. Searches in the book list of the library,
* - if the book is found send 'GET_BOOK <cost>' to client.
* - else send 'FIND_BOOK <b_id>' to library leader, get 'FOUND_BOOK <l_id`>' and send 'BOOK_REQUEST <b_id> <c_id> <l_id>'
*
* Note: i don't include <l_id> in the message 'BOOK_REQUEST' my rank can be found by "status.MPI_SOURCE"
* Note: in the 'BOOK_REQUEST' message instead of c_id i'm sending the client MPI rank.
* Note: i've modified 'ACK_TB' to include the cost of the book.
*/
void event_lend_book(library_t *library, int b_id, int client_rank, int N)
{
    char buffer[BUF_SIZE];
    book_library_t *book;
    MPI_Status status;


    book = search_book(library, b_id);
    print_debug("Library rank %d got 'LEND_BOOK %d' from client rank %d", library->rank, b_id, client_rank);

    // If i have an available copy of the book
    if(book != NULL && book->currently_available != 0)
    {
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "GET_BOOK ");
        strcat_int(buffer, book->book.cost);
        print_info("Library rank %d sending book %d to client %d", library->rank, book->book.id, client_rank);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD);

        book->currently_available--;
        book->loaned_num++;
        print_info("Library rank %d stats for book %d are: currently_available=%d, loaned_num=%d.", library->rank, book->book.id, book->currently_available, book->loaned_num);
    }
    else
    {
        // if you're the leader library don't send a message to yourself
        if(library->rank == library->leader_rank)
        {
            int l_id;

            l_id = b_id / N;        // Calculate the l_id that b_id should belong to.
            
            print_info(HRED"I'm the library leader"reset);
            // Simulate a message from yourself and continue with the function
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "FOUND_BOOK ");
            strcat_int(buffer, l_id+1);
        }
        else
        {
            // Send 'FIND_BOOK' to library leader.
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "FIND_BOOK ");
            strcat_int(buffer, b_id);
            print_info("Library rank %d doesn't have the book %d, sending 'FIND_BOOK' to library leader.", library->rank, b_id);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library->leader_rank, TAG_FIND_BOOK, MPI_COMM_WORLD);

            memset(buffer, 0, sizeof(buffer));
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, library->leader_rank, TAG_FIND_BOOK, MPI_COMM_WORLD, &status);
        }



        char **string_array = split_string(buffer, strlen(buffer), ' ');
        if(strcmp(string_array[0], "FOUND_BOOK") == 0)    // Leader found the library rank that has the b_id
        {
            int lib_rank = atoi(string_array[1]);


            // Sanity check
            if(library->rank == lib_rank)
            {
                print_warn(UYEL"Leader library returned my rank for the event '%s'. (Maybe this this book should be in my list but is not yet added?)"reset, buffer);
            }
            
            
            // In either case send a fail message to client.
            if(lib_rank == -1 || library->rank == lib_rank)
            {
                // Send to 'ACK_TB -1 0' to client
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "ACK_TB -1 0");
                print_info("Library rank %d didn't find the book %d, sending to client %d: %s", library->rank, b_id, client_rank, buffer);
                MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD);
            }
            else if(lib_rank != -1)
            {
                // Send 'BOOK_REQUEST <b_id> <c_id> <l_id>' the l_id` that the leader gave me
                // Note: i don't include <l_id> in the message my rank can be found by "status.MPI_SOURCE"
                // Note: instead of c_id i'm sending the client MPI rank.
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "BOOK_REQUEST ");
                strcat_int(buffer, b_id);
                strcat(buffer, " ");
                strcat_int(buffer, client_rank);
                print_info("Library rank %d sending 'BOOK_REQUEST %d %d' to rank %d (l_id %d) that the leader gave me.", library->rank, b_id, client_rank, lib_rank, lib_rank-1);
                MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, lib_rank, TAG_BOOK_REQUEST, MPI_COMM_WORLD);


                // Wait for 'ACK_TB <b_id> <cost>'
                memset(buffer, 0, sizeof(buffer));
                MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, lib_rank, TAG_BOOK_REQUEST, MPI_COMM_WORLD, &status);
                print_debug("Library rank %d got from rank %d (and will forward to client %d): %s", library->rank, lib_rank, client_rank, buffer);

                // Send 'ACK_TB <b_id> <cost>' to client
                MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client_rank, TAG_TAKE_BOOK, MPI_COMM_WORLD);
            }
            else
            {
                print_error("Something went wrong, got: %s", buffer);
            }
        }
        else
        {
            print_error("Library rank %d was expecting 'FOUND_BOOK' from leader but got %s", library->rank, buffer);
        }

        // Free memory
        free_string_array(string_array);
        string_array = NULL;
    }
}


/*
* This handles the 'FIND_BOOK <b_id>' message that is sent to the library leader.
* It calculates the l_id` of the library that has the b_id and returns the rank of that
* library (l_id` + 1) to the requesting library with the message 'FOUND_BOOK <rank>'. 
*/
void event_find_book(library_t *library, int b_id, int request_lib_rank, int N)
{
    char buffer[BUF_SIZE];
    int l_id;

    l_id = b_id / N;
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "FOUND_BOOK ");
    strcat_int(buffer, l_id+1);
    print_info("Leader library calculated that rank %d (l_id %d) has the book %d, sending 'FIND_BOOK' to library rank %d.", l_id + 1, l_id, b_id, request_lib_rank);
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, request_lib_rank, TAG_FIND_BOOK, MPI_COMM_WORLD);
}


/*
* This handles the 'BOOK_REQUEST <b_id> <c_id> <l_id>' message that is sent to a 
* library l_id` if the library l_id doesn't have the requested b_id book.
*
* Reply to the l_id library with 'ACK_TB <b_id>' where b_id could be -1 if i don't have the book.
*/
void event_book_request(library_t *library, int b_id, int client_rank, int lib_rank)
{
    char buffer[BUF_SIZE];
    book_library_t *book;
    int book_id, book_cost;


    book = search_book(library, b_id);

    // Send 'ACK_TB <b_id> <cost>' to l_id (don't forget to convert to MPI rank) if you have available copies of b_id.
    if(book != NULL && book->currently_available != 0)
    {
        book_id = book->book.id;
        book_cost = book->book.cost;

        book->currently_available--;
        book->loaned_num++;
        print_debug("Library rank %d has book %d and updated the counters: currently_available to %d and loaned_num to %d", library->rank, book_id, book->currently_available, book->loaned_num);
    }
    else
    {
        book_id = -1;
        book_cost = 0;
    }
    
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "ACK_TB ");
    strcat_int(buffer, book_id);
    strcat(buffer, " ");
    strcat_int(buffer, book_cost);
    print_info("Library rank %d sending 'ACK_TB <%d> <%d>' to library %d (that servers client rank %d)", library->rank, book_id, book_cost, lib_rank, client_rank);
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, lib_rank, TAG_BOOK_REQUEST, MPI_COMM_WORLD);
}


/*
* Helper function to hide the logic of adding a book to the book list of a library.
*/
void add_book(library_t *library, int b_id, int cost)
{
    book_library_t *book;
    book_library_t *tmp = library->book_list;


    book = MyCalloc(1, sizeof(book_library_t));
    book->currently_available = 1;
    book->donated_num = 1;
    book->book.id = b_id;
    book->book.cost = cost;
    book->next = NULL;      // Not actually needed cuz i used calloc.
        
    // Sanity check, could be useful if you want to initialize a library with donate messages.
    if(tmp == NULL)     
    {
        library->book_list = book;
    }
    else
    {
        while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        
        tmp->next = book;   // Add new entry at the end of the list.
    }
}


/*
* Handles the 'DONATE_BOOK <b_id> <cost>' message. Check if the book id being donated is already in the library
* - if yes increment the available counter
* - else create new entry
*/
void event_donate_book(library_t *library, int b_id, int cost, int client_rank)
{
    char buffer[BUF_SIZE];
    book_library_t *book;


    book = search_book(library, b_id);

    // New book entry
    if(book == NULL)
    {
        add_book(library, b_id, cost);
        print_info("Library rank %d added a new book entry with id %d.", library->rank, b_id);
    }
    else
    {
        book->donated_num++;
        book->currently_available++;
        print_info("Library rank %d updated book entry id %d: donated_num=%d, currently_available=%d.", library->rank, book->book.id, book->donated_num, book->currently_available);
    }


    // Send 'ACK_DB' to the client
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "ACK_DB");
    print_info("Library rank %d sending '%s' to client rank %d", library->rank, buffer, client_rank);
    MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, client_rank, TAG_DONATE_BOOKS_DONE, MPI_COMM_WORLD);
}


/*
* Returns the rank of the next library in the grid like playing snake. Even y numbers go right-wise in the libraries 
* grid, odd y numbers go left-wise. Might return 0 if you are the last node in the grid.
*/
int get_next_snake_lib_rank(library_t *library)
{
    int next_rank;

    if(library->y % 2 == 0)    // Go right-wise (or upwards)
    {
        if(library->right != 0)
            next_rank = library->right;
        else
            next_rank = library->up;
    }
    else
    {
        if(library->left != 0)
            next_rank = library->left;
        else
            next_rank = library->up;
    }

    return next_rank;
}


/*
* Goes through the book list and adds all the loan counters and returns the final value.
*/
int get_total_loaned_books(library_t *library)
{
    int total_loaned = 0;
    book_library_t *tmp_book;


    // Integrity check
    if(library == NULL)
        return 0;

    tmp_book = library->book_list;
    while(tmp_book != NULL)
    {
        total_loaned += tmp_book->loaned_num;
        tmp_book = tmp_book->next;  // Step
    }

    return total_loaned;
}


/*
* Handle the 'CHECK_NUM_BOOKS_LOAN' event. Send 'CHECK_NUM_BOOKS_LOAN' to every library starting from l_id 0 (MPI rank 1).
*/
void event_check_num_books_loan(library_t *library, int N)
{
    char buffer[BUF_SIZE];
    int next_rank, total_loaned;
    MPI_Status status;


    if(library->rank == library->leader_rank)
    {
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "CHECK_NUM_BOOKS_LOAN");

        // If the first node is also the leader, skip to the next
        // If you were the first node you won't receive any message either.
        if(library->rank == 1)
        {
            print_info("Library leader rank %d is the first in the grid, sending message to the next.");
            next_rank = library->right;

            print_info("Library leader rank %d sending '%s' to library rank %d", library->rank, buffer, next_rank);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, next_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
        }
        else
        {
            next_rank = 1;

            print_info("Library leader rank %d sending '%s' to library rank %d", library->rank, buffer, next_rank);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, next_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);


            // Wait to receive a 'CHECK_NUM_BOOKS_LOAN' message and pass it over. 
            memset(buffer, 0, sizeof(buffer));
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, MPI_ANY_SOURCE, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD, &status);
            if(strcmp(buffer, "CHECK_NUM_BOOKS_LOAN") != 0)
            {
                print_error("Library leader %d expected 'TAG_CHECK_NUM_BOOKS_LOANED' but instead got from library rank %d: %s", library->rank, status.MPI_SOURCE, buffer);
            }

             // if you are also the last node in the grid (rank N*N)
            if(library->rank == N*N)
            {
                print_info("Library leader %d: i'm the last node in the grid, stopping broadcasting.", library->rank);
            }
            else // pass it over...
            {
                next_rank = get_next_snake_lib_rank(library);

                // Sanity check
                if(next_rank == 0)
                    print_error("Library leader rank %d couldn't find next rank, impossible!");

                print_info("Library leader rank %d sending '%s' to library rank %d", library->rank, buffer, next_rank);
                MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, next_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Count section: Wait for messages from the other libraries and add all the counts
        char **str_array;
        int num_books_loan;
        
        total_loaned = get_total_loaned_books(library);
        print_info("Library leader rank %d has %d loaned books", library->rank, total_loaned);

        for(int i = 1; i <= N*N; i++)
        {
            // You can't expect a message from yourself.
            if(i == library->rank)
            {
                continue;
            }


            memset(buffer, 0, sizeof(buffer));
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, i, TAG_NUM_BOOKS_LOANED, MPI_COMM_WORLD, &status);
            print_debug("Library leader rank %d got from rank %d: %s", library->rank, i, buffer);

            str_array = split_string(buffer, strlen(buffer), ' ');
            if(strcmp(str_array[0], "NUM_BOOKS_LOANED") != 0)
            {
                print_error("Library leader rank %d expected 'NUM_BOOKS_LOANED' from rank %d but instead got: %s", library->rank, i, buffer);
            }

            num_books_loan = atoi(str_array[1]);
            total_loaned += num_books_loan;

            free_string_array(str_array);
            str_array = NULL;

            // Send 'ACK_NBL' back to library.
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "ACK_NBL");
            print_debug("Library leader rank %d sending '%s' to rank %d.", library->rank, buffer, i);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, i, TAG_ACK, MPI_COMM_WORLD);
        }

        // Print message as per the assignment.
        print_info("Library books: <%d>", total_loaned);
        
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "CHECK_NUM_BOOKS_LOAN_DONE ");
        strcat_int(buffer, total_loaned);
        print_debug("Library leader rank %d sending '%s' to coordinator.", library->rank, buffer);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, 0, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
        // End of leader.
    }
    else
    {
        next_rank = get_next_snake_lib_rank(library);
        
        // End of the broadcast.
        if(next_rank == 0)
        {
            print_info("Library rank %d is the last node in the grid, broadcasting stops here.", library->rank);
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, "CHECK_NUM_BOOKS_LOAN");
            print_info("Library rank %d sending '%s' to library rank %d", library->rank, buffer, next_rank);
            MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, next_rank, TAG_CHECK_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
        }

        // Calculate the total loaned books
        total_loaned = get_total_loaned_books(library);

        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "NUM_BOOKS_LOANED ");
        strcat_int(buffer, total_loaned);
        print_info("Library rank %d sending '%s' to library rank %d", library->rank, buffer, library->leader_rank);
        MPI_Send(buffer, strlen(buffer) + 1, MPI_CHAR, library->leader_rank, TAG_NUM_BOOKS_LOANED, MPI_COMM_WORLD);
        

        // Wait for 'ACK_NBL' from the leader.
        memset(buffer, 0, sizeof(buffer));
        MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, library->leader_rank, TAG_ACK, MPI_COMM_WORLD, &status);
        print_debug("Library rank %d got from leader: %s", library->rank, buffer);

        if(strcmp(buffer, "ACK_NBL") != 0)
        {
            print_error("Library rank %d expected 'ACK_NBL' from rank %d but instead got: %s", library->rank, library->leader_rank, buffer);
        }
    }
}



/*
* Function that starts a library (server) process. (The process is started from MPI and then calls this function)
*/
void start_server(int library_rank, int num_libs)
{
    library_t library;
    char buffer_recv[BUF_SIZE], buffer_send[BUF_SIZE];
    MPI_Status status;
    char **strings_array = NULL;
    int N;


    N = sqrt(num_libs);
    init_library(&library, library_rank, num_libs, N);


    print_info("Server rank %d is at (%d,%d) in the grid.", library_rank, library.x, library.y);
    print_debug("Server rank %d has neighbors the ranks up:%d, down:%d, left:%d, right:%d", library.rank, library.up, library.down, library.left, library.right);


    // Initialize buffer just in case
    memset(buffer_recv, 0, sizeof(buffer_recv));
    

    
    while(1)
    {
        //Block on receive and examine the message when it arrives (or use MPi_Probe for that)
        MPI_Recv(buffer_recv, sizeof(buffer_recv), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        

        strings_array = split_string(buffer_recv, strlen(buffer_recv), ' ');

        if(strcmp(strings_array[0], "START_LEADER_ELECTION") == 0)
        {
            event_lib_start_le(&library);
        }
        else if(strcmp(strings_array[0], "LEADER") == 0)
        {
            int new_leader_rank = atoi(strings_array[1]);

            event_recv_leader(&library, status.MPI_SOURCE, new_leader_rank);
        }
        else if(strcmp(strings_array[0], "ALREADY") == 0)
        {
            int new_rank = atoi(strings_array[1]);

            event_recv_already(&library, new_rank);
        }
        else if(strcmp(strings_array[0], "PARENT") == 0)
        {
            int new_rank = atoi(strings_array[1]);

            event_recv_parent(&library, new_rank, status.MPI_SOURCE);
        }
        else if(strcmp(strings_array[0], "LE_LIBR_DONE") == 0)
        {
            if(library.parent_rank != status.MPI_SOURCE)
            {
                print_error("Sender (rank %d) of 'LE_LIBR_DONE' is not my (rank %d) parent rank %d", status.MPI_SOURCE, library.rank, library.parent_rank);
            }
            event_le_done(&library);
        }


        else if(strcmp(strings_array[0], "LEND_BOOK") == 0)
        {
            int b_id = atoi(strings_array[1]);

            event_lend_book(&library, b_id, status.MPI_SOURCE, N);
        }
        else if(strcmp(strings_array[0], "FIND_BOOK") == 0)
        {
            int b_id = atoi(strings_array[1]);

            event_find_book(&library, b_id, status.MPI_SOURCE, N);
        }
        else if(strcmp(strings_array[0], "BOOK_REQUEST") == 0)
        {
            int b_id = atoi(strings_array[1]);
            int client_rank = atoi(strings_array[2]);

            event_book_request(&library, b_id, client_rank, status.MPI_SOURCE);
        }
        else if(strcmp(strings_array[0], "DONATE_BOOK") == 0)
        {
            int b_id = atoi(strings_array[1]);
            int cost = atoi(strings_array[2]);
            
            event_donate_book(&library, b_id, cost, status.MPI_SOURCE);
        }

        else if(strcmp(strings_array[0], "CHECK_NUM_BOOKS_LOAN") == 0)
        {
            event_check_num_books_loan(&library, N);
        }
        else if(strcmp(strings_array[0], "SHUTDOWN") == 0)
        {
            break;
        }
        else
        {
            print_error("Library got unknown message from %d: %s", status.MPI_SOURCE, buffer_recv);
        }


        // Release memory
        memset(buffer_recv, 0, sizeof(buffer_recv));    // Clear the memory just in case.
        free_string_array(strings_array);
        strings_array = NULL;
    }

    print_info(UBLU"Library"reset" rank %d got message from coordinator, shutting down...", library.rank);

    clear_library(&library);
}