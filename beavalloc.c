#ifndef MEMREQUEST
#  define MEMREQUEST 1024
#endif   // MEMREQUEST

typedef struct block {
   struct block *next;
   struct block *prev;
   size_t size;
}  block_t;

static block_t *head;
static block_t *tail;

block_t *initialize_block(size_t size)
{
   block_t *blk = { .next = NULL, .prev = NULL, .size = size};
}

void append_node(block_t *blk)
{
   if(!head){  
      head = blk; // if the head hasn't been initialized, make the new block the head
   }
   if(tail){
      tail->next = blk; // if the tail exists, make its *next point to the new block
      blk->prev = tail; // make the new block's *prev point to the old tail too
   }
   tail = blk; // append the new block to the list by making it the new tail
}

void *beavalloc(size_t size)
{
   if(size == 0 || size == NULL)
      return NULL;
   
   block_t *currblock;
   currblock = sbrk(MEMREQUEST);
   if(currblock == (void *) -1)
      return NULL;

   append_node(currblock);
   return (void *) currblock;
}

void beavfree(void *ptr)
{

}

void beavalloc_reset(void)
{

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