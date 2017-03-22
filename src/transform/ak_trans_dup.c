/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include <cglm.h>

AK_EXPORT
void
ak_transformDup(AkNode * __restrict srcNode,
                AkNode * __restrict destNode) {
  AkHeap   *heap;
  AkObject *transform, *newTransform, *lastTransform;

  if (!srcNode || !srcNode->transform || !destNode)
    return;

  heap = ak_heap_getheap(srcNode);
  newTransform = ak_heap_calloc(heap,
                                destNode,
                                sizeof(*newTransform));

  transform     = srcNode->transform;
  lastTransform = NULL;

  while (transform) {
    switch (transform->type) {
      case AK_NODE_TRANSFORM_TYPE_MATRIX: {
        AkMatrix *matrix, *newMatrix;

        matrix = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*matrix),
                                   AK_NODE_TRANSFORM_TYPE_MATRIX,
                                   true);
        newMatrix = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);
        glm_mat4_copy(matrix->val, newMatrix->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_LOOK_AT: {
        AkLookAt *lookAt, *newLookAt;

        lookAt = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*lookAt),
                                   AK_NODE_TRANSFORM_TYPE_LOOK_AT,
                                   true);
        newLookAt = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);

        glm_vec_copy(lookAt->val[0], newLookAt->val[0]);
        glm_vec_copy(lookAt->val[1], newLookAt->val[1]);
        glm_vec_copy(lookAt->val[2], newLookAt->val[2]);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_ROTATE: {
        AkRotate *rotate, *newRotate;

        rotate = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*rotate),
                                   AK_NODE_TRANSFORM_TYPE_ROTATE,
                                   true);
        newRotate = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);
        glm_vec4_copy(rotate->val, newRotate->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_SCALE: {
        AkScale *scale, *newScale;

        scale = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*scale),
                                   AK_NODE_TRANSFORM_TYPE_SCALE,
                                   true);
        newScale = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);
        glm_vec_copy(scale->val, newScale->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_TRANSLATE: {
        AkTranslate *translate, *newTranslate;

        translate = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*translate),
                                   AK_NODE_TRANSFORM_TYPE_TRANSLATE,
                                   true);
        newTranslate = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);
        glm_vec_copy(translate->val, newTranslate->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_SKEW: {
        AkSkew *skew, *newSkew;

        skew = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*skew),
                                   AK_NODE_TRANSFORM_TYPE_SKEW,
                                   true);
        newSkew = ak_objGet(newTransform);
        ak_sid_dup(newTransform, transform);

        newSkew->angle = skew->angle;
        glm_vec_copy(skew->aroundAxis, newSkew->aroundAxis);
        glm_vec_copy(skew->rotateAxis, newSkew->rotateAxis);
        break;
      }
    }

    if (lastTransform)
      lastTransform->next = newTransform;
    else
      destNode->transform = newTransform;

    lastTransform = newTransform;

    transform = transform->next;
  }
}
