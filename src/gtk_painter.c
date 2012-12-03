/*======================================================================
//  gtkpainter.c - Creates a gtk painter
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#include <stdio.h>
#include <math.h>
#include "moving_ants.h"
#include "gxgraph.h"
#include "gxgraph_hardcopy.h"
#include "gxgraph_about.h"

#include "pixmap_gxgraph.i"

static gint cb_configure_event (GtkWidget * widget, GdkEventConfigure * event,
				window_t * window);
static gint cb_expose_event (GtkWidget * widget, GdkEventExpose * event,
			     window_t * window);
static gint cb_motion_event (GtkWidget * widget, GdkEventButton * event,
			     gpointer user_data);
static gint cb_button_press_event (GtkWidget * widget, GdkEventButton * event,
				   gpointer user_data);
static gint cb_button_release_event (GtkWidget * widget,
				     GdkEventButton * event,
				     gpointer user_data);
static gint cb_key_press_event (GtkWidget * widget, GdkEventKey * event,
				gpointer user_data);
static gint cb_focus_in_out_event (GtkWidget * widget);

static gint cb_close_window (GtkObject * widget, window_t * window);
static gint cb_hardcopy_dialog (GtkObject * widget, gpointer user_data);
static gint cb_about_dialog (GtkObject * widget, gpointer user_data);

static void create_buttons (window_t * window, GtkWidget * button_box);
static GtkWidget *create_drawing_area (window_t * window, GtkWidget * daddy);
static void
gtk_painter_set_attributes (painter_t * painter,
			    GdkColor color,
			    double line_width,
			    int line_style,
			    gint mark_type,
			    gdouble mark_size_x, gdouble mark_size_y);

static void
gtk_painter_draw_line (painter_t * painter,
		       double x1, double y1, double x2, double y2);
static void
gtk_painter_draw_segments (painter_t * painter, GArray * segments);
static void gtk_painter_draw_marks (painter_t * painter, GArray * marks);
static void
gtk_painter_draw_text (struct painter_t_struct *painter,
		       double x_pos, double y_pos,
		       const char *text, int just, int style);
static void gtk_painter_set_attributes_style (painter_t * painter, int style);

static void gtk_painter_nop ();
static gint cb_zero_on_destroy (GtkObject * widget, gpointer userdata);

typedef struct
{
  painter_t painter;

  // private fields
  GtkWidget *w_toplevel;
  GtkWidget *drawing_area;
  GtkWidget *vbox;
  GtkWidget *button_box;
  PangoLayout *pango_layout;
  PangoContext *pango_context;
  PangoFontDescription *pango_font_description;
  moving_ants_t *moving_ants;
  GdkPixmap *pixmap;
  GtkWidget *gxgraph_hardcopy;
  cairo_t *cr;

  // stateful variables
  gboolean is_defining_zoom_area;
  gint start_cx;
  gint start_cy;
  gint current_mark_type;
  gdouble current_mark_size_x;
  gdouble current_mark_size_y;
  int current_line_style;
} gtk_painter_t;

#define PADDING         2
#define SPACE           10
#define TICKLENGTH      5

painter_t *
gtk_painter_new (window_t * window)
{
  gtk_painter_t *this = g_new0 (gtk_painter_t, 1);
  painter_t *parent = (painter_t *) this;
  const char *font_family = "Sans";
  GError *err = NULL;
  GdkPixbuf *icon = gdk_pixbuf_new_from_inline(sizeof(pixmap_gxgraph_inline),
                                               pixmap_gxgraph_inline,
                                               TRUE,
                                               &err);

  this->w_toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_icon (GTK_WINDOW (this->w_toplevel),
                       icon);
  g_object_unref(icon);
  gtk_widget_set_size_request (this->w_toplevel,
			       window->width, window->height);

  gtk_window_set_policy (GTK_WINDOW (this->w_toplevel), TRUE, TRUE, TRUE);
  this->vbox = gtk_vbox_new (FALSE, 5);
  this->button_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (this->vbox), this->button_box, FALSE, FALSE,
		      0);

  // This should be changed to destroying the window only
  gtk_signal_connect (GTK_OBJECT (this->w_toplevel), "destroy",
		      GTK_SIGNAL_FUNC (cb_close_window), window);

  /* Create callbacks for w_entry */
  this->drawing_area = create_drawing_area (window, this->vbox);

  // Other initializations.
  this->gxgraph_hardcopy = NULL;

#if 0
  // How close to xgraph are we supposed to be??
  {
    GdkColor background_color;
    /* Set background color to xgraph background color */
    gdk_color_parse ("gray70", &background_color);
    gtk_widget_modify_bg (this->drawing_area, GTK_STATE_NORMAL,
			  &background_color);
  }
#endif

  create_buttons (window, this->button_box);
  gtk_container_add (GTK_CONTAINER (this->w_toplevel), this->vbox);

  // Before showing we must assign the pointer...
  window->gtk_painter = (painter_t *) this;

  // Setup pango painting
  this->pango_context = gtk_widget_get_pango_context (this->drawing_area);
  this->pango_layout = pango_layout_new (this->pango_context);
  this->pango_font_description = pango_font_description_new ();
  pango_font_description_set_family (this->pango_font_description,
				     g_strdup (font_family));
  pango_font_description_set_style (this->pango_font_description,
				    PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (this->pango_font_description,
				      PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (this->pango_font_description,
				     PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (this->pango_font_description,
				      PANGO_STRETCH_NORMAL);


  parent->set_attributes = gtk_painter_set_attributes;
  parent->draw_segments = gtk_painter_draw_segments;
  parent->draw_marks = gtk_painter_draw_marks;
  parent->draw_line = gtk_painter_draw_line;
  parent->draw_text = gtk_painter_draw_text;
  parent->set_attributes_style = gtk_painter_set_attributes_style;
  parent->group_start = gtk_painter_nop;
  parent->group_end = gtk_painter_nop;

  parent->area_w = parent->area_h = 0;	/* Set later */
  parent->bdr_pad = PADDING;
  parent->axis_pad = SPACE;
  parent->legend_pad = 0;
  parent->tick_len = TICKLENGTH;
  parent->axis_width = 5;
  parent->axis_height = 13;
  parent->title_width = 5;
  parent->title_height = 5;

  // Create a moving ants structure for the selection of a zoom area
  this->moving_ants = NULL;

  // This will be created in the configure event
  this->pixmap = NULL;

  // Defaults that will be overriden
  this->current_mark_type = 0;
  this->current_mark_size_x = 1.0;
  this->current_mark_size_y = 1.0;

  // Show it all
  gtk_widget_show (this->vbox);
  gtk_widget_show (this->button_box);
  gtk_widget_show (this->w_toplevel);

  return (painter_t *) this;
}

void
gtk_painter_delete (painter_t * painter)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;
  gtk_widget_destroy (gtk_painter->w_toplevel);
}

static void
create_buttons (window_t * window, GtkWidget * button_box)
{
  GtkWidget *button_close, *button_about, *button_hardcopy;

  /* Create buttons */
  button_close = gtk_button_new_with_label ("Close");
  button_hardcopy = gtk_button_new_with_label ("Hardcopy");
  button_about = gtk_button_new_with_label ("About");

  /* Pack them */
  gtk_box_pack_start (GTK_BOX (button_box), button_close, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (button_box), button_hardcopy, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (button_box), button_about, FALSE, FALSE, 0);

  /* Show them */
  gtk_widget_show (button_close);
  gtk_widget_show (button_about);
  gtk_widget_show (button_hardcopy);

  /* Connect buttons to actions */
  gtk_signal_connect (GTK_OBJECT (button_close), "clicked",
		      GTK_SIGNAL_FUNC (cb_close_window), window);
  gtk_signal_connect (GTK_OBJECT (button_hardcopy), "clicked",
		      GTK_SIGNAL_FUNC (cb_hardcopy_dialog), window);
  gtk_signal_connect (GTK_OBJECT (button_about), "clicked",
		      GTK_SIGNAL_FUNC (cb_about_dialog), window);
}

static GtkWidget *
create_drawing_area (window_t * window, GtkWidget * daddy)
{
  /* Drawing area */
  GtkWidget *drawing_area = gtk_drawing_area_new ();
  gtk_widget_show (drawing_area);

  gtk_box_pack_start (GTK_BOX (daddy), drawing_area, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
		      G_CALLBACK (cb_configure_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      G_CALLBACK (cb_expose_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "motion_notify_event",
		      G_CALLBACK (cb_motion_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "key_press_event",
		      G_CALLBACK (cb_key_press_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
		      G_CALLBACK (cb_button_press_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_release_event",
		      G_CALLBACK (cb_button_release_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "focus_in_event",
		      G_CALLBACK (cb_focus_in_out_event), window);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "focus_out_event",
		      G_CALLBACK (cb_focus_in_out_event), window);

  gtk_widget_set_events (GTK_WIDGET (drawing_area),
			 GDK_EXPOSURE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK);

  GTK_WIDGET_SET_FLAGS (drawing_area, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (drawing_area);

  return drawing_area;
}

static gint
cb_configure_event (GtkWidget * widget,
		    GdkEventConfigure * event, window_t * window)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;
  painter_t *painter = (painter_t *) window->gtk_painter;
  int width = widget->allocation.width;
  int height = widget->allocation.height;
  double min_x = window->world.x0;
  double max_x = window->world.x1;
  double min_y = window->world.y0;
  double max_y = window->world.y1;

  window->width = width;
  window->height = height;
  painter->area_w = width;
  painter->area_h = height;
  window->world.scale_x = 1.0 * (width - 40) / (max_x - min_x);
  window->world.scale_y = 1.0 * -(height - 40) / (max_y - min_y);
  window->world.row_0 = height - 20;
  if (gtk_painter->moving_ants)
    free_moving_ants (gtk_painter->moving_ants);

  gtk_painter->moving_ants = new_moving_ants (gtk_painter->drawing_area);

  // Create the backing store pixmap
  if (gtk_painter->pixmap)
    gdk_pixmap_unref (gtk_painter->pixmap);
  gtk_painter->pixmap = gdk_pixmap_new (widget->window, width, height, -1);

  
  gtk_painter->cr = gdk_cairo_create(GDK_DRAWABLE(gtk_painter->pixmap));
  gdk_cairo_set_source_color(gtk_painter->cr,&widget->style->bg[GTK_STATE_NORMAL]);
  cairo_rectangle(gtk_painter->cr,
                  0,0,
                  widget->allocation.width, widget->allocation.height);
  cairo_fill(gtk_painter->cr);

  gxgraph_draw_window (window, NULL);

  return TRUE;
}

static gint
cb_expose_event (GtkWidget * widget, GdkEventExpose * event,
		 window_t * window)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;

  gdk_draw_drawable (widget->window,
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     gtk_painter->pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

  return FALSE;
}

static gint
cb_motion_event (GtkWidget * widget,
		 GdkEventButton * event, gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;

  int cx = event->x;
  int cy = event->y;

  if (gtk_painter->is_defining_zoom_area)
    {
      int bbox[4];

      bbox[0] = gtk_painter->start_cx;
      bbox[1] = gtk_painter->start_cy;
      bbox[2] = cx;
      bbox[3] = cy;

      if (bbox[0] > bbox[2])
	{
	  int tmp = bbox[0];
	  bbox[0] = bbox[2];
	  bbox[2] = tmp;
	}

      if (bbox[1] > bbox[3])
	{
	  int tmp = bbox[1];
	  bbox[1] = bbox[3];
	  bbox[3] = tmp;
	}

      moving_ants_draw_lasso (gtk_painter->moving_ants, bbox);
    }

  return TRUE;
}

static gint
cb_key_press_event (GtkWidget * widget, GdkEventKey * event,
		    gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) (window->gtk_painter);
  gint k = event->keyval;

  switch (k)
    {
    case 'c':
    case 'C':
      gtk_widget_destroy (gtk_painter->w_toplevel);
      break;
    case 'q':
    case 'Q':
      gtk_main_quit ();
      break;
    }
  return 1;
}

//======================================================================
//  The following definition prevents the default redrawing of the
//  whole drawing_area widget on focus in and focus out.
//----------------------------------------------------------------------
static gint
cb_focus_in_out_event (GtkWidget * widget)
{
  return TRUE;
}

static gint
cb_button_press_event (GtkWidget * widget,
		       GdkEventButton * event, gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;
  gboolean is_signal_caught = FALSE;

  int button = event->button;
  int cx = event->x;
  int cy = event->y;

  if (button == 1)
    {
      is_signal_caught = TRUE;
      gtk_painter->is_defining_zoom_area = TRUE;
      gtk_painter->start_cx = cx;
      gtk_painter->start_cy = cy;
    }

  return is_signal_caught;
}

/* Opposite definitions to SCREENX and SCREENY */
#define WORLDX(window, screenX) \
    (screenX - window->org_x) * window->world.scale_x + window->world_org_x
#define WORLDY(window, screenY) \
    (window->opp_y - screenY) * window->world.scale_y + window->world_org_y

static gint
cb_button_release_event (GtkWidget * widget,
			 GdkEventButton * event, gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;

  int button = event->button;
  int cx = event->x;
  int cy = event->y;

  if (!gtk_painter->is_defining_zoom_area)
    return FALSE;

  gtk_painter->is_defining_zoom_area = FALSE;

  if (button == 1)
    {
      double wx0, wx1, wy0, wy1;
      world_t world;

      moving_ants_restore_background (gtk_painter->moving_ants);

      // Convert coordinates to world coordinates
      wx0 = WORLDX (window, gtk_painter->start_cx);
      wy0 = WORLDY (window, gtk_painter->start_cy);
      wx1 = WORLDX (window, cx);
      wy1 = WORLDY (window, cy);

      if (wx0 > wx1)
	{
	  double tmp = wx1;
	  wx1 = wx0;
	  wx0 = tmp;
	}
      if (wy0 > wy1)
	{
	  double tmp = wy1;
	  wy1 = wy0;
	  wy0 = tmp;
	}

      world.x0 = wx0;
      world.y0 = wy0;
      world.x1 = wx1;
      world.y1 = wy1;

      gxgraph_add_window_with_world (window, &world);
    }

  return TRUE;
}

static gint
cb_close_window (GtkObject * widget, window_t * window)
{
  if (window->gtk_painter != NULL)
    window_delete (window);

  return 0;
}

static gint
cb_hardcopy_dialog (GtkObject * widget, gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;

  if (!gtk_painter->gxgraph_hardcopy)
    {
      gtk_painter->gxgraph_hardcopy =
	gxgraph_hardcopy_dialog_new (gtk_painter->w_toplevel, window);
      gtk_widget_show (gtk_painter->gxgraph_hardcopy);

      g_signal_connect (gtk_painter->gxgraph_hardcopy,
			"destroy",
			G_CALLBACK (cb_zero_on_destroy),
			&gtk_painter->gxgraph_hardcopy);
    }
  else
    gtk_widget_destroy (gtk_painter->gxgraph_hardcopy);

  return 0;
}

static gint
cb_about_dialog (GtkObject * widget, gpointer user_data)
{
  window_t *window = (window_t *) user_data;
  gtk_painter_t *gtk_painter = (gtk_painter_t *) window->gtk_painter;

  gxgraph_about_dialog (gtk_painter->w_toplevel);

  return 0;
}

static void
gtk_painter_set_attributes_style (painter_t * painter, int style)
{
  char *color_name;
  GdkColor color;
  int line_width;
  int line_style = 0;

  switch (style)
    {
    case L_AXIS:
      color_name = "black";
      line_width = 1;
      break;
    case L_GRID:
      color_name = "gray80";
      line_width = 1;
      break;
    case L_ZERO:
      color_name = "white";
      line_width = 2;
      break;
    }
  gdk_color_parse (color_name, &color);
  gtk_painter_set_attributes (painter,
			      color, line_width, line_style, 0, 1.0, 1.0);
}

static void
gtk_painter_set_attributes (painter_t * painter,
			    GdkColor color,
			    double line_width,
			    int line_style,
			    gint mark_type,
			    gdouble mark_size_x, gdouble mark_size_y)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;
  cairo_set_line_width(gtk_painter->cr, line_width);
  cairo_set_line_cap(gtk_painter->cr, CAIRO_LINE_CAP_ROUND);
  gdk_cairo_set_source_color(gtk_painter->cr, &color);
  gtk_painter->current_line_style = line_style;

  gtk_painter->current_mark_type = mark_type;
  gtk_painter->current_mark_size_x = mark_size_x;
  gtk_painter->current_mark_size_y = mark_size_y;
}

static void
gtk_painter_draw_line (painter_t * painter,
		       double x1, double y1, double x2, double y2)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;

  cairo_move_to(gtk_painter->cr, x1,y1);
  cairo_line_to(gtk_painter->cr, x2,y2);
  cairo_stroke(gtk_painter->cr);
}

static void
gtk_painter_draw_segments (painter_t * painter, GArray * segments)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;
  seg_t *segs = (seg_t *) segments->data;
  int seg_idx;

  // Translate segments to gtk segments just by rounding
  for (seg_idx = 0; seg_idx < segments->len; seg_idx++)
    {
      cairo_move_to(gtk_painter->cr, segs[seg_idx].x1,segs[seg_idx].y1);
      cairo_line_to(gtk_painter->cr, segs[seg_idx].x2,segs[seg_idx].y2);
    }
  cairo_stroke(gtk_painter->cr);
}

static void
draw_one_mark (GdkWindow * drawable,
               cairo_t *cr,
	       int x, int y,
               int mark_type,
               double size_x,
               double size_y,
               gboolean *need_stroke,
               gboolean *need_fill)
{
  double rx=size_x/2, ry=size_y/2; // Mark size
  if (mark_type == MARK_TYPE_CIRCLE) {
    cairo_move_to(cr, x+rx,y);
    cairo_arc(cr,
              x, y,
              rx, 0.0, 2*G_PI);
    *need_stroke = 1;
  }
  else if (mark_type == MARK_TYPE_FCIRCLE) {
    cairo_move_to(cr, x+rx,y);
    cairo_arc(cr,
              x, y,
              rx, 0.0, 2*G_PI);
    *need_fill = 1;
  }
  else if (mark_type == MARK_TYPE_SQUARE) {
    cairo_move_to(cr, x-rx,y-ry);
    cairo_rectangle(cr, x-rx,y-ry,2*rx,2*ry);
    *need_stroke = 1;
  }
  else if (mark_type == MARK_TYPE_FSQUARE) {
    cairo_rectangle(cr, x-rx,y-ry,2*rx,2*ry);
    *need_fill = 1;
  }
}

static void
gtk_painter_draw_marks (painter_t * painter, GArray * marks_array)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;
  mark_t *marks = (mark_t *) marks_array->data;
  int m_idx;
  gboolean need_stroke = FALSE;
  gboolean need_fill = FALSE;

  for (m_idx = 0; m_idx < marks_array->len; m_idx++)
    draw_one_mark (gtk_painter->pixmap,
		   gtk_painter->cr,
		   marks[m_idx].x,
		   marks[m_idx].y,
		   gtk_painter->current_mark_type,
		   gtk_painter->current_mark_size_x,
		   gtk_painter->current_mark_size_y,
                   &need_stroke,
                   &need_fill);
  if (need_stroke)
    cairo_stroke(gtk_painter->cr);
  if (need_fill)
    cairo_fill(gtk_painter->cr);
}

static void
gtk_painter_draw_text (struct painter_t_struct *painter,
		       double x_pos, double y_pos,
		       const char *text, int just, int style)
{
  gtk_painter_t *gtk_painter = (gtk_painter_t *) painter;
  double text_size = 8;
  GdkColor gc_black = { 0, 0, 0, 0 };
  int layout_width, layout_height;
  PangoRectangle log_rect, ink_rect;

  // This should be more configurable...
  if (style == T_TITLE)
    text_size = 18;

  pango_font_description_set_size (gtk_painter->pango_font_description,
				   (int) (text_size) * PANGO_SCALE);
  pango_context_set_font_description (gtk_painter->pango_context,
				      gtk_painter->pango_font_description);

  pango_layout_set_text (gtk_painter->pango_layout, text, -1);

  pango_layout_get_pixel_extents (gtk_painter->pango_layout,
				  &ink_rect, &log_rect);
  layout_width = log_rect.width;
  layout_height = log_rect.height;

  gdk_cairo_set_source_color(gtk_painter->cr, &gc_black);

  if (just == T_RIGHT)
    {
      x_pos -= layout_width;
      y_pos -= layout_height / 2;
    }
  else if (just == T_LOWERLEFT)
    y_pos -= layout_height;
  else if (just == T_UPPERLEFT)
    y_pos -= 0;
  else if (just == T_BOTTOM)
    {
      y_pos -= layout_height;
      x_pos -= layout_width / 2;
    }
  else if (just == T_TOP)
    x_pos -= layout_width / 2;


  cairo_move_to(gtk_painter->cr,x_pos,y_pos);
  pango_cairo_show_layout(gtk_painter->cr,gtk_painter->pango_layout);
}

void
gtk_painter_nop ()
{
}

static gint
cb_zero_on_destroy (GtkObject * widget, gpointer userdata)
{
  void **userpointer = (void **) userdata;

  *userpointer = NULL;

  return FALSE;
}
