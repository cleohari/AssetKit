/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_postscript.h"
#include "ak_collada_geom_fixup.h"
#include "../ak_memory_common.h"
#include "../ak_libxml.h"

void _assetkit_hide
ak_dae_retain_refs(AkXmlState * __restrict xst);

void _assetkit_hide
ak_dae_postscript(AkXmlState * __restrict xst) {
  ak_dae_retain_refs(xst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
    ak_dae_geom_fixup_all(xst->doc);

  /* now set used coordSys */
  if (!ak_opt_get(AK_OPT_USE_DOC_COORD))
    xst->doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);
}

void _assetkit_hide
ak_dae_retain_refs(AkXmlState * __restrict xst) {
  AkHeapAllocator *alc;
  AkURLQueue      *it, *tofree;
  AkURL           *url;
  AkHeapNode      *hnode;
  size_t          *refc;
  AkResult         ret;

  alc = xst->heap->allocator;
  it  = xst->urlQueue;

  while (it) {
    url    = it->url;
    tofree = it;

    /* currently only retain objects in this doc */
    if (it->url->doc == xst->doc) {
      ret = ak_heap_getNodeByURL(xst->heap, url, &hnode);
      if (ret == AK_OK) {
        /* retain <source> and source arrays ... */
        refc = ak_heap_ext_add(xst->heap,
                               hnode,
                               AK_HEAP_NODE_FLAGS_REFC);
        (*refc)++;
      }
    }

    it = it->next;
    alc->free(tofree);
  }
}
