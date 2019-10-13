#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "beavalloc.h"

#ifndef MINALLOC
#  define MINALLOC 1024
#endif   // MINALLOC

typedef struct block_list {
   struct block_list *next;   // next block in ll
   struct block_list *prev;   // previous block in ll
   void *data;                // address of start of data (after header)
   size_t capacity;           // remaining free space in block
   size_t size;               // size of data in block (not counting header)
   uint free;                 // flag that denotes a block as free or not
}  block_t;

const static size_t header_size =  sizeof(block_t);
static block_t *head = NULL, *tail = NULL;
static uint verbose = FALSE;

void append_node(block_t *);
void remove_node(block_t *);
void insert_node(block_t *, block_t *);
void set_block_vars(block_t *, size_t);
void set_split_block_vars(block_t *, block_t *, size_t);
void reset_freed_block(block_t *);
void *find_free_block(size_t);
block_t *split_block(block_t *, size_t);
void *allocate_new_block(size_t size);
block_t *find_block_in_ll(void *);
block_t *merge_two_blocks(block_t *, block_t *);
void coalesce(void);

void append_node(block_t *block)
{
   if(!head)
      head = block;  // set the head equal to the new block if there's no head
   if(tail){
      tail->next = block;  // push the new block onto the end of the linked list
      block->prev = tail;
   }
   tail = block;  // new block becomes the new tail
}

void insert_node(block_t *ptr, block_t *block)
{
   if(ptr->next){ // put the new block into the list after the block it split off of
      (ptr->next)->prev = block;
      block->next = ptr->next;
   }
   else if(!ptr->next){
      tail = block;  // if there's no ptr->next, then the new block is the new tail
      block->next = NULL;
   }
   block->prev = ptr;
   ptr->next = block;
}

void remove_node(block_t *block)
{
   if(block->next)
      (block->next)->prev = block->prev;
   if(block->prev)
      (block->prev)->next = block->next;
}

void set_block_vars(block_t *block, size_t size)
{
   block->size = size;
   block->data = (void *)((unsigned long)block + header_size);
   block->free = FALSE;
}

void set_split_block_vars(block_t *ptr, block_t *block, size_t size)
{
   block->size = size;
   block->data = (void *)((unsigned long)block + header_size);
   block->capacity = ptr->capacity - block->size - header_size;
   block->free = FALSE;
   ptr->capacity = 0;
}

void reset_freed_block(block_t *block)
{
   block->free = TRUE;
   block->capacity += block->size;
   block->size = 0;
}

void *find_free_block(size_t size)
{
   block_t *ptr = head;
   for( ; ptr; ptr = ptr->next){
      // block isn't free but it has enough space
      if(!ptr->free && (ptr->capacity >= (size + header_size))){
         return (*split_block(ptr, size)).data;
      }
      // block is free and has enough space
      else if(ptr->free && (ptr->capacity >= (size + header_size))){
         ptr->capacity -= (size + header_size);
         set_block_vars(ptr, size);
         return ptr->data;
      }
   }
   return NULL;   // if there is no free space in existing blocks
}

block_t *split_block(block_t *ptr, size_t size)
{
   block_t *block = ptr->data + ptr->size;  // end of data stored in block
   insert_node(ptr, block);
   set_split_block_vars(ptr, block, size);
   return block;
}

void *allocate_new_block(size_t size)
{
   block_t *block = NULL;
   int msize = (size + header_size >= MINALLOC) ? (size + header_size) : MINALLOC;
   if(verbose)
      fprintf(stderr, "Allocating block of size %d\n", msize);
   block = sbrk(msize); // increment break either 1024 OR size bytes, whichever's bigger
   set_block_vars(block, size);
   block->capacity = msize - size - header_size;
   append_node(block);
   return block->data;
}

block_t *find_block_in_ll(void *ptr)
{
   block_t *block;
   for(block = head; block; block = block->next){
      if(block->data == ptr){
         if(verbose)
            fprintf(stderr, "Found block data at address %p. Freeing...\n", ptr);
         return block;
      }
   }
   return NULL;
}

block_t *merge_two_blocks(block_t *b1, block_t *b2)
{
   b1->capacity += b2->capacity + header_size;
   remove_node(b2);
   return b1;
}

void coalesce(void)
{
   block_t *block;
   for(block = head; block; block = block->next){
      if(block->free && block->prev && (block->prev)->free){
         block = merge_two_blocks(block->prev, block);   // merge block down
      }
      if(block->free && block->next && (block->next)->free){
         block = merge_two_blocks(block, block->next);   // merge block->next down
      }
   }
}

void *beavalloc(size_t size)
{
   void *addr;
   if(!size || size == 0)  // return NULL if 0 or NULL is passed
      return NULL;
   if(head && (addr = find_free_block(size)))
      return addr;   // address of first free block found
   else{
      return allocate_new_block(size);
   }
}

void beavfree(void *ptr)
{
   block_t *block = find_block_in_ll(ptr);
   if(!ptr || !block || block->free){
      if(verbose)
         fprintf(stderr, "Failed to find block at address %p. Returning...\n", ptr);
      return;
   }
   reset_freed_block(block);
   coalesce();
}

void beavalloc_reset(void)
{
   if(head){
      brk(head);     // reset program break
      head = NULL;   // get rid of linked list nodes
      tail = NULL;
   }
}

void beavalloc_set_verbose(uint8_t set_verbose)
{
   verbose = set_verbose;
}

void *beavcalloc(size_t nmemb, size_t size)
{
   void *ptr;
   if(!nmemb || !size)
      return NULL;

   ptr = beavalloc(nmemb * size);   // allocate number of blocks * block size
   memset(ptr, 0, nmemb * size);    // fill allocated memory
   return ptr;
}

void *beavrealloc(void *ptr, size_t size)
{
   void *new_location;
   if(!size)
      return NULL;
   if(!ptr){
      return allocate_new_block(size * 2);   // no need to move anything from null ptr
   }
   new_location = allocate_new_block(size);  // append new block to end of linked list
   memmove(new_location, ptr, size);         // move data from old to new location
   beavfree(ptr);                            // deallocate from old location
   return new_location;
}

void beavalloc_dump(uint leaks_only)
{
   block_t *curr = NULL;
   uint i = 0;
   uint leak_count = 0;
   uint user_bytes = 0;
   uint capacity_bytes = 0;
   uint block_bytes = 0;
   uint used_blocks = 0;
   uint free_blocks = 0;
   void *lower_mem_bound = (void *) head;
   void *upper_mem_bound = sbrk(0);

   if (leaks_only) {
      fprintf(stderr, "heap lost blocks\n");
   }
   else {
      fprintf(stderr, "heap map\n");
   }
   fprintf(stderr
         , "  %s\t%s\t%s\t%s\t%s" 
         "\t%s\t%s\t%s\t%s\t%s\t%s"
         "\n"
         , "blk no  "
         , "block add "
         , "next add  "
         , "prev add  "
         , "data add  "
         
         , "blk off  "
         , "dat off  "
         , "capacity "
         , "size     "
         , "blk size "
         , "status   "
      );
   for (curr = head, i = 0; curr != NULL; curr = curr->next, i++) {
      if (leaks_only == FALSE || (leaks_only == TRUE && curr->free == FALSE)) {
         fprintf(stderr
                  , "  %u\t\t%9p\t%9p\t%9p\t%9p\t%u\t\t%u\t\t"
                     "%u\t\t%u\t\t%u\t\t%s\t%c\n"
                  , i
                  , curr
                  , curr->next
                  , curr->prev
                  , curr->data
                  , (unsigned) ((void *) curr - lower_mem_bound)
                  , (unsigned) ((void *) curr->data - lower_mem_bound)
                  , (unsigned) curr->capacity
                  , (unsigned) curr->size
                  , (unsigned) (curr->capacity + header_size)
                  , curr->free ? "free  " : "in use"
                  , curr->free ? '*' : ' '
               );
         user_bytes += curr->size;
         capacity_bytes += curr->capacity;
         block_bytes += curr->capacity + header_size;
         if (curr->free == FALSE && leaks_only == TRUE) {
               leak_count++;
         }
         if (curr->free == TRUE) {
               free_blocks++;
         }
         else {
               used_blocks++;
         }
      }
   }
   if (leaks_only) {
      if (leak_count == 0) {
         fprintf(stderr, "  *** No leaks found!!! That does NOT mean no leaks are possible. ***\n");
      }
      else {
         fprintf(stderr
                  , "  %s\t\t\t\t\t\t\t\t\t\t\t\t"
                     "%u\t\t%u\t\t%u\n"
                  , "Total bytes lost"
                  , capacity_bytes
                  , user_bytes
                  , block_bytes
               );
      }
   }
   else {
      fprintf(stderr
               , "  %s\t\t\t\t\t\t\t\t\t\t\t\t"
               "%u\t\t%u\t\t%u\n"
               , "Total bytes used"
               , capacity_bytes
               , user_bytes
               , block_bytes
         );
      fprintf(stderr, "  Used blocks: %u  Free blocks: %u  "
            "Min heap: %p    Max heap: %p\n"
            , used_blocks, free_blocks
            , lower_mem_bound, upper_mem_bound
         );
   }
}
