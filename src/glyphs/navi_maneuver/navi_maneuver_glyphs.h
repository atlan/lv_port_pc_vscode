#pragma once
/* src/ui/glyphs/navi_maneuver/navi_maneuver_glyphs.h
 *
 * LVGL CF_ALPHA_8BIT Manöver-Symbole (300×300 px) für Valhalla-Routentypen.
 * Farbe beim Rendern per lv_obj_set_style_img_recolor() setzen.
 *
 * Valhalla type → Symbol:
 *   4  = Ziel (map-marker-check)
 *   5  = Scharf rechts (map-marker-right)
 *   6  = Scharf links  (map-marker-left)
 *   8  = Geradeaus      (arrow-up-bold)
 *   9  = Halblinks (leicht)  (arrow-right-top-bold)
 *   10 = Halbrechts (leicht) (arrow-right-bold)
 *   11 = Rechts abbiegen (arrow-right-bold)
 *   12 = U-Turn rechts  (arrow-u-down-right)
 *   13 = U-Turn links   (arrow-u-down-left)
 *   14 = Links abbiegen (arrow-left-bold)
 *   15 = Halblinks (leicht) (arrow-left-top-bold)
 *   16 = Halbrechts scharf  (arrow-left-bold)
 *   20 = Auffahrt rechts    (arrow-top-right-thick)
 *   21 = Auffahrt links     (arrow-top-left-thick)
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#  include "lvgl.h"
#else
#  include "lvgl/lvgl.h"
#endif

/* LVGL 8/9 Kompatibilität -------------------------------------------------- */
#if LVGL_VERSION_MAJOR >= 9
#  define _NAVI_DECL(sym)  LV_IMAGE_DECLARE(sym)
   typedef lv_image_dsc_t _navi_img_dsc_t;
#else
#  define _NAVI_DECL(sym)  LV_IMG_DECLARE(sym)
   typedef lv_img_dsc_t    _navi_img_dsc_t;
#endif

_NAVI_DECL(map_marker_check);
_NAVI_DECL(map_marker_right);
_NAVI_DECL(map_marker_left);
_NAVI_DECL(arrow_up_bold);
_NAVI_DECL(arrow_right_top_bold);
_NAVI_DECL(arrow_right_bold);
_NAVI_DECL(arrow_u_down_right);
_NAVI_DECL(arrow_u_down_left);
_NAVI_DECL(arrow_left_bold);
_NAVI_DECL(arrow_left_top_bold);
_NAVI_DECL(arrow_top_right_thick);
_NAVI_DECL(arrow_top_left_thick);

/**
 * Gibt den passenden Descriptor-Zeiger für einen Valhalla-Manövertyp zurück.
 * Gibt NULL zurück wenn kein Symbol für den Typ vorhanden ist.
 */
static inline const _navi_img_dsc_t *navi_maneuver_icon(int valhalla_type)
{
    switch (valhalla_type)
    {
    case 4:  return &map_marker_check;
    case 5:  return &map_marker_right;
    case 6:  return &map_marker_left;
    case 8:  return &arrow_up_bold;
    case 9:  return &arrow_right_top_bold;
    case 10: return &arrow_right_bold;
    case 11: return &arrow_right_bold;
    case 12: return &arrow_u_down_right;
    case 13: return &arrow_u_down_left;
    case 14: return &arrow_left_bold;
    case 15: return &arrow_left_top_bold;
    case 16: return &arrow_left_top_bold;
    case 20: return &arrow_top_right_thick;
    case 21: return &arrow_top_left_thick;
    default: return NULL;
    }
}

#ifdef __cplusplus
}
#endif
