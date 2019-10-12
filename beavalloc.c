#include <stddef.h>

#ifndef MEMREQUEST
#  define MEMREQUEST 1024
#endif   // MEMREQUEST

typedef struct block_list {
   struct block_list *next;
   struct block_list *prev;
   size_t size;
   uint freed;
}  block_t;

static block_t *head;
static block_t *tail;

block_t *initialize_block(size_t);
void append_node(block_t *);
void clear_linked_list(void);

block_t *initialize_block(size_t size)
{
   block_t *blk;
   blk->next = NULL;
   blk->prev = NULL;
   blk->size = size;
   return blk;
}

void append_node(block_t *blk)
{
   blk->next = NULL; // new block is the last block in the list
   if(!head){  
      head = blk; // if the head hasn't been initialized, make the new block the head
   }
   if(tail){
      tail->next = blk; // if the tail exists, make its *next point to the new block
      blk->prev = tail; // make the new block's *prev point to the old tail too
   }
   tail = blk; // append the new block to the list by making it the new tail
}

void clear_linked_list(void)
{
   block_t *ptr = head;
   while(ptr){
      block_t *temp = ptr->next;
      ptr = NULL;
      ptr = temp;
   }
}

void *beavalloc(size_t size)
{
   if(!size || size == 0)
      return NULL;
   
   block_t *currblock;
   currblock = sbrk(MEMREQUEST);
   if(currblock == (void *) -1)
      return NULL;
   currblock->size = size;
   currblock->freed = FALSE;
   append_node(currblock);
   return (void *) currblock;
}

void beavfree(void *ptr)
{

}

void beavalloc_reset(void)
{
   if(head){
      brk(head);
      head = NULL;
   }
}

void beavalloc_set_verbose(uint8_t)
{

}

void *beavcalloc(size_t nmemb, size_t size)
{

}

void *beavrealloc(void *ptr, size_t size)
{

}

void beavalloc_dump(uint leaks_only)
{

}