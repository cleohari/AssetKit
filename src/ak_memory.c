/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_memory_rb.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef _WIN32
#  include <jemalloc/jemalloc.h>
#else
# define je_malloc(size)         malloc(size)
# define je_realloc(size, count) realloc(size, count)
# define je_free(size)           free(size)
#endif

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2);

static
int
ak__heap_strdup_def(const char * str);

static
int
ak__heap_strdup(ak_heap * __restrict heap,
                const char * str);

static const char * ak__emptystr = "";

static AkHeapAllocator ak__allocator = {
  .malloc   = je_malloc,
  .calloc   = je_calloc,
  .valloc   = je_valloc,
  .realloc  = je_realloc,
  .memalign = je_posix_memalign,
  .free     = je_free,
  .strdup   = ak__heap_strdup_def
};

static ak_heap ak__heap = {
  .allocator  = &ak__allocator,
  .cmp        = ak__heap_srch_cmp,
  .root       = NULL,
  .trash      = NULL,
  .srchRoot   = NULL,
  .flags      = 0
};

static
int
ak__heap_srch_cmp(void * __restrict key1,
                  void * __restrict key2) {
  return strcmp((char *)key1, (char *)key2);
}

static
int
ak__heap_strdup_def(const char * str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak__heap.allocator->malloc(memsize + 1);
  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

static
int
ak__heap_strdup(ak_heap * __restrict heap,
                const char * str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = heap->allocator->malloc(memsize + 1);
  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

AkHeapAllocator *
AK_EXPORT
ak_heap_allocator(ak_heap * __restrict heap) {
  return heap->allocator;
}

ak_heap *
AK_EXPORT
ak_heap_new(AkHeapAllocator *allocator,
            ak_heap_cmp cmp) {
  ak_heap *heap;

  heap = je_malloc(sizeof(*heap));
  ak_heap_init(heap, allocator, cmp);

  return heap;
}

void
AK_EXPORT
ak_heap_init(ak_heap * __restrict heap,
             AkHeapAllocator *allocator,
             ak_heap_cmp cmp) {
  AkHeapSrchNode *srchRootNode;
  AkHeapSrchNode *srchNullNode;
  size_t srchNodeSize;

  if (heap->flags & AK_HEAP_FLAGS_INITIALIZED)
    return;

  srchNodeSize = sizeof(AkHeapSrchNode) + ak__heapnd_sz;
  srchRootNode = je_calloc(srchNodeSize, 1);
  srchNullNode = je_calloc(srchNodeSize, 1);

  srchRootNode->key = ak__emptystr;
  srchRootNode->chld[AK__BST_LEFT]  = srchNullNode;
  srchRootNode->chld[AK__BST_RIGHT] = srchNullNode;

  srchNullNode->key = ak__emptystr;
  srchNullNode->chld[AK__BST_LEFT]  = srchNullNode;
  srchNullNode->chld[AK__BST_RIGHT] = srchNullNode;

  AK__HEAPNODE(srchRootNode)->flags = AK_HEAP_NODE_FLAGS_SRCH;
  AK__HEAPNODE(srchNullNode)->flags = AK_HEAP_NODE_FLAGS_SRCH;

  heap->root  = NULL;
  heap->trash = NULL;
  heap->flags |= AK_HEAP_FLAGS_INITIALIZED;
  heap->cmp = cmp ? cmp : ak__heap_srch_cmp;
  heap->allocator = allocator ? allocator : &ak__allocator;

  /* Real Root is srchRoot-right node */
  heap->srchRoot = srchRootNode;
  heap->srchNullNode = srchNullNode;
}

void
AK_EXPORT
ak_heap_destroy(ak_heap * __restrict heap) {
  AkHeapAllocator * __restrict allocator;

  allocator = heap->allocator;
  
  if (!(heap->flags & AK_HEAP_FLAGS_INITIALIZED))
    return;

  ak_heap_cleanup(&ak__heap);

  allocator->free(heap->srchRoot);
  allocator->free(heap->srchNullNode);

  if (heap->flags & AK_HEAP_FLAGS_DYNAMIC
      && heap != &ak__heap)
    allocator->free(heap);
}

void*
AK_EXPORT
ak_heap_alloc(ak_heap * __restrict heap,
              void * __restrict parent,
              size_t size,
              bool srch) {
  char         *chunk;
  ak_heap_node *currNode;
  ak_heap_node *parentNode;
  size_t memsize;

  memsize = ak__heapnd_sz + size;
  if (srch)
    memsize += sizeof(AkHeapSrchNode);

  memsize = ak__align(memsize);
  chunk = heap->allocator->malloc(memsize);
  assert(chunk && "malloc failed");

  if (srch) {
    AkHeapSrchNode *srchNode;
    memset(chunk,
           '\0',
           sizeof(AkHeapSrchNode));

    currNode = chunk + sizeof(AkHeapSrchNode);
    currNode->flags |= (AK_HEAP_NODE_FLAGS_SRCH | AK_HEAP_NODE_FLAGS_RED);

    srchNode = (AkHeapSrchNode *)chunk;
    srchNode->chld[AK__BST_LEFT]  = heap->srchNullNode;
    srchNode->chld[AK__BST_RIGHT] = heap->srchNullNode;
    srchNode->key = ak__emptystr;
  } else {
    currNode = chunk;
    currNode->flags = 0;
  }

  currNode->chld = NULL;

  if (parent) {
    ak_heap_node *chldNode;

    parentNode = ak__alignof(parent);
    chldNode   = parentNode->chld;

    parentNode->chld = currNode;
    if (chldNode) {
      chldNode->prev = currNode;
      currNode->next = chldNode;
    } else {
      currNode->next = NULL;
    }

    currNode->prev = parentNode;
  } else {
    if (heap->root) {
      heap->root->prev = currNode;
      currNode->next   = heap->root;
    } else {
      currNode->next = NULL;
    }

    heap->root = currNode;
    currNode->prev = NULL;
  }

  return ak__alignas(currNode);
}

void*
AK_EXPORT
ak_heap_realloc(ak_heap * __restrict heap,
                void * __restrict parent,
                void * __restrict memptr,
                size_t newsize) {
  ak_heap_node *oldNode;
  ak_heap_node *newNode;

  if (!memptr)
    return ak_heap_alloc(heap,
                         parent,
                         newsize,
                         false);

  oldNode = ak__alignof(memptr);

  if (newsize == 0) {
    ak_heap_free(heap, oldNode);
    return NULL;
  }

  newNode = je_realloc(oldNode,
                       ak__heapnd_sz + newsize);
  assert(newNode && "realloc failed");

  if (heap->root == oldNode)
    heap->root = newNode;

  if (heap->trash == oldNode)
    heap->trash = newNode;

  if (newNode->chld)
    newNode->chld->prev = newNode;

  if (newNode->next)
    newNode->next->prev = newNode;

  if (newNode->prev) {
    if (newNode->prev->chld == oldNode)
      newNode->prev->chld = newNode;

    if (newNode->prev->next == oldNode)
      newNode->prev->next = newNode;
  }

  return ak__alignas(newNode);
}

void
AK_EXPORT
ak_heap_setp(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node,
             ak_heap_node * __restrict newParent) {
  if (heap_node->prev)
    heap_node->prev->next = heap_node->next;

  if (heap_node->next)
    heap_node->next->prev = heap_node->prev;

  if (heap_node == heap->root)
    heap->root = heap_node->next;

  if (newParent->chld) {
    newParent->chld->prev = heap_node;
    heap_node->next = newParent->chld;
  }

  newParent->chld = heap_node;
}

void
AK_EXPORT
ak_heap_free(ak_heap * __restrict heap,
             ak_heap_node * __restrict heap_node) {
  AkHeapAllocator * __restrict allocator;
  allocator = heap->allocator;

  /* free all child nodes */
  if (heap_node->chld) {
    ak_heap_node *toFree;
    ak_heap_node *nextFree;

    toFree = heap_node->chld;

    do {
      nextFree = toFree->next;
      if (nextFree)
        nextFree->prev = NULL;

      if (heap->trash == toFree)
        heap->trash = nextFree;

      if (toFree->chld) {
        if (heap->trash) {
          ak_heap_node *lastNode;

          lastNode = toFree->chld;
          while (lastNode->next)
            lastNode = lastNode->next;

          heap->trash->prev = lastNode;
          lastNode->next = heap->trash;
        }

        heap->trash = toFree->chld;
        toFree->chld->prev = NULL;
      }

      /* connect prev and next */
      if (toFree->prev) {
        toFree->prev->next = nextFree;

        if (nextFree)
          nextFree->prev = toFree->prev;
      }

      if (toFree->flags & AK_HEAP_NODE_FLAGS_SRCH) {
        AkHeapSrchNode *srchNode;
        srchNode = ((char *)toFree) - sizeof(AkHeapSrchNode);

        /* remove it from rb tree */
        if (srchNode->key != ak__emptystr)
          ak_heap_rb_remove(heap, srchNode);

        allocator->free(srchNode);
      } else {
        allocator->free(toFree);
      }

      toFree = nextFree;

      /* empty trash */
      if (!toFree && heap->trash) {
        toFree = heap->trash;
        heap->trash = nextFree;
      }

    } while (toFree);
    
    heap_node->chld = NULL;
  }

  if (heap_node->prev)
    heap_node->prev->next = heap_node->next;

  if (heap->root == heap_node)
    heap->root = heap_node->next;

  heap_node->next = NULL;
  heap_node->prev = NULL;

  if (heap_node->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *srchNode;
    srchNode = ((char *)heap_node) - sizeof(AkHeapSrchNode);

    /* remove it from rb tree */
    if (srchNode->key != ak__emptystr)
      ak_heap_rb_remove(heap, srchNode);

    allocator->free(srchNode);
  } else {
    allocator->free(heap_node);
  }
}

void
AK_EXPORT
ak_heap_cleanup(ak_heap * __restrict heap) {
  while (heap->root)
    ak_heap_free(heap, heap->root);
}

void *
AK_EXPORT
ak_heap_getId(ak_heap * __restrict heap,
              ak_heap_node * __restrict heap_node) {
  if (heap_node->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *snode;
    snode = heap_node - sizeof(*snode);
    return snode->key;
  }

  return NULL;
}

void
AK_EXPORT
ak_heap_setId(ak_heap * __restrict heap,
              ak_heap_node * __restrict heap_node,
              void * __restrict memId) {
  if (heap_node->flags & AK_HEAP_NODE_FLAGS_SRCH) {
    AkHeapSrchNode *snode;
    snode = (char *)heap_node - sizeof(*snode);

    if (!memId) {
      ak_heap_rb_remove(heap, snode);
      snode->key = ak__emptystr;
    } else {
      snode->key = memId;
      ak_heap_rb_insert(heap, snode);
    }
  }
}

AkResult
AK_EXPORT
ak_heap_getMemById(ak_heap * __restrict heap,
                   void * __restrict memId,
                   void ** __restrict dest) {
  ak_heap_node   *heapNode;
  AkHeapSrchNode *srchNode;

  srchNode = ak_heap_rb_find(heap, memId);
  if (!srchNode || srchNode == heap->srchNullNode) {
    *dest = NULL;
    return AK_EFOUND;
  }

  heapNode = AK__HEAPNODE(srchNode);
  *dest = ak__alignas(heapNode);

  return AK_OK;
}

void
ak_heap_printKeys(ak_heap * __restrict heap) {
  ak_heap_rb_print(heap);
}

AkHeapAllocator *
AK_EXPORT
ak_mem_allocator() {
  return ak__heap.allocator;
}

void
ak_mem_printKeys() {
  ak_heap_rb_print(&ak__heap);
}

void *
AK_EXPORT
ak_mem_getId(void * __restrict memptr) {
  return ak_heap_getId(&ak__heap,
                       ak__alignof(memptr));
}

void
AK_EXPORT
ak_mem_setId(void * __restrict memptr,
             void * __restrict memId) {
  ak_heap_setId(&ak__heap,
                ak__alignof(memptr),
                memId);
}

AkResult
AK_EXPORT
ak_mem_getMemById(void * __restrict memId,
                  void ** __restrict dest) {
  return ak_heap_getMemById(&ak__heap,
                            memId,
                            dest);
}

void
AK_EXPORT
ak_mem_setp(void * __restrict memptr,
            void * __restrict newparent) {
  ak_heap_setp(&ak__heap,
                ak__alignof(memptr),
                ak__alignof(newparent));
}

void*
AK_EXPORT
ak_malloc(void * __restrict parent,
          size_t size,
          bool srch) {
  return ak_heap_alloc(&ak__heap,
                       parent,
                       size,
                       srch);
}

void*
AK_EXPORT
ak_calloc(void * __restrict parent,
          size_t size,
          bool srch) {
  void  *memptr;

  memptr = ak_heap_alloc(&ak__heap,
                         parent,
                         size,
                         srch);
  memset(memptr, '\0', size);

  return memptr;
}

void*
AK_EXPORT
ak_realloc(void * __restrict parent,
           void * __restrict memptr,
           size_t newsize) {
  return ak_heap_realloc(&ak__heap,
                          parent,
                          memptr,
                          newsize);
}

char*
AK_EXPORT
ak_strdup(void * __restrict parent,
          const char * __restrict str) {
  void  *memptr;
  size_t memsize;

  memsize = strlen(str);
  memptr  = ak_heap_alloc(&ak__heap,
                          parent,
                          memsize + 1,
                          false);

  memcpy(memptr, str, memsize);

  /* NULL */
  memset((char *)memptr + memsize, '\0', 1);

  return memptr;
}

void
AK_EXPORT
ak_free(void * __restrict memptr) {
  ak_heap_free(&ak__heap,
                ak__alignof(memptr));
}

AkObject*
AK_EXPORT
ak_objAlloc(void * __restrict memParent,
            size_t typeSize,
            AkEnum typeEnum,
            AkBool zeroed,
            bool srch) {
  AkObject * obj;

  assert(typeSize > 0 && "invalid parameter value");

  obj = ak_heap_alloc(&ak__heap,
                      memParent,
                      sizeof(*obj) + typeSize,
                      srch);

  obj->size  = typeSize;
  obj->type  = typeEnum;
  obj->next  = NULL;
  obj->pData = (void *)((char *)obj + offsetof(AkObject, data));

  if (zeroed)
    memset(obj->pData, '\0', typeSize);

  return obj;
}

AkObject*
AK_EXPORT
ak_objFrom(void * __restrict memptr) {
  AkObject *obj;
  obj = (void *)((char *)memptr - offsetof(AkObject, data));
  assert(obj->pData == memptr && "invalid cas to AkObject");
  return obj;
}

void
ak_CONSTRUCTOR
ak__init() {
  ak_heap_init(&ak__heap, NULL, NULL);
}

void
ak_DESTRUCTOR
ak__cleanup() {
  ak_heap_destroy(&ak__heap);
}
