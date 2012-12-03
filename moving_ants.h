/*======================================================================
//  moving_ants.h - A class for dealing with the selection of a
//                  rectangular area.
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#ifndef MOVING_ANTS_H
#define MOVING_ANTS_H

#include <gtk/gtk.h>

typedef struct
{
  GtkWidget *ref_widget;
  GdkDrawable *window;
  GdkPixmap *backing_storage[4];	// top, right, bottom, left
  GdkGC *gc_backing_storage[4];	// Probably do not need to keep these around
  GdkRectangle storage_coords[4];	// Coordinates of storage
  GdkGC *gc_window;
  gboolean has_store;
} moving_ants_t;

/** 
 * Used to store a rectangular region into a moving ants structure.
 * 
 * @param moving_ants 
 * @param bbox 
 * 
 * @return 
 */
moving_ants_t *new_moving_ants (GtkWidget * ref_widget);
int moving_ants_store_background (moving_ants_t * moving_ants, int bbox[4]);
int moving_ants_draw_lasso (moving_ants_t * moving_ants, int bbox[4]);
int moving_ants_restore_background (moving_ants_t * moving_ants);
void free_moving_ants (moving_ants_t * moving_ants);


#endif /* MOVING_ANTS */
