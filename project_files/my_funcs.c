#include "my_funcs.h"


/*
* Prints every color from "ansi-color-codes.h"
*/
void print_all_colors()
{
    printf(BLK "BLK" reset "\n");
    printf(RED "RED" reset "\n");
    printf(GRN "GRN" reset "\n");
    printf(YEL "YEL" reset "\n");
    printf(BLU "BLU" reset "\n");
    printf(MAG "MAG" reset "\n");
    printf(CYN "CYN" reset "\n");
    printf(WHT "WHT" reset "\n");

    printf(BBLK "BBLK (bold)" reset "\n");
    printf(BRED "BRED (bold)" reset "\n");
    printf(BGRN "BGRN (bold)" reset "\n");
    printf(BYEL "BYEL (bold)" reset "\n");
    printf(BBLU "BBLU (bold)" reset "\n");
    printf(BMAG "BMAG (bold)" reset "\n");
    printf(BCYN "BCYN (bold)" reset "\n");
    printf(BWHT "BWHT (bold)" reset "\n");

    printf(UBLK "UBLK (underline)" reset "\n");
    printf(URED "URED (underline)" reset "\n");
    printf(UGRN "UGRN (underline)" reset "\n");
    printf(UYEL "UYEL (underline)" reset "\n");
    printf(UBLU "UBLU (underline)" reset "\n");
    printf(UMAG "UMAG (underline)" reset "\n");
    printf(UCYN "UCYN (underline)" reset "\n");
    printf(UWHT "UWHT (underline)" reset "\n");

    printf(BLKB "BLKB (background)" reset "\n");
    printf(REDB "REDB (background)" reset "\n");
    printf(GRNB "GRNB (background)" reset "\n");
    printf(YELB "YELB (background)" reset "\n");
    printf(BLUB "BLUB (background)" reset "\n");
    printf(MAGB "MAGB (background)" reset "\n");
    printf(CYNB "CYNB (background)" reset "\n");
    printf(WHTB "WHTB (background)" reset "\n");

    printf(HBLK "HBLK (high intensity)" reset "\n");
    printf(HRED "HRED (high intensity)" reset "\n");
    printf(HGRN "HGRN (high intensity)" reset "\n");
    printf(HYEL "HYEL (high intensity)" reset "\n");
    printf(HBLU "HBLU (high intensity)" reset "\n");
    printf(HMAG "HMAG (high intensity)" reset "\n");
    printf(HCYN "HCYN (high intensity)" reset "\n");
    printf(HWHT "HWHT (high intensity)" reset "\n");

    printf(BHBLK "BHBLK (bold high intensity)" reset "\n");
    printf(BHRED "BHRED (bold high intensity)" reset "\n");
    printf(BHGRN "BHGRN (bold high intensity)" reset "\n");
    printf(BHYEL "BHYEL (bold high intensity)" reset "\n");
    printf(BHBLU "BHBLU (bold high intensity)" reset "\n");
    printf(BHMAG "BHMAG (bold high intensity)" reset "\n");
    printf(BHCYN "BHCYN (bold high intensity)" reset "\n");
    printf(BHWHT "BHWHT (bold high intensity)" reset "\n");
}

/*
*   Works like printf but instead it first prints an information message and then the given arguments
*   with the corresponding format.
*/
void print_info(const char *format, ...)
{
    time_t t;
    struct tm *tm_info;
    char time_buffer[30];
    char message_buf[1024];   // buffer for formatted args
    va_list args;
    
    
    if(format == NULL)
    {
        fprintf(stderr, RED"[Error]"reset" in %s, line %d: format is NULL\n", __FILE__, __LINE__);
        return;
    }

    // Get the current time
    time(&t);
    tm_info = localtime(&t);

    // Format the time as "[day-month-year hour:minutes:seconds]"
    strftime(time_buffer, sizeof(time_buffer), "[%d-%b-%y %H:%M:%S]", tm_info);
    

    // Format the variable args into message_buf
    va_start(args, format);
    vsnprintf(message_buf, sizeof(message_buf), format, args);
    va_end(args);       // Clean up

    // Print everything in one call
    printf("[INFO] %s %s\n", time_buffer, message_buf);
}


/*
*   Works like printf but instead it first prints an information message and then the given arguments
*   with the corresponding format. Can be disabled by not defining the 'DEBUG_ENABLEd' constant.
*/
void print_debug(const char *format, ...)
{
#ifdef DEBUG_ENABLED
    time_t t;
    struct tm *tm_info;
    char time_buffer[30];
    char message_buf[1024];   // buffer for formatted args
    va_list args;
    
    
    if(format == NULL)
    {
        fprintf(stderr, "[Error] in %s, line %d: format is NULL\n", __FILE__, __LINE__);
        return;
    }

    // Get the current time
    time(&t);
    tm_info = localtime(&t);

    // Format the time as "[day-month-year hour:minutes:seconds]"
    strftime(time_buffer, sizeof(time_buffer), "[%d-%b-%y %H:%M:%S]", tm_info);
    

    // Format the variable args into message_buf
    va_start(args, format);
    vsnprintf(message_buf, sizeof(message_buf), format, args);
    va_end(args);       // Clean up

    // Print everything in one call
    printf(MAG"[DEBUG]"reset" %s %s\n", time_buffer, message_buf);
#endif   
}


/*
*   Works like printf but instead it first prints an warn message and then the given arguments
*   with the corresponding format.
*/
void print_warn(const char *format, ...)
{
    time_t t;
    struct tm *tm_info;
    char time_buffer[30];
    char message_buf[1024];   // buffer for formatted args
    va_list args;
    
    
    if(format == NULL)
    {
        fprintf(stderr, RED"[Error]"reset" in %s, line %d: format is NULL\n", __FILE__, __LINE__);
        return;
    }

    // Get the current time
    time(&t);
    tm_info = localtime(&t);

    // Format the time as "[day-month-year hour:minutes:seconds]"
    strftime(time_buffer, sizeof(time_buffer), "[%d-%b-%y %H:%M:%S]", tm_info);
    

    // Format the variable args into message_buf
    va_start(args, format);
    vsnprintf(message_buf, sizeof(message_buf), format, args);
    va_end(args);       // Clean up

    // Print everything in one call
    printf(HYEL"[WARN]"reset" %s %s\n", time_buffer, message_buf);
}


/*
*   Works like printf but instead it first prints an information message and then the given arguments
*   with the corresponding format.
*/
void _print_error_internal(const char *file, int line, const char *format, ...)
{
    time_t t;
    struct tm *tm_info;
    char time_buffer[30];
    char message_buf[1024];   // buffer for formatted args
    va_list args;
    
    
    if(format == NULL)
    {
        fprintf(stderr, RED"[Error]"reset" File %s, line %d: format is NULL\n", __FILE__, __LINE__);
        return;
    }

    // Get the current time
    time(&t);
    tm_info = localtime(&t);

    // Format the time as "[day-month-year hour:minutes:seconds]"
    strftime(time_buffer, sizeof(time_buffer), "[%d-%b-%y %H:%M:%S]", tm_info);
    

    // Format the variable args into message_buf
    va_start(args, format);
    vsnprintf(message_buf, sizeof(message_buf), format, args);
    va_end(args);       // Clean up

    // Print everything in one call
    fprintf(stderr, RED"[ERROR]"reset" %s File: %s, line: %d, %s\n", time_buffer, file, line, message_buf);
}


/*
* Maybe the most useless function i've ever made. This way i have a constant number of '=' in the seperating line!
* It justs prints a line of '#' to help me seperate prints.
*/
void print_barrier()
{
    printf("==============================================================================================\n");
}


/*
* Maybe the most useless function i've ever made VOL2. This way i have a constant number of '#' in the seperating line!
* It justs prints a line of '#' to help me seperate prints.
*/
void print_barrier2()
{
    printf("##############################################################################################\n");
}


/*
* Maybe the most useless function i've ever made VOL3. This way i have a constant number of '@' in the seperating line!
* It justs prints a line of '@' to help me seperate prints.
*/
void print_barrier3()
{
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}


/*
* Maybe the most useless function i've ever made VOL3. This way i have a constant number of '()' in the seperating line!
* It justs prints a line of '()' to help me seperate prints.
*/
void print_barrier4()
{           
    printf("((((((((((((((((((((((((((((((((((((((((((((((()))))))))))))))))))))))))))))))))))))))))))))))\n");
}


/*
*   Returns a random integer in the given lower and upper limit (Inclusive).
*/
int get_random_in_range(int lower, int upper)
{
    return (rand()%(upper - lower + 1) + lower);
}


void *_MyMalloc_internal(size_t size, const char *file, int line)
{
    void *ptr = malloc(size);
    if(ptr == NULL)
    {
        _print_error_internal(file, line, "Malloc failed.");
        exit(-1);
    }
    return ptr;
}

/*
* Calloc wrapper function that includes error checking of a null pointer.
* @return A pointer to a newly allocated memory block just like calloc would.
*/
void *_MyCalloc_internal(size_t count, size_t size, const char *file, int line)
{
    void *ptr = calloc(count, size);
    if(ptr == NULL)
    {
        _print_error_internal(file, line, "Calloc failed.");
        exit(-1);
    }
    return ptr;
}

void *_MyRealloc_internal(void *ptr, size_t size, const char *file, int line)
{
    void *new_ptr = realloc(ptr, size);
    if(new_ptr == NULL)
    {
        _print_error_internal(file, line, "Realloc failed.");
        exit(-1);
    }
    return new_ptr;
}


/*
* Splits the given buffer based on the given delimeter. User should call my free function or 
* free the memory (each cell + the array) when he's done with it.
* @param buff A string buffer.
* @param buff_len The length of the string (not the buffer).
* @param delim The character that will be used as a delimeter.
* @return An array of strings that contains an extra slot with NULL to mark the end of the array.
*/
char **split_string(const char *buff, int buff_len, const char delim)
{
    int i, j, k, array_len, str_start;
    char **split_array = NULL, *tmp_str = NULL, **tmp_arr;


    if(buff == NULL)
    {
        fprintf(stderr, "Error in %s, line %d: buff is NULL\n", __FILE__, __LINE__);
        exit(-1);
    }


    array_len = 1;
    split_array = (char **) malloc(array_len * sizeof(char *));
    if(split_array == NULL)
    {
        fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
        exit(-1);
    }


    str_start = 0;
    for(i = 0; i < buff_len; i++)   // "This is a test"
    {
        if(buff[i] == delim)
        {
            tmp_str = (char *) malloc((i - str_start + 1)*sizeof(char));    // +1 for the "\0"
            if(tmp_str == NULL)
            {
                fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
                exit(-1);
            }

            // Manually copy the characters (cuz why not)
            for(k = 0, j = str_start; j < i; j++, k++)
            {
                tmp_str[k] = buff[j];
            }
            tmp_str[k] = '\0';

            //print_debug("%s", tmp_str);

            // Add the string to my array
            split_array[array_len - 1] = tmp_str;
            array_len++;
            tmp_arr = realloc(split_array, array_len * sizeof(char *));
            if(tmp_arr == NULL)
            {
                fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
                exit(-1);
            }
            split_array = tmp_arr;
            split_array[array_len - 1] = NULL;

            str_start = i+1;    // The start of the next substring
        }
    }

    // If the buffer doesn't end with a delimeter, get the final substring
    if(str_start != i)
    {
        tmp_str = (char *) malloc((i - str_start + 1)*sizeof(char));    // +1 for the "\0"
        if(tmp_str == NULL)
        {
            fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
            exit(-1);
        }

        // Manually copy the characters (cuz why not)
        for(k = 0, j = str_start; j < i; j++, k++)
        {
            tmp_str[k] = buff[j];
        }
        // If the string ends with \n remove it.
        if(tmp_str[k-1] == '\n')
            tmp_str[k-1] = '\0';
        else
            tmp_str[k] = '\0';

        //print_debug("%s", tmp_str);

        // Add the string to my array
        split_array[array_len - 1] = tmp_str;
        array_len++;
        split_array = realloc(split_array, array_len * sizeof(char *));
        if(split_array == NULL)
        {
            fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
            exit(-1);
        }
        split_array[array_len - 1] = NULL;
    }
    //print_debug("Size :%d\n", array_len);

    return split_array;
}


/*
* @param str_array A string array that has an extra spot with NULL to mark the end.
* @return The size of a string array without including the NULL cell at the end.
*/
int get_string_array_size(char **str_array)
{
    int i;
    
    if(str_array == NULL)
        return 0;

    i = 0;
    while(str_array[i] != NULL)
    {
        i++;    // step
    }
    return i;
}

/*
* Prints all the strings in the given string array.
* @param str_array A string array that contains an extra slot with NULL to mark the end of the array.
*/
void print_string_array(char **str_array)
{
    int i;
    
    if(str_array == NULL)
        return;

    i = 0;
    while(str_array[i] != NULL)
    {
        print_info("%s", str_array[i]);
        i++;    // step
    }
}


/*
* Frees the given array of strings.
* @param An array of strings.
*/
void free_string_array(char **str_array)
{
    int array_index = 0;

    if(str_array == NULL)
        return;

    while (str_array[array_index] != NULL)
    {
        // free all the strings in the array.
        free(str_array[array_index]);
        str_array[array_index] = NULL;
        array_index++;
    }

    // Free the array (the space that used to hold pointers).
    free(str_array);
}


/*
* Converts the given integer to a null-terminated string. (Reverse of atoi!)
* @param x The integer you want to convert!
* @return A newly allocated string! Free the memory when you are done with it.
*/
char *int_to_string(int x)
{
    int x_str_size;
    char *toStr = NULL;

    x_str_size = snprintf(NULL, 0, "%d", x);
    toStr = (char *) malloc((x_str_size + 1) * sizeof(char));

    if(toStr == NULL)
    {
        fprintf(stderr, "Error in %s, line %d when allocating memory.\n", __FILE__, __LINE__);
        exit(-1);
    }


    snprintf(toStr, x_str_size + 1, "%d", x);
    return toStr;
}


/*
* Appends the given integer to the given buffer, User must check that the buffer is large enough for this action otherwise behaviour
* is undefined. The function first uses my other function "int_to_string" to allocate
* memory and convert the integer to string, then it calls strcat to combine the 2 strings and releases the memory allocated.
*
* @returns Nothing, the result is appended in the given buffer.
*/
void strcat_int(char *buffer, int x)
{
    char *tmp_str = int_to_string(x);
    strcat(buffer, tmp_str);
    free(tmp_str);
    tmp_str = NULL;
}