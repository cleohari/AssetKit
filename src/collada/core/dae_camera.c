/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_camera.h"
#include "dae_asset.h"
#include "dae_technique.h"

_assetkit_hide
void
dae_cam(xml_t * __restrict xml,
        void  * __restrict userdata) {
  DAEState    *dst;
  AkCamera    *camera;
  AkDoc       *doc;
  AkHeap      *heap;
  AkTechnique *tq;

  dst  = userdata;
  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  while (xml) {
    camera       = ak_heap_calloc(heap, doc, sizeof(*camera));
    camera->name = xmla_strdup_by(xml, heap, _s_dae_name, camera);

    xmla_setid(camera, heap, xml);

    if (xml_tag_eq(xml, _s_dae_asset)) {
      dae_asset(xml, dst);
    } else if (xml_tag_eq(xml, _s_dae_optics)) {
      AkOptics *optics;
      xml_t    *xoptics;

      optics  = ak_heap_calloc(heap, camera, sizeof(*optics));
      xoptics = xml->val;

      while (xoptics) {
        if (xml_tag_eq(xoptics, _s_dae_techniquec)) {
          xml_t *xcam, *xtech, *xtechv;
          
          xtech = xoptics->val;

          while (xtech) {
            if (xml_tag_eq(xtech, _s_dae_perspective)) {
              AkPerspective *persp;
              
              persp  = ak_heap_calloc(heap, optics, sizeof(*persp));
              xtechv = xtech->val;

              while (xtechv) {
                if (xml_tag_eq(xtechv, _s_dae_xfov)) {
                  sid_seta(xtechv, heap, persp, &persp->xfov);
                  persp->xfov = glm_rad(xml_float(xtechv, 0.0f));
                } else if (xml_tag_eq(xtechv, _s_dae_yfov)) {
                  sid_seta(xtechv, heap, persp, &persp->yfov);
                  persp->yfov = glm_rad(xml_float(xtechv, 0.0f));
                } else if (xml_tag_eq(xtechv, _s_dae_aspect_ratio)) {
                  sid_seta(xtechv, heap, persp, &persp->aspectRatio);
                  persp->aspectRatio = xml_float(xtechv, 0.0f);
                } else if (xml_tag_eq(xtechv, _s_dae_znear)) {
                  sid_seta(xtechv, heap, persp, &persp->znear);
                  persp->znear = xml_float(xtechv, 0.0f);
                } else if (xml_tag_eq(xtechv, _s_dae_zfar)) {
                  sid_seta(xtechv, heap, persp, &persp->zfar);
                  persp->zfar = xml_float(xtechv, 0.0f);
                }
                xtechv = xtechv->next;
              }
              
              persp->base.type = AK_PROJECTION_PERSPECTIVE;
              if (!persp->aspectRatio && persp->yfov && persp->xfov) {
                persp->aspectRatio = persp->xfov / persp->yfov;
              } else if (!persp->yfov && persp->aspectRatio && persp->xfov) {
                persp->yfov = persp->xfov / persp->aspectRatio;
              } else if (!persp->xfov && persp->aspectRatio && persp->yfov) {
                persp->xfov = persp->yfov * persp->aspectRatio;
              }
              
              optics->tcommon = &persp->base;
            } else if (xml_tag_eq(xcam, _s_dae_orthographic)) {
              AkOrthographic *ortho;
              
              ortho = ak_heap_calloc(heap, optics, sizeof(*ortho));
              xtechv = xtech->val;
              
              while (xtechv) {
                if (xml_tag_eq(xtechv, _s_dae_xmag)) {
                  sid_seta(xtechv, heap, ortho, &ortho->xmag);
                  ortho->xmag = glm_rad(xml_float(xtechv, 0.0f));
                } else if (xml_tag_eq(xtechv, _s_dae_ymag)) {
                  sid_seta(xtechv, heap, ortho, &ortho->ymag);
                  ortho->ymag = glm_rad(xml_float(xtechv, 0.0f));
                } else if (xml_tag_eq(xtechv, _s_dae_aspect_ratio)) {
                  sid_seta(xtechv, heap, ortho, &ortho->aspectRatio);
                  ortho->aspectRatio = xml_float(xtechv, 0.0f);
                } else if (xml_tag_eq(xtechv, _s_dae_znear)) {
                  sid_seta(xtechv, heap, ortho, &ortho->znear);
                  ortho->znear = xml_float(xtechv, 0.0f);
                } else if (xml_tag_eq(xtechv, _s_dae_zfar)) {
                  sid_seta(xtechv, heap, ortho, &ortho->zfar);
                  ortho->zfar = xml_float(xtechv, 0.0f);
                }
                xtechv = xtechv->next;
              }

              ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;
              if (!ortho->aspectRatio && ortho->ymag && ortho->xmag) {
                ortho->aspectRatio = ortho->xmag / ortho->ymag;
              } else if (!ortho->ymag && ortho->aspectRatio && ortho->xmag) {
                ortho->ymag = ortho->xmag / ortho->aspectRatio;
              } else if (!ortho->xmag && ortho->aspectRatio && ortho->ymag) {
                ortho->xmag = ortho->ymag * ortho->aspectRatio;
              }
              
              optics->tcommon = &ortho->base;
            }
            xtech = xtech->next;
          }
        } else if (xml_tag_eq(xoptics, _s_dae_technique)) {
          tq                = dae_technique(xoptics, heap, optics);
          tq->next          = optics->technique;
          optics->technique = tq;
        }
        xoptics = xoptics->next;
      }

      camera->optics = optics;
    } else if (xml_tag_eq(xml, _s_dae_imager)) {
      AkImager *imager;
      xml_t    *ximager;

      imager  = ak_heap_calloc(heap, camera, sizeof(*imager));
      ximager = xml->val;

      while (ximager) {
        if (xml_tag_eq(ximager, _s_dae_technique)) {
          tq                = dae_technique(ximager, heap, imager);
          tq->next          = imager->technique;
          imager->technique = tq;
        } else if (xml_tag_eq(ximager, _s_dae_extra)) {
          imager->extra = tree_fromxml(heap, imager, xml);
        }
        ximager = ximager->next;
      }

      camera->imager = imager;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      camera->extra = tree_fromxml(heap, camera, xml);
    }

    xml = xml->next;
  }
}
