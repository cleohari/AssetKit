/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__ak_collada_fx_effect__h_
#define __libassetkit__ak_collada_fx_effect__h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetkit_hide
ak_dae_effect(void * __restrict memParent,
               xmlTextReaderPtr reader,
               ak_effect ** __restrict dest);

int _assetkit_hide
ak_dae_fxEffectInstance(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         ak_effect_instance ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_effect__h_ */
