#include "kernel/types.h"
#include "user/user.h"

void _free(void *ap);

void* _malloc(uint nbytes);

typedef long align;
union header{
    struct{ //linked list of headers
        uint block_size; //size of current node's memory block
        uint available; //flag is set to 0 if block is not free and 1 if block is free
        union header *next_block; //pointer to next node in linked list
        union header *prev_block; //pointer to previous node in linked list
    } s;
    align a;
};
typedef union header block_header;
block_header *start, *end; //start and end nodes of linked list