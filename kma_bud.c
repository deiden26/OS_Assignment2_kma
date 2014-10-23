/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
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
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

kma_page_t* firstPageListPage = NULL;

typedef struct
{
	char bitMap[512]; //Bit map to track allocated vs free data in data pages
	void* nextNode; //Pointer to next page List Node
	kma_page_t* dataPage; //Pointer to page object that points to a data page
	kma_page_t*  myPage; //Pointer to page object that points to the page this node is stored on
} pageListNode;

typedef struct
{
	void* buffLocation;
	int buffSize;
	void* nextNode;
	kma_page_t*  myPage; //Pointer to the page object that points to this node's page
} freeListNode;

//Returns the first node in the page list
#define PAGELIST (pageListNode*)((void*)firstPageListPage->ptr + 4)
//Returns the first node in the free list
#define FREELIST (freeListNode*)((*((kma_page_t**)firstPageListPage->ptr))->ptr)

/************Global Variables*********************************************/

/************Function Prototypes******************************************/
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
	//If there are no pages yet
	if (firstPageListPage == NULL)
	{
		//Allocate a page list page (pointed to by firstPageListPage)
		firstPageListPage = get_page();
		//Allocate a free list page (first entry in page list) and
		//put a pointer to it at the head of the first page list page
		*((kma_page_t**)firstPageListPage->ptr) = get_page();

		//Create a node for the first data page
		pageListNode* firstPageNode = (pageListNode*)((void*)firstPageListPage->ptr + 4);
		//Allocate a data page (second entry in page list)
		firstPageNode->dataPage = get_page();
		//Clear bitMap
		memset(&firstPageNode->bitMap[0], 0, sizeof(firstPageNode->bitMap));
		//Set next node to null (there aren't any other nodes yet)
		firstPageNode->nextNode = NULL;
		//Store pointer to node page's page object
		firstPageNode->myPage = firstPageListPage;

		//Get pointer to the head of the free list
		void* freeListHead = (*((kma_page_t**)firstPageListPage->ptr))->ptr;
		//Make first free list node for the entire first data page
		freeListNode* firstFreeNode = (freeListNode*)freeListHead;
		//Fill in pointer to the available buffer
		firstFreeNode->buffLocation = (firstPageNode->dataPage)->ptr;
		//Fill in the size of the available buffer
		firstFreeNode->buffSize = 8192;
		//Make next node null (there aren't any other nodes yet)
		firstFreeNode->nextNode = NULL;
		//Store pointer to node page's page object
		firstFreeNode->myPage = *((kma_page_t**)firstPageListPage->ptr);
	}


  return NULL;
}

void kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_BUD
