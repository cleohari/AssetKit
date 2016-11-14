/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_pass.h"
#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_annotate.h"

#include "ak_collada_fx_states.h"
#include "ak_collada_fx_program.h"
#include "ak_collada_fx_evaluate.h"

AkResult _assetkit_hide
ak_dae_fxPass(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkPass ** __restrict dest) {
  AkPass     *pass;
  AkAnnotate *last_annotate;

  pass = ak_heap_calloc(xst->heap,
                        memParent,
                        sizeof(*pass));

  pass->sid = ak_xml_attr(xst, pass, _s_dae_sid);

  last_annotate = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_pass))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, pass, &assetInf);
      if (ret == AK_OK)
        pass->inf = assetInf;
    } else if (ak_xml_eqelm(xst, _s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(xst, pass, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          pass->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_states)) {
      AkStates *states;
      AkResult  ret;

      ret = ak_dae_fxState(xst, pass, &states);
      if (ret == AK_OK)
        pass->states = states;

    } else if (ak_xml_eqelm(xst, _s_dae_program)) {
      AkProgram *prog;
      AkResult   ret;

      ret = ak_dae_fxProg(xst, pass, &prog);
      if (ret == AK_OK)
        pass->program = prog;
    } else if (ak_xml_eqelm(xst, _s_dae_evaluate)) {
      AkEvaluate * evaluate;
      AkResult ret;

      ret = ak_dae_fxEvaluate(xst, pass, &evaluate);
      if (ret == AK_OK)
        pass->evaluate = evaluate;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          pass,
                          nodePtr,
                          &tree,
                          NULL);
      pass->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  *dest = pass;
  
  return AK_OK;
}
