/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_geomety_fixup.h"
#include "../ak_memory_common.h"

AK_INLINE
_assetkit_hide
AkResult
ak_dae_meshFixupPrimitive(AkHeap          *heap,
                          AkDoc           *doc,
                          AkMeshPrimitive *primitive) {
  AkUIntArray *indices;
  AkUIntArray *newIndices;
  AkInput     *verticeInput;
  AkInput     *input;
  int          inputCount;
  uint32_t     vertOffset;
  size_t       indicesCount;
  size_t       i;

  verticeInput = NULL;
  inputCount   = 0;
  input        = primitive->input;
  indices      = primitive->indices;

  /* stride count */
  while (input) {
    inputCount++;
    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
      verticeInput = input;

    input = (AkInput *)input->base.next;
  }

  if (!verticeInput)
    return AK_ERR;

  vertOffset   = verticeInput->offset;
  indicesCount = indices->count / inputCount;

  /* fix indices for other inputs */
  input = primitive->input;
  while (input) {
    AkSource *source;

    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
      input = (AkInput *)input->base.next;
      continue;
    }

    source = ak_getObjectByUrl(doc, input->base.source);

    /* TODO: INT, DOUBLE.. */
    if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkObject      *obj;
      AkFloatArrayN *oldArray;
      AkFloatArrayN *newArray;
      AkFloat       *oldItems;
      AkFloat       *newItems;
      size_t         j;

      /* TODO: replace 3 with vertex count */
#define vertc 3
      obj = ak_objAlloc(heap,
                        source,
                        sizeof(*newArray)
                        + sizeof(AkFloat) * indicesCount * vertc,
                        AK_SOURCE_ARRAY_TYPE_FLOAT,
                        false,
                        false);

      newArray = ak_objGet(obj);
      oldArray = ak_objGet(source->data);

      oldItems = oldArray->items;
      newItems = newArray->items;

      newArray->count     = indicesCount * vertc;
      newArray->digits    = oldArray->digits;
      newArray->magnitude = oldArray->magnitude;

      if (oldArray->name) {
        newArray->name = oldArray->name;
        ak_mem_setp((void *)oldArray->name, newArray);
      }

      /* fix indices */
      for (i = 0; i < indicesCount; i++) {
        AkUInt index;
        AkUInt index2;

        index  = indices->items[i * inputCount + vertOffset];
        index2 = indices->items[i * inputCount + input->offset];

        for (j = 0; j < vertc; j++)
          newItems[index * vertc + j] = oldItems[index2 * vertc + j];
      }

      ak_free(source->data);
      source->data = obj;
    }

    input = (AkInput *)input->base.next;
  }

  newIndices = ak_heap_alloc(heap,
                             primitive,
                             sizeof(AkFloat) * indicesCount,
                             false);

  newIndices->count = indicesCount;

  /* keep only vertex indices in primitives */
  for (i = 0; i < indicesCount; i++)
    newIndices->items[i] = indices->items[i * inputCount + vertOffset];

  ak_free(indices);

  primitive->indices = newIndices;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_meshFixup(AkMesh * mesh) {
  AkDoc  *doc;
  AkHeap *heap;
  AkMeshPrimitive *primitive;

  heap      = ak_heap_getheap(mesh->vertices);
  doc       = heap->data;
  primitive = mesh->primitive;

  while (primitive) {
    ak_dae_meshFixupPrimitive(heap, doc, primitive);
    primitive = primitive->next;
  }

  /* fixup coord system */
  ak_changeCoordSysMesh(mesh, ak_defaultCoordSys());

  return AK_OK;
}
