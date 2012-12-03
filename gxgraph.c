/*======================================================================
//  A convertion of the spirit of xgraph into gtk.
//----------------------------------------------------------------------
*/
#include <math.h>
#include <gdk/gdktypes.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "gxgraph.h"
#include "gtk_painter.h"
#include "parser.h"

#ifndef HUGE
#define HUGE 1e-100
#endif

#define MAXBUFSIZE 1024

void die (const char *fmt, ...);
static void read_data_sets (int argp, char *argv[]);
window_t *new_window (window_t * previous_window);
static void put_datasets_in_window (dataset_t * datasets,
				    window_t * window, world_t * world);
static void gxgraph_init();
void gxgraph_draw_title (window_t * window, painter_t * painter);
void gxgraph_draw_legend (window_t * window, painter_t * painter);
void gxgraph_draw_grid_and_axis (window_t * window, painter_t * painter);
void gxgraph_draw_data (window_t * window, painter_t * painter);
int compute_transform (window_t * window, painter_t * painter);
double step_grid ();
double init_grid (double low, double step, int logFlag);
double round_Up (double val);
void write_value (char *str,	/* String to write into */
		  double val,	/* Value to print       */
		  int expv,	/* Exponent             */
		  int axis_num,	/* xaxis = 0, yaxis = 1 */
		  int logFlag);	/* is this a log axis?  */
double round_up (double val);

#define nlog10(x)      (x == 0.0 ? 0.0 : log10(x) + 1e-15)
#define SCREENX(window, userX)						\
  (((((userX) - window->world_org_x)/window->world.scale_x)) + window->org_x)
#define SCREENY(window, userY)						\
  (window->opp_y - ((((userY) - window->world_org_y)/window->world.scale_y)))
#define ZERO_THRESH 1e-7

/* Define an enum for tri-state variables that will be used to set
   properties so that they may use either the default bahaviour or
   be explicitely set.
*/
enum {
  DEFAULT = -1
};
  
/* Global variables */
window_t *first_window;
dataset_t *first_dataset = NULL;

/* Global parameter values */
gboolean  prm_do_draw_ticks = FALSE;
gchar    *prm_x_unit_text = "X";
gchar    *prm_y_unit_text = "Y";
gboolean  prm_do_draw_bounding_box = FALSE;

/* low > hi, means set based on data, like in xgraph */
gdouble   prm_x_low_limit = 1;
gdouble   prm_x_hi_limit = 0;
gdouble   prm_y_low_limit = 1;
gdouble   prm_y_hi_limit = 0;

gboolean default_draw_lines = TRUE;
gboolean default_draw_marks = FALSE;
gboolean default_scale_marks = FALSE;
gint default_mark_type = 1;
gint default_render_type = -1;
gdouble default_line_width = 0;
gdouble default_mark_size = 7;
int num_datasets = 0;
gchar *prm_title_text = NULL;
gchar *prm_xfmt = "%.2f";
gchar *prm_yfmt = "%.2f";
gboolean prm_do_logx = FALSE;
gboolean prm_do_logy = FALSE;
GArray *prm_override_names = NULL;
gint prm_requested_width = 600;
gint prm_requested_height = 600;

GdkColor set_colors[] = { {0, 0xffff, 0, 0},
{0, 0, 0xffff, 0},
{0, 0, 0, 0xffff},
{0, 0xffff, 0xffff, 0},
{0, 0, 0xffff, 0xffff},
{0, 0xffff, 0, 0xffff}
};
gint nset_colors = 6;

#define CASE(s) if (strcmp(s, S_) == 0)

int
main (int argc, char *argv[])
{
  int argp = 1;
  gtk_init (&argc, &argv);
  gxgraph_init();
  
  /* Parse the rest of the command line */
  while (argp < argc && (argv[argp][0] == '-' || argv[argp][0] == '='))
    {
      char *S_ = argv[argp++];

      CASE ("-help")
	{
	  printf ("gxgraph - Draw x-y plots\n"
		  "\n"
		  "Syntax:\n"
		  "    gxgraph [-P] [-nl] [-t t] [-xfmt xfmt] [-yfmt yfmt]\n"
		  "            [-lnx] [-lny] [-0 0-name] [-1 1-name] ...\n"
		  "            =WxH data1 data2 data3\n");
	  exit (0);
	};
      CASE ("-P")
	{
	  default_draw_marks = TRUE;
	  continue;
	}
      CASE ("-nl")
	{
	  default_draw_lines = FALSE;
	  continue;
	}
      CASE ("-t")
	{
	  if (prm_title_text)
	    g_free(prm_title_text);
	  prm_title_text = argv[argp++];
	  continue;
	}
      CASE ("-xfmt")
	{
	  prm_xfmt = argv[argp++];
	  continue;
      }
      CASE ("-yfmt")
	{
	prm_yfmt = argv[argp++];
	continue;
      }
      CASE ("-lnx")
	{
	  prm_do_logx = TRUE;
	  continue;
	}
      CASE ("-lny")
	{
	  prm_do_logy = TRUE;
	  continue;
	}
      CASE ("-bar")
	{
	  die ("Sorry! Bar graphs are not supported yet in gxgraph.\n");
	  continue;
	}
      CASE ("-x")
	{
	  prm_x_unit_text = argv[argp++];
	  continue;
	}
      CASE("-y")
	{
	  prm_y_unit_text = argv[argp++];
	  continue;
	}
      CASE("-lx")
	{
	  /* Support both the command and the two parameter notation */
	  char *lim_param = argv[argp++];
	  double low,hi;

	  if (split_string_to_double_pair(lim_param,
					  /* output */
					  &low, &hi) == 0)
	    {
	      prm_x_low_limit = low;
	      prm_x_hi_limit = hi;
	    }
	  else
	    {
	      prm_x_low_limit = atof(lim_param);
	      prm_x_hi_limit = atof(argv[argp++]);
	    }
	  continue;
	}
      CASE("-ly")
	{
	  /* Support both the command and the two parameter notation */
	  char *lim_param = argv[argp++];
	  double low,hi;

	  if (split_string_to_double_pair(lim_param,
					  /* output */
					  &low, &hi) == 0)
	    {
	      prm_y_low_limit = low;
	      prm_y_hi_limit = hi;
	    }
	  else
	    {
	      prm_y_low_limit = atof(lim_param);
	      prm_y_hi_limit = atof(argv[argp++]);
	    }
	  continue;
	}
	
      /* -%d for set names */
      {
	int set_idx;
	if (sscanf (S_, "-%d", &set_idx) == 1)
	  {
	    char *name = argv[argp++];
	    
	    if (!prm_override_names)
	      prm_override_names = g_array_new (TRUE, TRUE, sizeof (char *));
	    if (prm_override_names->len < set_idx)
	      prm_override_names =
		g_array_set_size (prm_override_names, set_idx);
	    g_array_insert_val (prm_override_names, set_idx, name);
	    continue;
	  }
      }
      /* = for geometry - tbd */
      {
	int geom_width, geom_height;
	if (sscanf (S_, "=%dx%d", &geom_width, &geom_height))
	  {
	    prm_requested_height = geom_height;
	    prm_requested_width = geom_width;
	    continue;
	  }
      }

      die ("Unknown option %s!\n", S_);
    }

  /* Get filename */
  read_data_sets (argc - argp, &argv[argp]);

  first_window = new_window (NULL);
  put_datasets_in_window (first_dataset, first_window, NULL);

  gtk_main ();

  return 0;
}

static void gxgraph_init()
{
  prm_title_text = g_strdup("gxgraph");
}

static dataset_t *
new_dataset (int set_idx, const char *filename)
{
  dataset_t *dataset_p = (dataset_t *) g_malloc (sizeof (dataset_t));

  dataset_p->points = g_array_new (FALSE, FALSE, sizeof (point_t));
  dataset_p->next_dataset = NULL;
  dataset_p->do_draw_marks = DEFAULT;
  dataset_p->do_draw_lines = DEFAULT;
  dataset_p->do_draw_polygon = FALSE;
  dataset_p->do_draw_polygon_outline = FALSE;
  dataset_p->do_scale_marks = default_scale_marks;
  dataset_p->mark_type = default_mark_type;
  dataset_p->mark_size = default_mark_size;
  dataset_p->line_style = 0;
  dataset_p->line_width = 1;
  dataset_p->text_size = 12;

  dataset_p->set_name = NULL;
  if (prm_override_names)
    dataset_p->set_name = g_array_index (prm_override_names, char *, set_idx);
  if (!dataset_p->set_name)
    dataset_p->set_name = g_strdup (filename);
  dataset_p->path_name = g_strdup_printf ("Dataset %d", num_datasets);
  dataset_p->tree_path_string = NULL;
  dataset_p->is_visible = TRUE;

  return dataset_p;
}

static void
delete_dataset (dataset_t * dataset_p)
{
  g_array_free (dataset_p->points, TRUE);
  free (dataset_p);
}

static void
read_data_sets (int argc, char *argv[])
{
  gboolean is_new_set;
  FILE *IN;
  dataset_t *previous_dataset = NULL;
  dataset_t *dataset_p = first_dataset;
  int argp = 0;
  int linenum = 0;
  double min_x = HUGE_VAL;
  double max_x = 0;
  double min_y = HUGE_VAL;
  double max_y = 0;
  gboolean do_stdin = argc == 0;

  while (argp < argc || do_stdin)
    {
      char *filename;

      if (do_stdin)
	{
	  IN = stdin;
	  filename = "(stdin)";
	}
      else
	{
	  filename = argv[argp++];
	  IN = fopen (filename, "r");
	}

      if (!IN)
	{
	  fprintf (stderr, "Warning! Couldn't open %s!\n", filename);
	  continue;
	}

      is_new_set = TRUE;
      while (!feof (IN))
	{
	  char S_[256];
	  char dummy[256];
	  gint type;
	  point_t p;

	  linenum++;
	  fgets (S_, sizeof (S_), IN);
	  if (is_new_set)
	    {
	      dataset_p = new_dataset (num_datasets, filename);

	      dataset_p->color = set_colors[num_datasets % nset_colors];
	      dataset_p->file_name = g_strdup (filename);

	      if (!first_dataset)
		first_dataset = dataset_p;
	      if (previous_dataset)
		previous_dataset->next_dataset = dataset_p;
	      previous_dataset = dataset_p;

	      is_new_set = FALSE;
	      num_datasets++;
	    }


	  if (strlen (S_) == 1)
	    {
	      if (dataset_p && ((GArray *) dataset_p->points)->len > 0)
		is_new_set++;
	      continue;
	    }


#if 0
	  sscanf (line, "%lf %lf\n", &point.x, &point.y);
	  if (dataset_p->num_points >= dataset_p->num_alloc_points)
	    grow_dataset (dataset_p);

	  dataset_p->points[dataset_p->num_points] = point;
	  dataset_p->num_points++;
#endif

	  /* Parse the line */
	  type = gxgraph_parse_string (S_, filename, linenum);
	  switch (type)
	    {
	    case STRING_COMMENT:
	      break;
	    case STRING_DRAW:
	    case STRING_MOVE:
	      if (type == STRING_DRAW)
		{
		  sscanf (S_, "%lf %lf", &p.data.point.x, &p.data.point.y);
		  p.op = OP_DRAW;
		}
	      else
		{
		  sscanf (S_, "%s %lf %lf", dummy, &p.data.point.x,
			  &p.data.point.y);
		  p.op = OP_MOVE;
		}

	      /* Find marks bounding box */
	      if (p.data.point.x < min_x)
		min_x = p.data.point.x;
	      else if (p.data.point.x > max_x)
		max_x = p.data.point.x;
	      if (p.data.point.y < min_y)
		min_y = p.data.point.y;
	      else if (p.data.point.y > max_y)
		max_y = p.data.point.y;

	      g_array_append_val (dataset_p->points, p);
	      break;
	    case STRING_TEXT:
	      {
		text_mark_t *tm = (text_mark_t *) g_new (text_mark_t, 1);
		sscanf (S_, "%s %lf %lf", dummy, &tm->x, &tm->y);
		tm->string = string_strdup_rest (S_, 3);
		p.op = OP_TEXT;
		p.data.point.x = tm->x;
		p.data.point.y = tm->y;
		p.data.text_object = tm;
		g_array_append_val (dataset_p->points, p);
	      }
	      break;
	    case STRING_CHANGE_LINE_WIDTH:
	      dataset_p->line_width = string_to_atof (S_, 1);
	      break;
#if 0
	      /* Currently no support for images */
	    case STRING_IMAGE_REFERENCE:
	      {
		char *image_filename = string_strdup_word (S_, 1);

		/* Todo: Make image relative to the marks list */
		add_filename_to_image_list (image_filename,
					    image_file_name_list);
	      }
	      free (image_filename);
	      break;
	    case STRING_MARKS_REFERENCE:
	      {
		char *marks_filename = string_strdup_word (S_, 1);

		/* Todo: Make image relative to the marks list */
		g_ptr_array_add (mark_file_name_list, marks_filename);

		break;
	      }
	    case STRING_LOW_CONTRAST:
	      {
		giv_current_transfer_function = TRANS_FUNC_LOW_CONTRAST;
		break;
	      }
#endif
	    case STRING_CHANGE_NO_LINE:
	      dataset_p->do_draw_lines = FALSE;
	      break;
	    case STRING_CHANGE_POLYGON:
	      dataset_p->do_draw_polygon = TRUE;
	      break;
	    case STRING_CHANGE_LINE:
	      dataset_p->do_draw_lines = TRUE;
	      break;
	    case STRING_CHANGE_NO_MARK:
	      dataset_p->do_draw_marks = FALSE;
	      break;
	    case STRING_CHANGE_MARK_SIZE:
	      dataset_p->mark_size = string_to_atof (S_, 1);
	      break;
	    case STRING_CHANGE_TEXT_SIZE:
	      dataset_p->text_size = string_to_atof (S_, 1);
	      break;
	    case STRING_CHANGE_COLOR:
	      {
		char *color_name = string_strdup_word (S_, 1);
		GdkColor color;
		if (gdk_color_parse (color_name, &color))
		  dataset_p->color = color;
		g_free (color_name);
		break;
	      }
	    case STRING_CHANGE_OUTLINE_COLOR:
	      {
		char *color_name = string_strdup_word (S_, 1);
		GdkColor color;
		if (gdk_color_parse (color_name, &color))
		  dataset_p->outline_color = color;
		dataset_p->do_draw_polygon_outline = TRUE;
		g_free (color_name);
		break;
	      }
	    case STRING_CHANGE_MARKS:
	      {
		char *mark_name = string_strdup_word (S_, 1);
		dataset_p->do_draw_marks = TRUE;

		dataset_p->mark_type =
		  gxgraph_parse_mark_type (mark_name, filename, linenum);

		g_free (mark_name);
		break;
	      }
	    case STRING_CHANGE_SCALE_MARKS:
	      if (string_count_words (S_) == 1)
		dataset_p->do_scale_marks = 1;
	      else
		dataset_p->do_scale_marks = string_to_atoi (S_, 1);
	      break;
	    case STRING_PATH_NAME:
	      if (dataset_p->path_name)
		g_free (dataset_p->path_name);
	      dataset_p->path_name = string_strdup_rest (S_, 1);
	      break;
	    case STRING_SET_NAME:
	      if (dataset_p->set_name)
		g_free (dataset_p->set_name);

	      /* This is uggly. It is doing part of the parsing here...
		 My excuse is that the xgraph syntax is really broken.
	       */
	      if (S_[0] == '"')
		dataset_p->set_name = g_strdup (&S_[1]);
	      else
		dataset_p->set_name = string_strdup_rest(S_, 1);
	      break;
	    case STRING_SET_TITLE:
	      if (prm_title_text)
		g_free(prm_title_text);
	      prm_title_text = string_strdup_rest(S_, 1);
	      break;
	    case STRING_SET_LARGE_PIXELS:
	      default_draw_marks = TRUE;
	      break;
	    case STRING_SET_XUNIT_TEXT:
	      if (prm_x_unit_text)
		g_free(prm_x_unit_text);
	      prm_x_unit_text = string_strdup_rest(S_, 1);
	      break;
	    case STRING_SET_YUNIT_TEXT:
	      if (prm_y_unit_text)
		g_free(prm_y_unit_text);
	      prm_y_unit_text = string_strdup_rest(S_, 1);
	      break;
	    }

	}

      /* Get rid of empty data sets */
      if (dataset_p && dataset_p->points->len == 0)
	{
	  dataset_t *ds_p;
	  /* Search and get rid of the last dataset */
	  for (ds_p = first_dataset; ds_p; ds_p = ds_p->next_dataset)
	    {
	      if (ds_p->next_dataset == dataset_p)
		{
		  ds_p->next_dataset = NULL;
		  break;
		}
	    }
	  num_datasets--;

	  delete_dataset (dataset_p);
	}

      fclose (IN);

      if (do_stdin)
          break;
    }
}

static void
put_datasets_in_window (dataset_t * datasets,
			window_t * window, world_t * world)
{
  dataset_t *ds_p;

  window->first_dataset = datasets;

  if (world)
    {
      window->world.x0 = world->x0;
      window->world.x1 = world->x1;
      window->world.y0 = world->y0;
      window->world.y1 = world->y1;
    }
  else
    {
      double min_x, max_x, min_y, max_y;
      double world_pad_x, world_pad_y;

      min_x = min_y = HUGE;
      max_x = max_y = -HUGE;

      for (ds_p = datasets; ds_p; ds_p = ds_p->next_dataset)
	{
	  int p_idx;
	  if (!ds_p->points)
	    continue;
	  for (p_idx = 0; p_idx < ds_p->points->len; p_idx++)
	    {
	      point_t p = g_array_index (ds_p->points, point_t, p_idx);

	      if (p.data.point.y < min_y)
		min_y = p.data.point.y;
	      if (p.data.point.y > max_y)
		max_y = p.data.point.y;

	      if (p.data.point.x < min_x)
		min_x = p.data.point.x;
	      if (p.data.point.x > max_x)
		max_x = p.data.point.x;
	    }
	}

      /* Check if external paramaters are valid, then use these.
	 If both x and y are overridden then it is a waste of
	 time to search for the min and max above, but I am ignoring
	 that for the moment.
       */
      if (prm_x_hi_limit > prm_x_low_limit)
	{
	  min_x = prm_x_low_limit;
	  max_x = prm_x_hi_limit;
	}
      if (prm_y_hi_limit > prm_y_low_limit)
	{
	  min_y = prm_y_low_limit;
	  max_y = prm_y_hi_limit;
	}
      
      /* Add 10% padding */
      world_pad_x = (max_x - min_x) * 0.05;
      world_pad_y = (max_y - min_y) * 0.05;
      window->world.x0 = min_x - world_pad_x;
      window->world.x1 = max_x + world_pad_x;
      window->world.y0 = min_y - world_pad_y;
      window->world.y1 = max_y + world_pad_y;
    }

  window->world.col_0 = 20;
  window->world.scale_x =
    1.0 * (window->width - 40) / (window->world.x1 - window->world.x0);
  window->world.scale_y =
    1.0 * -(window->height - 40) / (window->world.y1 - window->world.y0);

  window->world.col_0 = 20 - window->world.x0 * window->world.scale_x;
  gxgraph_draw_window (window, NULL);
}

void
die (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  vfprintf (stderr, fmt, ap);
  exit (-1);
}

void
gxgraph_add_window_with_world (window_t * previous_window, world_t * world)
{
  window_t *window = new_window (previous_window);
  put_datasets_in_window (first_dataset, window, world);
}

window_t *
new_window (window_t * previous_window)
{
  window_t *window;

  window = (window_t *) g_malloc (sizeof (window_t));
  window->first_dataset = NULL;
  window->width = prm_requested_width;
  window->height = prm_requested_height;
  window->next_window = 0;
  if (previous_window)
    previous_window->next_window = window;
  window->previous_window = previous_window;
  window->gtk_painter = gtk_painter_new (window);

  return window;
}

void
gxgraph_draw_window (window_t * window, painter_t * painter)
{
  /* Use the gtk painter if no other painter has been provided */
  if (painter == NULL)
    {
      painter = window->gtk_painter;

      /* This is somewhat of a hack. The transform should only
       * be calculated for gtk windows...
      */
      compute_transform (window, painter);
    }

  if (window->first_dataset == NULL)
    return;

  gxgraph_draw_title (window, painter);

  gxgraph_draw_legend (window, painter);

  gxgraph_draw_grid_and_axis (window, painter);
  gxgraph_draw_data (window, painter);
}

/*
 * This routine figures out how to draw the axis labels and grid lines.
 * Both linear and logarithmic axes are supported.  Axis labels are
 * drawn in engineering notation.  The power of the axes are labeled
 * in the normal axis labeling spots.  The routine also figures
 * out the necessary transformation information for the display
 * of the points (it touches XOrgX, XOrgY, UsrOrgX, UsrOrgY, and
 * UnitsPerPixel).
 */
int
compute_transform (window_t * window, painter_t * painter)
{
  double bbCenX, bbCenY, bbHalfWidth, bbHalfHeight;
  int maxName, leftWidth;
  dataset_t *ds_p;

  /*
   * First,  we figure out the origin in the X window.  Above
   * the space we have the title and the Y axis unit label.
   * To the left of the space we have the Y axis grid labels.
   */

  window->org_x = painter->bdr_pad + (7 * painter->axis_width)
    + painter->bdr_pad;
  window->org_y = painter->bdr_pad + painter->title_height
    + painter->bdr_pad + painter->axis_height
    + painter->axis_height / 2 + painter->bdr_pad;

  /*
   * Now we find the lower right corner.  Below the space we
   * have the X axis grid labels.  To the right of the space we
   * have the X axis unit label and the legend.  We assume the 
   * worst case size for the unit label.
   */
  maxName = 0;
  for (ds_p = window->first_dataset; ds_p; ds_p = ds_p->next_dataset)
    {
      int tempSize;

      if (!ds_p->set_name)
	continue;

      tempSize = strlen (ds_p->set_name);
      if (tempSize > maxName)
	maxName = tempSize;
    }

  /* Worst case size of the X axis label: */
  leftWidth = (strlen (prm_x_unit_text)) * painter->axis_width;
  if ((maxName * painter->axis_width) + painter->bdr_pad > leftWidth)
    leftWidth = maxName * painter->axis_width + painter->bdr_pad;

  window->opp_x = painter->area_w - painter->bdr_pad - leftWidth;
  window->opp_y = painter->area_h - painter->bdr_pad
    - painter->axis_height - painter->bdr_pad;

  if ((window->org_x >= window->opp_x) || (window->org_y >= window->opp_y))
    {
      fprintf (stderr,
	       "Drawing area is too small. org opp = (%f %f) (%f %f)\n",
	       window->org_x, window->opp_x, window->org_y, window->opp_y);
      return -1;
    }

  /* 
   * We now have a bounding box for the drawing region.
   * Figure out the units per pixel using the data set bounding box.
   */
  window->world.scale_x =
    (window->world.x1 -
     window->world.x0) / ((double) (window->opp_x - window->org_x));
  window->world.scale_y =
    (window->world.y1 -
     window->world.y0) / ((double) (window->opp_y - window->org_y));

  /*
   * Find origin in user coordinate space.  We keep the center of
   * the original bounding box in the same place.
   */
  bbCenX = (window->world.x0 + window->world.x1) / 2.0;
  bbCenY = (window->world.y0 + window->world.y1) / 2.0;

  /* Calculate half width and multiply by 5% for esthetics */
  bbHalfWidth =
    ((double) (window->opp_x - window->org_x)) / 2.0 * window->world.scale_x;
  bbHalfHeight =
    ((double) (window->opp_y - window->org_y)) / 2.0 * window->world.scale_y;
  window->world_org_x = bbCenX - bbHalfWidth;
  window->world_org_y = bbCenY - bbHalfHeight;
  window->world_opp_x = bbCenX + bbHalfWidth;
  window->world_opp_y = bbCenY + bbHalfHeight;

  /*
   * Everything is defined so we can now use the SCREENX and SCREENY
   * transformations.
   */
  return 0;
}

void
gxgraph_draw_title (window_t * window, painter_t * painter)
{
  painter->draw_text (painter,
		      painter->area_w / 2,
		      painter->axis_pad / 4, prm_title_text, T_TOP, T_TITLE);

}

/*
 * This draws a legend of the data sets displayed.  Only those that
 * will fit are drawn.
 */
void
gxgraph_draw_legend (window_t * window, painter_t * painter)
{
  dataset_t *ds_p;
  int spot, lineLen;
  double leg_line_x1, leg_line_x2;
  double scale_x = 1.0;
  double scale_y = 1.0;

  /*    set_mark_flags(&markFlag, &pixelMarks, &bigPixel, &colorMark); */
  spot = window->org_y;
  lineLen = 0;

  /* First pass draws the text */
  for (ds_p = window->first_dataset; ds_p; ds_p = ds_p->next_dataset)
    {
      if (spot + painter->axis_height + 2 < window->opp_y)
	{
	  /* Meets the criteria */
	  int oneLen = strlen (ds_p->set_name);
	  if (oneLen > lineLen)
	    lineLen = oneLen;

	  painter->draw_text (painter,
			      window->opp_x + painter->bdr_pad,
			      spot + 1, ds_p->set_name, T_UPPERLEFT, T_AXIS);

	  spot += 2 + painter->axis_height + painter->bdr_pad;
	}
    }
  lineLen = lineLen * painter->axis_width;


  leg_line_x1 = window->opp_x + painter->bdr_pad;
  leg_line_x2 = leg_line_x1 + lineLen;
  spot = window->org_y;

  /* second pass draws the lines */
  for (ds_p = window->first_dataset; ds_p; ds_p = ds_p->next_dataset)
    {
      if (spot + painter->axis_height + 2 < window->opp_y)
	{
	  double leg_line_y = spot - painter->legend_pad;
	  GArray *mark_array = g_array_sized_new (FALSE,
						  FALSE,
						  sizeof (mark_t),
						  1);

	  painter->set_attributes (painter,
				   ds_p->color,
				   ds_p->line_width,
				   GDK_LINE_SOLID,
				   ds_p->mark_type,
				   ds_p->mark_size * scale_x,
				   ds_p->mark_size * scale_y);
	  painter->draw_line (painter,
			      leg_line_x1, leg_line_y,
			      leg_line_x2, leg_line_y);

	  if (ds_p->do_draw_marks == TRUE
	      || (ds_p->do_draw_marks == DEFAULT && default_draw_marks))
	    {
	      mark_t mark;
	      mark.x = leg_line_x1;
	      mark.y = leg_line_y;

	      g_array_append_val (mark_array, mark);

	      painter->draw_marks (painter, mark_array);

	    }
	  g_array_free (mark_array, TRUE);

	  spot += 2 + painter->axis_height + painter->bdr_pad;
	}
    }
}

void
gxgraph_draw_grid_and_axis (window_t * window, painter_t * painter)
{
  int expX, expY;		/* Engineering powers */
  int startX;
  int Yspot, Xspot;
  double Xincr, Yincr, Xstart, Ystart, Yindex, Xindex, larger;
  char power[10], value[10], final[256];
  world_t *world = &window->world;

  painter->group_start (painter, "grid");

  if (painter == NULL)
    painter = window->gtk_painter;

  painter->set_attributes_style (painter, L_AXIS);
  /*
   * Grid display powers are computed by taking the log of
   * the largest numbers and rounding down to the nearest
   * multiple of 3.
   */
  if (prm_do_logx)
    {
      expX = 0;
    }
  else
    {
      if (fabs (window->world.x0) > fabs (window->world.x1))
	{
	  larger = fabs (window->world.x0);
	}
      else
	{
	  larger = fabs (window->world.x1);
	}
      expX = ((int) floor (nlog10 (larger) / 3.0)) * 3;
    }
  if (prm_do_logy)
    {
      expY = 0;
    }
  else
    {
      if (fabs (window->world.y0) > fabs (window->world.y1))
	{
	  larger = fabs (window->world.y0);
	}
      else
	{
	  larger = fabs (window->world.y1);
	}
      expY = ((int) floor (nlog10 (larger) / 3.0)) * 3;
    }

  /*
   * With the powers computed,  we can draw the axis labels.
   */
  if (expY != 0)
    {
      strcpy (final, prm_y_unit_text);
      /*        strcat(final, " ?10"); */ /* Unicode... */
      strcat (final, " x 10");
      Xspot = ((strlen (prm_y_unit_text) + 7) * painter->axis_width);
      Yspot = painter->bdr_pad * 2 + painter->title_height +
	painter->axis_height / 2;
      painter->draw_text (painter, Xspot, Yspot, final, T_RIGHT, T_AXIS);
      sprintf (power, "%d", expY);
      painter->draw_text (painter, Xspot, Yspot, power, T_LOWERLEFT, T_AXIS);
    }
  else
    {
      Yspot = painter->bdr_pad * 2 + painter->title_height;
      painter->draw_text (painter,
			  painter->bdr_pad, Yspot, prm_y_unit_text,
			  T_UPPERLEFT, T_AXIS);
    }

  startX = painter->area_w - painter->bdr_pad;
  if (expX != 0)
    {
      sprintf (power, "%d", expX);
      startX -= (strlen (power) * painter->axis_width);
      painter->draw_text (painter,
			  startX, window->opp_y, power, T_LOWERLEFT, T_AXIS);
      strcpy (final, prm_x_unit_text);
      /*        strcat(final, " ?10");  */ /* unicode */
      strcat (final, " x 10");	// unicode
      painter->draw_text (painter,
			  startX, window->opp_y, final, T_RIGHT, T_AXIS);
    }
  else
    {
      painter->draw_text (painter,
			  startX, window->opp_y, prm_x_unit_text, T_RIGHT,
			  T_AXIS);
    }

  /* 
   * First,  the grid line labels
   */
  Yincr = (painter->axis_pad + painter->axis_height) * world->scale_y;
  Ystart = init_grid (window->world_org_y, Yincr, prm_do_logy);
  for (Yindex = Ystart; Yindex < window->world_opp_y; Yindex = step_grid ())
    {

      Yspot = SCREENY (window, Yindex);
      /* Write the axis label */
      write_value (value, Yindex, expY, 1, prm_do_logy);
      painter->draw_text (painter,
			  painter->bdr_pad + 7 * painter->axis_width,
			  Yspot, value, T_RIGHT, T_AXIS);
    }

  Xincr = (painter->axis_pad + (painter->axis_width * 7)) * world->scale_x;
  Xstart = init_grid (window->world_org_x, Xincr, prm_do_logx);
  for (Xindex = Xstart; Xindex < window->world_opp_x; Xindex = step_grid ())
    {
      Xspot = SCREENX (window, Xindex);
      /* Write the axis label */
      write_value (value, Xindex, expX, 0, prm_do_logx);
      painter->draw_text (painter,
			  Xspot,
			  painter->area_h - painter->bdr_pad,
			  value, T_BOTTOM, T_AXIS);
    }

  /*
   * Now,  the grid lines or tick marks
   */
  Yincr = (painter->axis_pad + painter->axis_height) * world->scale_y;
  Ystart = init_grid (window->world_org_y, Yincr, prm_do_logy);
  for (Yindex = Ystart; Yindex < window->world_opp_y; Yindex = step_grid ())
    {
      double sx1, sx2, sx3, sx4, sy;

      Yspot = SCREENY (window, Yindex);
      sy = Yspot;

      /* Draw the grid line or tick mark */
      if (prm_do_draw_ticks)
	{
	  sx1 = window->org_x;
	  sx2 = window->org_x + painter->tick_len;
	  sx3 = window->opp_x - painter->tick_len;
	  sx4 = window->opp_x;
	}
      else
	{
	  sx1 = window->org_x;
	  sx2 = window->opp_x;
	}

      if ((ABS (Yindex) < ZERO_THRESH * (world->y1 - world->y0))
	  && !prm_do_logy)
	{
	  painter->set_attributes_style (painter, L_ZERO);
	}
      else
	{
	  painter->set_attributes_style (painter, L_GRID);
	}
      painter->draw_line (painter, sx1, sy, sx2, sy);

      if (prm_do_draw_ticks)
	{
	  painter->draw_line (painter, sx3, sy, sx4, sy);
	}

    }

  Xincr = (painter->axis_pad + (painter->axis_width * 7)) * world->scale_x;
  Xstart = init_grid (window->world_org_x, Xincr, prm_do_logx);
  for (Xindex = Xstart; Xindex < window->world_opp_x; Xindex = step_grid ())
    {
      double sx, sy1, sy2, sy3, sy4;

      Xspot = SCREENX (window, Xindex);
      sx = Xspot;
      /* Draw the grid line or tick marks */
      if (prm_do_draw_ticks)
	{
	  sy1 = window->org_y;
	  sy2 = window->org_y + painter->tick_len;
	  sy3 = window->opp_y - painter->tick_len;
	  sy4 = window->opp_y;
	}
      else
	{
	  sy1 = window->org_y;
	  sy2 = window->opp_y;
	}
      if ((ABS (Xindex) < ZERO_THRESH * (world->x1 - world->x0))
	  && !prm_do_logx)
	{
	  painter->set_attributes_style (painter, L_ZERO);
	}
      else
	{
	  painter->set_attributes_style (painter, L_GRID);
	}
      painter->draw_line (painter, sx, sy1, sx, sy2);

      if (prm_do_draw_ticks)
	{
	  painter->draw_line (painter, sx, sy3, sx, sy4);
	}
    }

  if (prm_do_draw_bounding_box)
    {
      // Draw bounding box
      painter->draw_line (painter,
			  window->org_x, window->org_y,
			  window->opp_x, window->org_y);
      painter->draw_line (painter,
			  window->opp_x, window->org_y,
			  window->opp_x, window->opp_y);
      painter->draw_line (painter,
			  window->opp_x, window->opp_y,
			  window->org_x, window->opp_y);
      painter->draw_line (painter,
			  window->org_x, window->opp_y,
			  window->org_x, window->org_y);
    }

  painter->group_end (painter, "grid");
}

#define LEFT_CODE	0x01
#define RIGHT_CODE	0x02
#define BOTTOM_CODE	0x04
#define TOP_CODE	0x08

/* Clipping algorithm from Neumann and Sproull by Cohen and Sutherland */
#define C_CODE(xval, yval, rtn)					\
  rtn = 0;							\
  if ((xval) < window->world_org_x) rtn = LEFT_CODE;		\
  else if ((xval) > window->world_opp_x) rtn = RIGHT_CODE;	\
  if ((yval) < window->world_org_y) rtn |= BOTTOM_CODE;		\
  else if ((yval) > window->world_opp_y) rtn |= TOP_CODE


void
gxgraph_draw_data (window_t * window, painter_t * painter)
{
  dataset_t *ds_p;
  double sx1, sy1, sx2, sy2, tx, ty;
  int code1, code2, cd, mark_inside;
  double scale_x = 1.0;
  double scale_y = 1.0;

  // TBD: If do_scale_marks is on, then the scale of the marks
  // should be adjusted.
  for (ds_p = window->first_dataset; ds_p; ds_p = ds_p->next_dataset)
    {
      int i;
      gboolean do_draw_marks;
      gboolean do_draw_lines;
      point_t prev_point = g_array_index (ds_p->points, point_t, 0);
      GArray *seg_array = g_array_sized_new (FALSE,
					     FALSE,
					     sizeof (seg_t),
					     ds_p->points->len);
      GArray *mark_array = g_array_sized_new (FALSE,
					      FALSE,
					      sizeof (mark_t),
					      ds_p->points->len);

      do_draw_lines = ds_p->do_draw_lines == TRUE
	|| (ds_p->do_draw_lines == DEFAULT && default_draw_lines);
      do_draw_marks = ds_p->do_draw_marks == TRUE
	|| (ds_p->do_draw_marks == DEFAULT && default_draw_marks);
      
      painter->set_attributes (painter,
			       ds_p->color,
			       ds_p->line_width,
			       GDK_LINE_SOLID,
			       ds_p->mark_type,
			       ds_p->mark_size * scale_x,
			       ds_p->mark_size * scale_y);
      for (i = 0; i < ds_p->points->len; i++)
	{
	  point_t p = g_array_index (ds_p->points, point_t, i);
	  double x = p.data.point.x;
	  double y = p.data.point.y;

	  if (ds_p->do_draw_lines && i > 0 && p.op == OP_DRAW)
	    {
	      sx1 = prev_point.data.point.x;
	      sy1 = prev_point.data.point.y;
	      sx2 = x;
	      sy2 = y;

	      C_CODE (sx1, sy1, code1);
	      C_CODE (sx2, sy2, code2);
	      mark_inside = (code1 == 0);

	      while (code1 || code2)
		{
		  if (code1 & code2)
		    break;
		  cd = (code1 ? code1 : code2);
		  if (cd & LEFT_CODE)
		    {		/* Crosses left edge */
		      ty =
			sy1 + (sy2 - sy1) * (window->world_org_x -
					     sx1) / (sx2 - sx1);
		      tx = window->world_org_x;
		    }
		  else if (cd & RIGHT_CODE)
		    {		/* Crosses right edge */
		      ty =
			sy1 + (sy2 - sy1) * (window->world_opp_x -
					     sx1) / (sx2 - sx1);
		      tx = window->world_opp_x;
		    }
		  else if (cd & BOTTOM_CODE)
		    {		/* Crosses bottom edge */
		      tx =
			sx1 + (sx2 - sx1) * (window->world_org_y -
					     sy1) / (sy2 - sy1);
		      ty = window->world_org_y;
		    }
		  else if (cd & TOP_CODE)
		    {		/* Crosses top edge */
		      tx =
			sx1 + (sx2 - sx1) * (window->world_opp_y -
					     sy1) / (sy2 - sy1);
		      ty = window->world_opp_y;
		    }
		  if (cd == code1)
		    {
		      sx1 = tx;
		      sy1 = ty;
		      C_CODE (sx1, sy1, code1);
		    }
		  else
		    {
		      sx2 = tx;
		      sy2 = ty;
		      C_CODE (sx2, sy2, code2);
		    }
		}

	      if (!(code1 && code2))
		{
		  seg_t seg;
		  seg.x1 = SCREENX (window, sx1);
		  seg.y1 = SCREENY (window, sy1);
		  seg.x2 = SCREENX (window, sx2);
		  seg.y2 = SCREENY (window, sy2);

		  g_array_append_val (seg_array, seg);
		}
	    }

	  /* Marks */
	  if (x >= window->world_org_x
	      && x <= window->world_opp_x
	      && y >= window->world_org_y && y <= window->world_opp_y)
	    {
	      mark_t mark;
	      mark.x = SCREENX (window, x);
	      mark.y = SCREENY (window, y);
	      g_array_append_val (mark_array, mark);
	    }
	  prev_point = p;
	}

      if (do_draw_lines && do_draw_marks)
	painter->group_start (painter, "lines_marks");

      if (do_draw_lines)
	{
	  painter->group_start (painter, "lines");
	  painter->draw_segments (painter, seg_array);
	  painter->group_end (painter, "lines");
	}
      if (do_draw_marks)
	{
	  painter->group_start (painter, "marks");
	  painter->draw_marks (painter, mark_array);
	  painter->group_end (painter, "marks");
	}

      g_array_free (seg_array, TRUE);
      g_array_free (mark_array, TRUE);

      if (do_draw_lines && do_draw_marks)
	painter->group_end (painter, "lines_marks");

    }
}

void
window_delete (window_t * window)
{
  painter_t *painter = window->gtk_painter;

  if (window->previous_window == NULL)
    {
      first_window = window->next_window;
    }
  else
    {
      window->previous_window->next_window = window->next_window;
    }

  if (window->next_window != NULL)
    {
      window->next_window->previous_window = window->previous_window;
    }

  window->gtk_painter = NULL;

  gtk_painter_delete (painter);
  free (window);
  if (first_window == NULL)
    {
      gtk_main_quit ();
    }
}

/*======================================================================
//  Grid support. This should be cleaned up.
//----------------------------------------------------------------------
*/
static double gridBase, gridStep, gridJuke[101];
static int gridNJuke, gridCurJuke;

#define ADD_GRID(val)	(gridJuke[gridNJuke++] = log10(val))

double
init_grid (double low,		/* desired low value          */
	   double step,		/* desired step (user coords) */
	   int logFlag)		/* is axis logarithmic?       */
{
  double ratio, x;

  gridNJuke = gridCurJuke = 0;
  gridJuke[gridNJuke++] = 0.0;

  if (logFlag)
    {
      ratio = pow (10.0, step);
      gridBase = floor (low);
      gridStep = ceil (step);
      if (ratio <= 3.0)
	{
	  if (ratio > 2.0)
	    {
	      ADD_GRID (3.0);
	    }
	  else if (ratio > 4.0 / 3.0)
	    {
	      ADD_GRID (2.0);
	      ADD_GRID (5.0);
	    }
	  else if (ratio > 1.25)
	    {
	      ADD_GRID (1.5);
	      ADD_GRID (2.0);
	      ADD_GRID (3.0);
	      ADD_GRID (5.0);
	      ADD_GRID (7.0);
	    }
	  else
	    {
	      for (x = 1.0; x < 10.0 && (x + .5) / (x + .4) >= ratio; x += .5)
		{
		  ADD_GRID (x + .1);
		  ADD_GRID (x + .2);
		  ADD_GRID (x + .3);
		  ADD_GRID (x + .4);
		  ADD_GRID (x + .5);
		}
	      if (floor (x) != x)
		ADD_GRID (x += .5);
	      for (; x < 10.0 && (x + 1.0) / (x + .5) >= ratio; x += 1.0)
		{
		  ADD_GRID (x + .5);
		  ADD_GRID (x + 1.0);
		}
	      for (; x < 10.0 && (x + 1.0) / x >= ratio; x += 1.0)
		{
		  ADD_GRID (x + 1.0);
		}
	      if (x == 7.0)
		{
		  gridNJuke--;
		  x = 6.0;
		}
	      if (x < 7.0)
		{
		  ADD_GRID (x + 2.0);
		}
	      if (x == 10.0)
		gridNJuke--;
	    }
	  x = low - gridBase;
	  for (gridCurJuke = -1; x >= gridJuke[gridCurJuke + 1];
	       gridCurJuke++)
	    {
	    }
	}
    }
  else
    {
      gridStep = round_up (step);
      gridBase = floor (low / gridStep) * gridStep;
    }
  return (step_grid ());
}

double
step_grid ()
{
  if (++gridCurJuke >= gridNJuke)
    {
      gridCurJuke = 0;
      gridBase += gridStep;
    }
  return (gridBase + gridJuke[gridCurJuke]);
}

/*
 * This routine rounds up the given positive number such that
 * it is some power of ten times either 1, 2, or 5.  It is
 * used to find increments for grid lines.
 */
double
round_up (double val)
{
  int exponent, idx;

  exponent = (int) floor (nlog10 (val));
  if (exponent < 0)
    {
      for (idx = exponent; idx < 0; idx++)
	{
	  val *= 10.0;
	}
    }
  else
    {
      for (idx = 0; idx < exponent; idx++)
	{
	  val /= 10.0;
	}
    }
  if (val > 5.0)
    val = 10.0;
  else if (val > 2.0)
    val = 5.0;
  else if (val > 1.0)
    val = 2.0;
  else
    val = 1.0;
  if (exponent < 0)
    {
      for (idx = exponent; idx < 0; idx++)
	{
	  val /= 10.0;
	}
    }
  else
    {
      for (idx = 0; idx < exponent; idx++)
	{
	  val *= 10.0;
	}
    }
  return val;
}

void
write_value (char *str,		/* String to write into */
	     double val,	/* Value to print       */
	     int expv,		/* Exponent             */
	     int axis_num,	/* xaxis = 0, yaxis = 1 */
	     int logFlag)	/* is this a log axis?  */
{
  int idx;

  if (logFlag)
    {
      if (val == floor (val))
	{
	  (void) sprintf (str, "%.0e", pow (10.0, val));
	}
      else
	{
	  (void) sprintf (str, "%.2g", pow (10.0, val - floor (val)));
	}
    }
  else
    {
      if (expv < 0)
	{
	  for (idx = expv; idx < 0; idx++)
	    {
	      val *= 10.0;
	    }
	}
      else
	{
	  for (idx = 0; idx < expv; idx++)
	    {
	      val /= 10.0;
	    }
	}
      if (axis_num == 0)
	sprintf (str, prm_xfmt, val);
      else
	sprintf (str, prm_yfmt, val);
    }
}
