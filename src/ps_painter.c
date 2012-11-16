/*======================================================================
//  ps_painter.c - Postscript backend
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#include <stdio.h>
#include <string.h>
#include "gxgraph.h"
#include "ps_painter.h"

typedef struct
{
  painter_t painter;

  gboolean is_pipe;
  FILE *PS;
  /* Caching of current values */
  double current_text_size;
  gint current_mark_type;
  gdouble current_mark_size_x;
  gdouble current_mark_size_y;
} ps_painter_t;

#define PADDING         2
#define SPACE           10
#define TICKLENGTH      5

static void
ps_painter_set_attributes (painter_t * painter,
			   GdkColor color,
			   double line_width,
			   int line_style,
			   int mark_type, gdouble scale_x, gdouble scale_y);
static void
ps_painter_draw_line (painter_t * painter,
		      double x1, double y1, double x2, double y2);
static void ps_painter_draw_segments (painter_t * painter, GArray * segments);
static void ps_painter_draw_marks (painter_t * painter, GArray * marks);
static void
ps_painter_draw_text (struct painter_t_struct *painter,
		      double x_pos, double y_pos,
		      const char *text, int just, int style);
static void ps_painter_set_attributes_style (painter_t * painter, int style);

static void
ps_painter_group_start (struct painter_t_struct *painter,
			const char *group_name);

static void
ps_painter_group_end (struct painter_t_struct *painter,
		      const char *group_name);

painter_t *
ps_painter_new (window_t * window, const char *filename)
{
  ps_painter_t *this = g_new0 (ps_painter_t, 1);
  painter_t *parent = (painter_t *) this;
  double bbox[4];
  double graph_width = window->width;
  double graph_height = window->height;

  // This should be configurable
  double paper_height = 842;
  double paper_width = 595;

  // Don't need these two statements, do I?
  parent->area_w = window->width;
  parent->area_h = window->height;

  parent->set_attributes = ps_painter_set_attributes;
  parent->draw_segments = ps_painter_draw_segments;
  parent->draw_marks = ps_painter_draw_marks;
  parent->draw_line = ps_painter_draw_line;
  parent->draw_text = ps_painter_draw_text;
  parent->set_attributes_style = ps_painter_set_attributes_style;
  parent->group_start = ps_painter_group_start;
  parent->group_end = ps_painter_group_end;

  parent->bdr_pad = PADDING;
  parent->axis_pad = SPACE;
  parent->legend_pad = 0;
  parent->tick_len = TICKLENGTH;
  parent->axis_width = 5;
  parent->axis_height = 13;
  parent->title_width = 5;
  parent->title_height = 5;

  /* Open postscript file and write header to it */
  bbox[0] = (paper_width - graph_width) / 2;
  bbox[1] = (paper_height - graph_height) / 2;
  bbox[2] = (paper_width + graph_width) / 2;
  bbox[3] = (paper_height + graph_height) / 2;

  this->is_pipe = FALSE;
  if (filename[0] == '|')
    {
      this->PS = popen (&filename[1], "w");
      this->is_pipe = TRUE;
    }
  else
    this->PS = fopen (filename, "w");
  fprintf (this->PS,
	   "%%!PS-Adobe-2.0 EPSF-2.0\n"
	   "%%%%BoundingBox: %.0f %.0f %.0f %.0f\n"
	   "%% PostScript output by gxgraph.\n"
	   "%% A program by Dov Grobgeld <dov.grobgeld@gmail.com>\n"
	   "%f %f translate\n"
	   "/M {moveto} bind def\n"
	   "/S {stroke} bind def\n"
	   "/L {lineto} bind def\n"
	   "/F {fill} bind def\n"
	   "/xd {exch def} bind def\n"
	   "/R { [] 0 setdash 0 0 0 setrgbcolor } bind def\n"
	   "/LW {setlinewidth} bind def\n"
	   "/RGB { 255 div 3 1 roll\n"
	   "       255 div 3 1 roll\n"
	   "       255 div 3 1 roll setrgbcolor } bind def\n"
	   "/N {newpath}bind def\n"
	   "%% Define marks\n"
	   "/ms { /marksize_y exch def /marksize_x exch def } def\n"
	   "/mC { /y exch def /x exch def\n"
	   "       x marksize_x 2 div add y moveto\n"
	   "       x y marksize_x 2 div 0 360 arc stroke\n"
	   "    } bind def\n"
	   "/mFC { /y exch def /x exch def\n"
	   "       x marksize_x 2 div add y moveto\n"
	   "       x y marksize_x 2 div 0 360 arc fill stroke\n"
	   "    } bind def\n"
	   "/mS { /y exch def /x exch def\n"
	   "      x marksize_x 2 div sub  y marksize_y 2 div sub moveto\n"
	   "      marksize_x 0 rlineto\n"
	   "      0 marksize_y rlineto\n"
	   "      marksize_x neg 0 rlineto\n"
	   "      0 marksize_y neg rlineto\n"
	   "      stroke } bind def\n"
	   "/mFS { /y exch def /x exch def\n"
	   "      x marksize_x 2 div sub  y marksize_y 2 div sub moveto\n"
	   "      marksize_x 0 rlineto\n"
	   "      0 marksize_y rlineto\n"
	   "      marksize_x neg 0 rlineto\n"
	   "      0 marksize_y neg rlineto\n"
	   "      fill stroke } bind def\n"
	   "/mark_size_x 0 def /mark_size_y 0 def\n"
	   "/rshow { dup stringwidth neg exch neg exch rmoveto show } def\n"
	   "/cshow { dup stringwidth -2 div exch -2 div exch rmoveto show } def\n"
	   // These should be fixed!
	   "/bshow { cshow } def\n"
	   "/llshow { show } def\n",
	   bbox[0], bbox[1], bbox[2], bbox[3], bbox[0], bbox[1]);

  /* Marker definitions */
  fprintf (this->PS,
	   "%% Marker definitions\n"
	   "/m0 {/size xd /y xd /x xd\n"
	   "N x size sub y size sub moveto\n"
	   "size size add 0 rlineto 0 size size add rlineto\n"
	   "0 size size add sub 0 rlineto closepath F} def\n"
	   "/m1 {/size xd /y xd /x xd\n"
	   "N x size sub y size sub moveto\n"
	   "size size add 0 rlineto 0 size size add rlineto\n"
	   "0 size size add sub 0 rlineto closepath stroke} def\n"
	   "/m2 {/size xd /y xd /x xd\n"
	   "N x y moveto x y size 0 360 arc stroke} def\n"
	   "/m3 {/size xd /y xd /x xd\n"
	   "N x size sub y size sub moveto x size add y size add lineto\n"
	   "x size sub y size add moveto x size add y size sub lineto stroke} def\n"
	   "/m4 {/size xd /y xd /x xd\n"
	   "N x size sub y moveto x y size add lineto\n"
	   "x size add y lineto x y size sub lineto\n"
	   "closepath stroke} def\n"
	   "/m5 {/size xd /y xd /x xd\n"
	   "x y size m1\n"
	   "N x size sub y moveto size size add 0 rlineto stroke} def\n"
	   "/m6 {/size xd /y xd /x xd\n"
	   "N x y moveto x y size 0 360 arc F} def\n"
	   "/m7 {/size xd /y xd /x xd\n"
	   "N x y M x size sub y size sub L\n"
	   "x size add y size sub lineto closepath F\n"
	   "N x y M x size add y size add L\n"
	   "x size sub y size add lineto closepath F} def\n");

  /* Set initial font... */
  this->current_text_size = 11;
  fprintf (this->PS,
	   "/Helvetica findfont %.1f scalefont setfont\n",
	   this->current_text_size);

  parent->set_attributes = ps_painter_set_attributes;
  return (painter_t *) this;
}

#define PSY(y) (painter->area_h - y)

void
ps_painter_delete (painter_t * painter)
{
  ps_painter_t *ps_painter = (ps_painter_t *) painter;
  fprintf (ps_painter->PS, "showpage\n");
  if (ps_painter->is_pipe)
    pclose (ps_painter->PS);
  else
    fclose (ps_painter->PS);

  g_free (ps_painter);
}

static void
ps_painter_set_attributes (painter_t * painter,
			   GdkColor color,
			   double line_width,
			   int line_style,
			   gint mark_type,
			   gdouble mark_size_x, gdouble mark_size_y)
{
  ps_painter_t *ps_painter = (ps_painter_t *) painter;

  fprintf (ps_painter->PS,
	   "%f LW\n"
	   "%f %f %f RGB\n",
	   line_width,
	   1.0 / 256 * color.red,
	   1.0 / 256 * color.green, 1.0 / 256 * color.blue);

  // TBD: Set the mark properties
  ps_painter->current_mark_type = mark_type;
  ps_painter->current_mark_size_x = mark_size_x;
  ps_painter->current_mark_size_y = mark_size_y;
}

static void
ps_painter_draw_line (painter_t * painter,
		      double x1, double y1, double x2, double y2)
{
  ps_painter_t *ps_painter = (ps_painter_t *) painter;

  fprintf (ps_painter->PS, "%g %g M %g %g L S\n", x1, PSY (y1), x2, PSY (y2));
}

static void
ps_painter_draw_segments (painter_t * painter, GArray * segments)
{
  seg_t *segs = (seg_t *) segments->data;
  int seg_idx;

  for (seg_idx = 0; seg_idx < segments->len; seg_idx++)
    ps_painter_draw_line (painter,
			  segs[seg_idx].x1,
			  segs[seg_idx].y1,
			  segs[seg_idx].x2, segs[seg_idx].y2);
}

static void
ps_painter_draw_marks (painter_t * painter, GArray * marks_array)
{
  ps_painter_t *ps_painter = (ps_painter_t *) painter;
  mark_t *marks = (mark_t *) marks_array->data;
  FILE *PS = ps_painter->PS;	/* Shortcut */
  int m_idx;
  int mark_type = ps_painter->current_mark_type;

  /* Define postscript for the current mark based on the current
     size and type. */
  fprintf (PS,
	   "%f %f ms\n",
	   ps_painter->current_mark_size_x, ps_painter->current_mark_size_y);

  /* Define postscript for the current mark */
  if (mark_type == MARK_TYPE_CIRCLE)
    fprintf (PS, "/m /mC load def\n");
  else if (mark_type == MARK_TYPE_FCIRCLE)
    fprintf (PS, "/m /mFC load def\n");
  else if (mark_type == MARK_TYPE_SQUARE)
    fprintf (PS, "/m /mS load def\n");
  else if (mark_type == MARK_TYPE_FSQUARE)
    fprintf (PS, "/m /mFS load def\n");

  for (m_idx = 0; m_idx < marks_array->len; m_idx++)
    fprintf (PS, "%.2f %.2f m\n", marks[m_idx].x, PSY (marks[m_idx].y));

}

static void
ps_painter_draw_text (struct painter_t_struct *painter,
		      double x_pos, double y_pos,
		      const char *text, int just, int style)
{
  ps_painter_t *ps_painter = (ps_painter_t *) painter;
  char *show_string = "show";
  int char_idx, len;
  double text_size = ps_painter->current_text_size;

  // This should be more configurable...
  if (style == T_TITLE)
    text_size = 18;
  else if (style == T_AXIS)
    text_size = 11;

  if (text_size != ps_painter->current_text_size)
    {
      fprintf (ps_painter->PS,
	       "/Helvetica findfont %.1f scalefont setfont\n", text_size);
      ps_painter->current_text_size = text_size;
    }

  if (just == T_RIGHT)
    {
      /* The following is emperical for Helvetica fonts... */
      y_pos += 0.37 * text_size;
      show_string = "rshow";
    }
  else if (just == T_LOWERLEFT)
    {
      show_string = "show";
    }
  else if (just == T_UPPERLEFT)
    {
      show_string = "show";
      y_pos += 0.8 * text_size;
    }
  else if (just == T_BOTTOM)
    {
      show_string = "cshow";
    }
  else if (just == T_TOP)
    {
      show_string = "cshow";
      y_pos += 0.74 * text_size;
    }

  // Quote string and output it
  fprintf (ps_painter->PS, "%g %g M (", x_pos, PSY (y_pos));
  len = strlen (text);
  for (char_idx = 0; char_idx < len; char_idx++)
    {
      if (text[char_idx] == ')' || text[char_idx] == '(')
	fputc ('\\', ps_painter->PS);
      fputc (text[char_idx], ps_painter->PS);
    }
  fprintf (ps_painter->PS, ") %s\n", show_string);
}

static void
ps_painter_set_attributes_style (painter_t * painter, int style)
{
  char *color_name;
  GdkColor color;
  double line_width;
  int line_style;

  switch (style)
    {
    case L_AXIS:
      color_name = "black";
      line_width = 1;
      break;
    case L_GRID:
      color_name = "gray80";
      line_width = 0.5;
      break;
    case L_ZERO:
      color_name = "black";
      line_width = 2;
      break;
    }
  gdk_color_parse (color_name, &color);
  ps_painter_set_attributes (painter,
			     color, line_width, line_style, 0, 1.0, 1.0);
}

static void
ps_painter_group_start (struct painter_t_struct *painter,
			const char *group_name)
{
}

static void
ps_painter_group_end (struct painter_t_struct *painter,
		      const char *group_name)
{
}
