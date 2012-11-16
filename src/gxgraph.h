#ifndef GXGRAPH_H
#define GXGRAPH_H

#include <gtk/gtk.h>

/* Text justifications */
#define T_CENTER        0
#define T_LEFT          1
#define T_UPPERLEFT     2
#define T_TOP           3
#define T_UPPERRIGHT    4
#define T_RIGHT         5
#define T_LOWERRIGHT    6
#define T_BOTTOM        7
#define T_LOWERLEFT     8

/* Text styles */
#define T_AXIS          0
#define T_TITLE         1

/* Line Styles */
#define L_AXIS          0
#define L_ZERO          1
#define L_VAR           2
#define L_GRID          3

/* Drawing operations */
enum
{
  OP_MOVE = 0,
  OP_DRAW = 1,
  OP_TEXT = 2
};

/* Mark types */
enum
{
  MARK_TYPE_FCIRCLE = 1,
  MARK_TYPE_FSQUARE = 2,
  MARK_TYPE_CIRCLE = 3,
  MARK_TYPE_SQUARE = 4,
  MARK_TYPE_PIXEL = 5,
};

typedef struct
{
  char *string;
  double x, y;
  double size;
} text_mark_t;

typedef struct
{
  gint op;
  union
  {
    struct
    {
      gdouble x, y;
    } point;
    text_mark_t *text_object;
  } data;
} point_t;

typedef struct
{
  double x, y;
} mark_t;

/// segments used in drawing
typedef struct
{
  double x1;
  double y1;
  double x2;
  double y2;
} seg_t;

typedef struct dataset_t
{
  GdkColor color;
  GdkColor outline_color;
  gdouble line_width;
  gint line_style;
  gint mark_type;
  gint text_size;
  gdouble mark_size;
  gboolean do_scale_marks;
  gboolean do_draw_marks;
  gboolean do_draw_lines;
  gboolean do_draw_polygon;
  gboolean do_draw_polygon_outline;
  GArray *points;
  gchar *path_name;
  gchar *file_name;
  gchar *tree_path_string;
  gboolean is_visible;
  char *set_name;
  struct dataset_t *next_dataset;
} dataset_t;

typedef struct world_t
{
  double x0, y0, x1, y1;	/* Bounding box of data in world */

  /* Transformation between world and screen */
  int col_0, row_0;
  double scale_x, scale_y;

} world_t;

typedef struct painter_t_struct
{
  void *user_data;
  int area_w, area_h;		/* Width and height in pixels            */
  int bdr_pad;			/* Padding from border                   */
  int axis_pad;			/* Extra space around axis labels        */
  int tick_len;			/* Length of tick mark on axis           */
  int legend_pad;		/* Top of legend text to legend line     */
  int axis_width;		/* Width of big character of axis font   */
  int axis_height;		/* Height of big character of axis font  */
  int title_width;		/* Width of big character of title font  */
  int title_height;		/* Height of big character of title font */

  void (*draw_segments) (struct painter_t_struct * painter,
			 GArray * segments);
  void (*draw_marks) (struct painter_t_struct * painter, GArray * points);
  void (*draw_text) (struct painter_t_struct * painter,
		     double x_pos, double y_pos,
		     const char *text, int just, int style);
  void (*draw_dot) (void *dot);
  void (*draw_line) (struct painter_t_struct * painter,
		     double x1, double y1, double x2, double y2);
  void (*set_attributes) (struct painter_t_struct * painter,
			  GdkColor color,
			  double line_width,
			  gint line_style,
			  gint mark_type,
			  gdouble mark_size_x, gdouble mark_size_y);
  void (*set_attributes_style) (struct painter_t_struct * painter, int style);
  void (*group_start) (struct painter_t_struct * painter,
		       const char *group_name);
  void (*group_end) (struct painter_t_struct * painter,
		     const char *group_name);
} painter_t;

struct window_t_struct;

typedef struct window_t_struct
{
  double width, height;
  double org_x, org_y, opp_x, opp_y;
  double world_org_x, world_org_y, world_opp_x, world_opp_y;
  world_t world;
  struct window_t_struct *next_window;
  struct window_t_struct *previous_window;
  dataset_t *first_dataset;
  painter_t *gtk_painter;
} window_t;

typedef struct properties_t
{
  int dum;
} properties_t;

void gxgraph_draw_window (window_t * window, painter_t * painter);
void window_delete (window_t * window);
void gxgraph_add_window_with_world (window_t * previous_window,
				    world_t * world);

#endif
