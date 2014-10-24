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
#include <math.h>


/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"
#include <stdio.h>

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
	void* nextNode;
	kma_page_t*  myPage; //Pointer to the page object that points to this node's page
} freeListNode;

//Returns pointer to the first node in the filled page list
#define FILLED_PAGE_NODE_LIST (*(pageListNode**)((void*)firstPageListPage->ptr + sizeof(int*)))
//Returns pointer to the first node in the empty page node list
#define EMPTY_PAGE_NODE_LIST (*(pageListNode**)((void*)firstPageListPage->ptr + sizeof(int*)*2))

//Returns pointer to the first node in the empty free node list
#define EMPTY_FREE_NODE_LIST (*(freeListNode**)((*(kma_page_t**)firstPageListPage->ptr)->ptr + sizeof(int*)*10))
//Returns pointer to the first node in the free node list of buffSize size
#define FILLED_FREE_NODE_LIST(size) (*(freeListNode**)((*(kma_page_t**)firstPageListPage->ptr)->ptr + ((int)floor(log((double)size)/log(2.0)) - 4)*sizeof(int*)))

//Returns node count int of the page you passed in
#define NODE_COUNT(page) (*(int*)((void*)page->ptr + page->size - sizeof(int)))

/************Global Variables*********************************************/

/************Function Prototypes******************************************/

void initialize();
pageListNode* fillWithEmptyPageNodes(kma_page_t* pageListPage, int offsetFromHead);
freeListNode* fillWithEmptyFreeNodes(kma_page_t* freeListPage, int offsetFromHead);
void getNewDataPage();
void getNewPageListPage();
void getNewFreeListPage();
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
	//If there are no pages yet (no KMA_mallocs have occured)
	if (firstPageListPage == NULL)
	{
		initialize();
	}

  return NULL;
}

void kma_free(void* ptr, kma_size_t size)
{
  ;
}

void initialize()
{
	//######################################//
	//##### Prepare 1st page list page #####//
	//######################################//

	//Allocate first page list page (pointed to by firstPageListPage)
	firstPageListPage = get_page();

	//Fill page list page with empty page nodes leaving room for the pointers to
	//the free list page, the first filled node, and the first empty node. Also,
	//fill the pointer to the first empty page node
	EMPTY_PAGE_NODE_LIST = fillWithEmptyPageNodes(firstPageListPage, 3*sizeof(int*));

	//Create pointer to first filled page node and set equal to NULL
	FILLED_PAGE_NODE_LIST = NULL;

	//######################################//
	//##### Prepare 1st free list page #####//
	//######################################//

	//Allocate a free list page (first entry in page list) and
	//put a pointer to it at the head of the first page list page
	kma_page_t* freeListPage = get_page();
	*((kma_page_t**)firstPageListPage->ptr) = freeListPage;

	//Fill free list page with empty free nodes leaving room for the pointers to
	//the first filled nodes of different buff sizes and the first empty node. Also,
	//fill the pointer to the first empty page node
	EMPTY_FREE_NODE_LIST = fillWithEmptyFreeNodes(freeListPage, 11*sizeof(int*));

	int size;
	//Set pointers to first filled free nodes of all different lists equal to NULL
	int i;
	for (i = 4; i < 14; i++)
	{
		//1 << i == 2^i
		size = 1 << i;
		FILLED_FREE_NODE_LIST(size) = NULL;
	}

	//##################################//
	//##### Prepare 1st data  page #####//
	//##################################//

	//Create first data page and fill a node for it
	getNewDataPage();

}

pageListNode* fillWithEmptyPageNodes(kma_page_t* pageListPage, int offsetFromHead)
{
	//Get a pointer to the effective head of the page and type cast it to pageListNode
	pageListNode* pageNode = (pageListNode*)((void*)pageListPage->ptr + offsetFromHead);
	//Save the location of the first node
	pageListNode* firstPageNode = pageNode;
	//Get a pointer to the effective end of the page (making room for an int)
	void* effectivePageEnd = (void*)pageListPage->ptr + pageListPage->size - sizeof(int);
	//Fill in the int we made room for with zero. This is the count of the nubmer of filled
	//nodes on the page
	*(int*)effectivePageEnd = 0;

	//While there is room to insert another page list node... (probably can be <=, but try that later)
	while((void*)pageNode + sizeof(pageListNode) < effectivePageEnd)
	{
		//Add pointer to the pageListPage to each new empty page node
		pageNode->myPage = pageListPage;
		//Add pointer to the next empty node to each new empty page node
		pageNode->nextNode = pageNode + 1;
		//Go to the next empty page node
		pageNode = pageNode + 1;
	}

	//Make the nextNode pointer in the last pageNode = NULL
	(pageNode - 1)->nextNode = NULL;

	//Return pointer to the head of the empty node list
	return firstPageNode;

}

freeListNode* fillWithEmptyFreeNodes(kma_page_t* freeListPage, int offsetFromHead)
{

	//Get a pointer to the effective head of the page and type cast it to freeListNode
	freeListNode* freeNode = (freeListNode*)((void*)freeListPage->ptr + offsetFromHead);
	//Save the location of the first node
	freeListNode* firstFreeNode = freeNode;
	//Get a pointer to the effective end of the page (making room for an int)
	void* effectivePageEnd = (void*)freeListPage->ptr + freeListPage->size - sizeof(int);
	//Fill in the int we made room for with zero. This is the count of the nubmer of filled
	//nodes on the page
	*(int*)effectivePageEnd = 0;

	//While there is room to insert another page list node... (probably can be <=, but try that later)
	while((void*)freeNode + sizeof(freeListNode) < effectivePageEnd)
	{
		//Add pointer to the freeListPage to each new empty free node
		freeNode->myPage = freeListPage;
		//Add pointer to the next empty node to each new empty free node
		freeNode->nextNode = freeNode + 1;
		//Go to the next empty free node
		freeNode = freeNode + 1;
	}

	//Make the nextNode pointer in the last freeNode = NULL
	(freeNode - 1)->nextNode = NULL;

	//Return pointer to the head of the empty node list
	return firstFreeNode;

}

//Gets a new data page and puts a new page node at the front of the filled node list
void getNewDataPage()
{
	//If there isn't an empty page node to use, request a new page list page
	if(EMPTY_PAGE_NODE_LIST == NULL)
		getNewPageListPage();

	//Get an empty page node
	pageListNode* newPageNode = EMPTY_PAGE_NODE_LIST;
	//Make empty page node list point to the next node in the list
	EMPTY_PAGE_NODE_LIST = EMPTY_PAGE_NODE_LIST->nextNode;
	//Increment node counter at the end of the page
	NODE_COUNT(newPageNode->myPage) = NODE_COUNT(newPageNode->myPage) + 1;
	//Allocate new data page
	newPageNode->dataPage = get_page();
	//Clear bitMap
	int i;
	for (i = 0; i < 256; i++)
	{
		newPageNode->bitMap[i] = '\0';
	}
	//Make node point to first node in the filled page node list
	newPageNode->nextNode = FILLED_PAGE_NODE_LIST;
	//Make filled page node list point to the new node (insert in front)
	FILLED_PAGE_NODE_LIST = newPageNode;

	//If there isn't an empty free node to use, request a new free list page
	if(EMPTY_FREE_NODE_LIST == NULL)
		getNewFreeListPage();

	//Get an empty free node
	freeListNode* newFreeNode = EMPTY_FREE_NODE_LIST;
	//Make empty free node list point to the next node in the list
	EMPTY_FREE_NODE_LIST = EMPTY_FREE_NODE_LIST->nextNode;
	//Increment node counter at the end of the page
	NODE_COUNT(newFreeNode->myPage) = NODE_COUNT(newFreeNode->myPage) + 1;
	//Fill pointer to buffer location
	newFreeNode->buffLocation = newPageNode->dataPage->ptr;
	//Make node point to the first node in the size 8192 filled free node list
	newFreeNode->nextNode = FILLED_FREE_NODE_LIST(8192);
	//Make size 8192 filled free node list point to the new node (insert in front)
	FILLED_FREE_NODE_LIST(8192) = newFreeNode;


	return;
}

void getNewPageListPage()
{
}

void getNewFreeListPage()
{

}

#endif // KMA_BUD
