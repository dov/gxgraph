/*======================================================================
//  moving_ants.c - Implementation of moving ants functionality
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#include "moving_ants.h"
#include <stdio.h>

moving_ants_t *
new_moving_ants (GtkWidget * ref_widget)
{
  moving_ants_t *moving_ants = g_new0 (moving_ants_t, 1);
  int side_idx;
  GdkColor color;

  moving_ants->ref_widget = ref_widget;
  moving_ants->window = ref_widget->window;

  /* Set additional default parameters */
  for (side_idx = 0; side_idx < 4; side_idx++)
    moving_ants->backing_storage[side_idx] = NULL;

  moving_ants->gc_window = gdk_gc_new (moving_ants->window);
  gdk_gc_copy (moving_ants->gc_window,
	       moving_ants->ref_widget->style->
	       bg_gc[GTK_WIDGET_STATE (moving_ants->ref_widget)]);

  /* Set the color of the lasso */
  gdk_color_parse ("Midnight Blue", &color);	/* Color of xgraph */
  gdk_colormap_alloc_color (gdk_colormap_get_system (), &color, FALSE, TRUE);
  gdk_gc_set_foreground (moving_ants->gc_window, &color);

  /* We still don't store anything */
  moving_ants->has_store = FALSE;

  return moving_ants;
}

static void
free_backing_storage (moving_ants_t * moving_ants)
{
  int side_idx;

  for (side_idx = 0; side_idx < 4; side_idx++)
    {
      if (moving_ants->backing_storage[side_idx])
	{
	  gdk_pixmap_unref (moving_ants->backing_storage[side_idx]);
	  gdk_gc_unref (moving_ants->gc_backing_storage[side_idx]);
	  moving_ants->backing_storage[side_idx] = NULL;
	}
    }
}

void
free_moving_ants (moving_ants_t * moving_ants)
{
  free_backing_storage (moving_ants);
  gdk_gc_unref (moving_ants->gc_window);
  g_free (moving_ants);
}

int
moving_ants_store_background (moving_ants_t * moving_ants, int bbox[4])
{
  int side_idx;
  int width = bbox[2] - bbox[0];
  int height = bbox[3] - bbox[1];

  // sanity check
  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;
  // Build the storage coordinates from  the bounding box

  // 0: top
  moving_ants->storage_coords[0].x = bbox[0];
  moving_ants->storage_coords[0].y = bbox[1];
  moving_ants->storage_coords[0].width = width;
  moving_ants->storage_coords[0].height = 1;

  // 1: right side
  moving_ants->storage_coords[1].x = bbox[2];
  moving_ants->storage_coords[1].y = bbox[1];
  moving_ants->storage_coords[1].width = 1;
  moving_ants->storage_coords[1].height = height;

  // 2: bottom side
  moving_ants->storage_coords[2].x = bbox[0];
  moving_ants->storage_coords[2].y = bbox[3];
  moving_ants->storage_coords[2].width = width;
  moving_ants->storage_coords[2].height = 1;

  // 3: left side
  moving_ants->storage_coords[3].x = bbox[0];
  moving_ants->storage_coords[3].y = bbox[1];
  moving_ants->storage_coords[3].width = 1;
  moving_ants->storage_coords[3].height = height;

  // Now get the four areas
  free_backing_storage (moving_ants);
  for (side_idx = 0; side_idx < 4; side_idx++)
    {
      GdkRectangle r = moving_ants->storage_coords[side_idx];
      moving_ants->backing_storage[side_idx]
	= gdk_pixmap_new (moving_ants->window, r.width, r.height, -1);
      moving_ants->gc_backing_storage[side_idx] =
	gdk_gc_new (moving_ants->backing_storage[side_idx]);

      gdk_draw_drawable (moving_ants->backing_storage[side_idx],
			 moving_ants->gc_backing_storage[side_idx],
			 moving_ants->window,
			 r.x, r.y, 0, 0, r.width, r.height);
    }

  moving_ants->has_store = TRUE;

  return 0;
}

int
moving_ants_restore_background (moving_ants_t * moving_ants)
{
  int side_idx;

  for (side_idx = 0; side_idx < 4; side_idx++)
    {
      GdkRectangle r = moving_ants->storage_coords[side_idx];
      gdk_draw_drawable (moving_ants->window,
			 moving_ants->gc_window,
			 moving_ants->backing_storage[side_idx],
			 0, 0, r.x, r.y, r.width, r.height);
    }

  // Get rid of the backing storage
  free_backing_storage (moving_ants);
  moving_ants->has_store = FALSE;

  return 0;
}

int
moving_ants_draw_lasso (moving_ants_t * moving_ants, int bbox[4])
{
  int side_idx;

  if (moving_ants->has_store)
    moving_ants_restore_background (moving_ants);

  moving_ants_store_background (moving_ants, bbox);

  for (side_idx = 0; side_idx < 4; side_idx++)
    {
      GdkRectangle rect = moving_ants->storage_coords[side_idx];

      gdk_draw_rectangle (moving_ants->window,
			  moving_ants->gc_window,
			  TRUE, rect.x, rect.y, rect.width, rect.height);
    }
  return 0;
}
