/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_RM
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

 #define TAKEN TRUE
 #define FREE FALSE

 #define LAST TRUE
 #define NOT_LAST FALSE

 #define PAGE_SIZE 8192

typedef struct kma_frame kma_frame;

struct kma_frame
{
  kma_page_t* page;
  kma_frame* prev;//the pointer to the previous frame
  kma_frame* next;//the pointer to the next frame
  bool occupied;
  bool last;//true when last in "chain"
};

/************Global Variables*********************************************/


kma_page_t* entry_page = NULL;


/************Function Prototypes******************************************/
 //each memory "frame" looks like this
//byte 0: pointer to kma_page_t that allocated this memory
//byte 4: int (0 means free, 1 means taken)
//byte 8: int, size

kma_frame* write_new_frame(void* addr, kma_page_t* page, kma_frame* prev, kma_frame* next, bool occupied, bool last);
void* data_ptr(kma_frame* current);
void print_debug();

/************External Declaration*****************************************/

/**************Implementation***********************************************/
 


kma_frame* write_new_frame(void* addr, kma_page_t* page, kma_frame* prev, kma_frame* next, bool occupied, bool last){

  kma_frame* frame = (kma_frame*) addr;

  // add a pointer to the page structure at the beginning of the page
  frame->page = page;

  frame->prev = prev;

  frame->next = next;

  //set equal to free
  frame->occupied = occupied;

  frame->last = last;

  return frame;

}

int frame_size(kma_frame* frame){

	return ((char*)frame->next) - (  ((char*)frame)   + sizeof(kma_frame)  );

}


kma_frame* last_frame(){

	kma_frame* current = (kma_frame*)entry_page->ptr;

	while(current->last == NOT_LAST){
		current = current->next;
	}
	return current;
}

kma_frame* first_frame(){

	return (kma_frame*)entry_page->ptr;
}

void* data_ptr(kma_frame* current){

	return current + 1;//add sizeof(kma_frame)
}


//changes the kma_frame objects appropriately
void allocate_frame(kma_frame* frame, int new_size){

	frame->occupied = TAKEN;

	int sub_frame_size = frame_size(frame) - new_size  - sizeof(kma_frame);

	if(sub_frame_size > 0){
		//if theres anough room to allocate another frame
		void* sub_frame_addr = ((char*)frame) + sizeof(kma_frame) + new_size;
		write_new_frame( sub_frame_addr, frame->page, frame, frame->next, FREE, frame->last);

		//point the orginal frame to point to the new sub_frame
		frame->next = (sub_frame_addr);

		//point the original next's prev to point to the new sub_frame
		frame->next->next->prev = frame->next;

		//mark it as no longer the last in the chain
		frame->last = NOT_LAST;
	}

}

bool first_frame_in_page(kma_frame* frame){

	return frame == frame->page->ptr;
}

//changes the kma_frame objects appropriately
void free_frame(kma_frame* frame){

	//fprintf(stdout, "free: %p\n", frame);

	frame->occupied = FREE;

	if(frame->last == NOT_LAST && frame->next->occupied == FREE && !first_frame_in_page(frame->next)){
	//if theres a nextframe and its free then combine it with the current frame
		//however, do not combine it if the next is the beginning of a page
		//since that is how we keep track of when to free a page

		//if the next frame is last, copy that last denotion over to this frame
		frame->last = frame->next->last;
		//combine this frame with next
		frame->next = frame->next->next;//fix next pointer
		frame->next->prev = frame;//fix next's previous pointer
	}

	//do the same with the previous frame
	if(frame->prev != NULL && frame->prev->occupied == FREE && !first_frame_in_page(frame)){

		frame->prev->last = frame->last;
		frame->prev->next = frame->next;//fix next pointer
		frame->prev->next->prev = frame->prev;//fix next's prev pointer

		//for page-freeing purposes, set 'frame' to prev
		frame = frame->prev;

	}

	bool stop = FALSE;//used to detect when we are free the last page (special case)
	while(!stop && first_frame_in_page(frame) && frame->last == LAST){

		kma_page_t* temp = frame->page;

		if(frame->prev == NULL){
			stop = TRUE;
			entry_page = NULL;

		}

		//change frame to the previous one and set it to last
		if(frame->prev != NULL){
			frame->prev->last = LAST;
			frame = frame->prev;
		}


		//print_debug();
		//need a temp since frame gets changed above but we cannot do the page free until th very last step
		free_page(temp);



	}
	

}


//called once to set up the entry_page
void init_first_page(){

	entry_page = get_page();

	void* next = ((char*) entry_page->ptr) + entry_page->size;
	write_new_frame( entry_page->ptr, entry_page, NULL, next, FREE, LAST);

}


void* kma_malloc(kma_size_t size)
{

	if(size >= PAGE_SIZE)
		return NULL;

	if(entry_page == NULL)
		init_first_page();

	kma_frame* current = (kma_frame*)entry_page->ptr;

	//go until last in the list or we find one that fits
	bool fits = (current->occupied == FREE &&  frame_size(current) >= size);
	while(current->last == NOT_LAST && !fits){
		current = current->next;
		fits = (current->occupied == FREE &&  frame_size(current) >= size);
	}

	void* ret_addr;

	//if it fits...
	if(fits){

		allocate_frame(current,size);
		ret_addr =  data_ptr(current);

	//if not...
	}else{

		//if nothing in the resource map fits, we need to allocate a new page
		//ifwe are here, current should be last


		kma_page_t* new_page = get_page();
		void* next = ((char*) new_page->ptr) + new_page->size;
		
 		kma_frame* new_frame = write_new_frame(new_page->ptr, new_page, current, next, FREE, LAST);

 		allocate_frame(new_frame,size);

 		current->last = NOT_LAST;

 		
 		ret_addr =  data_ptr(new_frame);
	}

  //print_debug();
  return ret_addr;
  
}

kma_frame* get_frame(kma_frame* wanted){

	kma_frame* current = first_frame();

	while(current != wanted){
		current = current->next;
	}

	return current;
}

void kma_free(void* ptr, kma_size_t size)
{

	kma_frame* ptr_to_frame = ptr - sizeof(kma_frame);

	//fprintf(stdout, "request, free: %p\n", ptr_to_frame);

	free_frame(ptr_to_frame);
	//print_debug();
}

void print_debug(){

	kma_frame* current = first_frame();


	while(current->last != LAST){
		fprintf(stdout, "frame: %p, page: %p, prev: %p, next: %p, occupied: %x, last: %x, \n",
				 current, current->page, current->prev, current->next, current->occupied, current->last);

				
		current = current->next;
	}

			fprintf(stdout, "frame: %p, page: %p, prev: %p, next: %p, occupied: %x, last: %x, \n",
				 current, current->page, current->prev, current->next, current->occupied, current->last);

	fprintf(stdout, "================================================\n");



}

#endif // KMA_RM

//r ~/school/OS_Assignment2_kma/testsuite/1.trace
//r ~/school/OS_Assignment2_kma/testsuite/2.trace
//r ~/school/OS_Assignment2_kma/testsuite/3.trace
//r ~/school/OS_Assignment2_kma/testsuite/4.trace
//r ~/school/OS_Assignment2_kma/testsuite/5.trace