#include "user/memory_management.h"

void* _malloc(uint nbytes){
    if(nbytes<=0){ //check if number of nbytes is 0
        return 0;
    }
    block_header *block = start;
    uint tbytes = nbytes + sizeof(block_header); //calculates total number of bytes to store (header + memory)
    int alignment = 8;
    if(tbytes < alignment){
        tbytes = alignment;
    }
    if(tbytes % alignment != 0){
        tbytes += alignment - (tbytes % alignment);
    }
    while(block != 0){ //while block exists in linked list
        if(block->s.block_size >= nbytes && block->s.available == 1){
            if(block->s.block_size - sizeof(block_header) > tbytes){
                block_header *new_block = (void*)block + tbytes; //new block of memory to split up regions of memory to avoid over allocation
                new_block->s.block_size = block->s.block_size - tbytes; //new block size
                new_block->s.available = 1; //new block is available
                block->s.next_block->s.prev_block = new_block; //prev pointer of next block now points to new block
                new_block->s.next_block = block->s.next_block; //new block points to next block
                block->s.next_block = new_block; //previous block points to old block
                new_block->s.prev_block = block; //new block points to previous block
                block->s.block_size = tbytes - sizeof(block_header);
            }
            block->s.available = 0; //block is no longer available
            return (void*)(block+1); //+1 so we can move out of the header into the actual memory
        } else{
            block = block->s.next_block; //checks next block in linked list
        }
    }
    int pages = tbytes;
    if(pages < 4096){
        pages = 4096;
    }
    if(pages % 4096 != 0){
        pages = pages + ( 4096 - (pages % 4096));
    }
    void *mem_start = sbrk(pages); //increases heap to store tbytes
    if(mem_start == (void*) -1){ //if sbrk fails
        return 0;
    }
    block = mem_start; //assinging start of allocated memory to block_header type
    block->s.block_size = tbytes - sizeof(block_header);
    block->s.available = 0;
    block->s.next_block = 0;
    if(start == 0){
        start = block; //if this is the first header
        block->s.prev_block = 0; //prev pointer is 0 because there is no previous block
    }else{
        block->s.prev_block = end; //prev pointer point to former end block in linked list
    }
    end->s.next_block = block;
    end = block; //block is the last value in the linked list
    if((sbrk(0) - sizeof(block_header) > (char*)(block) + block->s.block_size)){
        block_header *new_block = (void*)block + block->s.block_size;
        new_block->s.block_size = sbrk(0) - (char*)(block + 1) - block->s.block_size - sizeof(block_header);
        new_block->s.next_block = 0;
        new_block->s.prev_block = block;
        new_block->s.available = 1;
        block->s.next_block = new_block;
        end = new_block;  
    } 
    return (void*)(block + 1); //returns void pointer to start of allocated memory
}

void _free(void *ap){
    if(ap == 0){
        return;
    }
    int prev_flag = 0;
    block_header *block = (block_header*)ap - 1; //block_header set to header of ap
    uint blocksize = block->s.block_size;
    //merges regions of memory to avoid internal memory fragmentation:
    if(block->s.next_block->s.available == 1){ //merges next block if it is also available
        block->s.block_size += block->s.next_block->s.block_size + sizeof(block_header);
        if(block->s.next_block == end){
            end = block;
        }
        block->s.next_block = block->s.next_block->s.next_block;
        block->s.next_block->s.prev_block = block; //bypasses next block
    }
    if(block != start){
        if(block->s.prev_block->s.available == 1){ //merges prev block if it is also available
            block->s.prev_block->s.block_size += block->s.block_size + sizeof(block_header);
            block->s.prev_block->s.next_block = block->s.next_block;
            block->s.next_block->s.prev_block = block->s.prev_block; //next block points to prev block (bypasses block)
            prev_flag = 1;
        }
    }
    if(blocksize + sizeof(block_header) < 4096){
        blocksize = 4096 - sizeof(block_header);
    }
    if((blocksize + sizeof(block_header)) % 4096 != 0){
        blocksize += 4096 - (blocksize % 4096);
        blocksize -= sizeof(block_header);
    }
    void *end_heap = sbrk(0);
    int* start_mem = (int*)ap;
    if(prev_flag == 1){
        start_mem = (int*)block->s.prev_block;
    }
    if(block == end){ //checks if the current block is the last block in the heap
        if(4*((int*)end_heap - start_mem) > 4096 + sizeof(block_header)){
            int num_pages = ((4*((int*)end_heap - (start_mem+sizeof(block_header))))) / 4096;
            if(prev_flag == 0){
                block->s.block_size -= (num_pages * 4096);
                block->s.available = 1; //block is freed and now available
            }else{
                block->s.prev_block->s.block_size -= num_pages * 4096;
                block->s.prev_block->s.available = 1;
                end = block->s.prev_block;
            }
            sbrk(-(num_pages * 4096));
            return;
        }
    }
    block->s.available = 1; //block is freed and now available
}