/*======================================================================
//  svgpainter.c - svg backend
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#include <stdio.h>
#include <string.h>
#include "gxgraph.h"
#include "svg_painter.h"

typedef struct
{
  painter_t painter;

  FILE *SVG;
  /* Caching of current values */
  double current_text_size;
  double current_line_width;
  gint current_mark_type;
  gdouble current_mark_size_x;
  gdouble current_mark_size_y;
  GdkColor current_color;
} svg_painter_t;

static void
svg_painter_set_attributes (painter_t * painter,
                            GdkColor color,
                            double line_width,
                            int line_style,
                            int mark_type, gdouble scale_x, gdouble scale_y);
static void
svg_painter_draw_line (painter_t * painter,
                       double x1, double y1, double x2, double y2);
static void
svg_painter_draw_segments (painter_t * painter, GArray * segments);
static void svg_painter_draw_marks (painter_t * painter, GArray * marks);
static void
svg_painter_draw_text (struct painter_t_struct *painter,
                       double x_pos, double y_pos,
                       const char *text, int just, int style);
static void svg_painter_set_attributes_style (painter_t * painter, int style);

static void
svg_painter_group_start (struct painter_t_struct *painter,
                         const char *group_name);

static void
svg_painter_group_end (struct painter_t_struct *painter,
                       const char *group_name);



#define VDPI                    72.0
#define LDIM                    11.0
#define SDIM                    8.5
#define MICRONS_PER_INCH        2.54E+04
#define POINTS_PER_INCH         72.0
#define INCHES_PER_POINT        1.0/72.0
#define DEV(val)        ((double)val * POINTS_PER_INCH / VDPI)

#define PADDING         2
#define SPACE           10
#define TICKLENGTH      5

painter_t *
svg_painter_new (window_t * window, const char *filename)
{
  svg_painter_t *this = g_new0 (svg_painter_t, 1);

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

  parent->set_attributes = svg_painter_set_attributes;
  parent->draw_segments = svg_painter_draw_segments;
  parent->draw_marks = svg_painter_draw_marks;
  parent->draw_line = svg_painter_draw_line;
  parent->draw_text = svg_painter_draw_text;
  parent->set_attributes_style = svg_painter_set_attributes_style;
  parent->group_start = svg_painter_group_start;
  parent->group_end = svg_painter_group_end;

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

  this->SVG = fopen (filename, "w");

  fprintf (this->SVG,
           "<?xml version=\"1.0\" standalone=\"no\"?>\n"
           "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20000303 Stylable//EN\"\n"
           "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");

  /* This corresponds to bounding box */
  fprintf (this->SVG,
           "<svg width=\"%.1f\" height=\"%.1f\">\n",
           DEV (graph_width), DEV (graph_height));

  /* Set initial font... */
  this->current_text_size = 11;

  return (painter_t *) this;
}

#define SVGY(y) (y)

void
svg_painter_delete (painter_t * painter)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;

  fprintf (svg_painter->SVG, "</svg>\n");

  fclose (svg_painter->SVG);

  g_free (svg_painter);
}

static void
svg_painter_set_attributes (painter_t * painter,
                            GdkColor color,
                            double line_width,
                            int line_style,
                            gint mark_type,
                            gdouble mark_size_x,
                            gdouble mark_size_y)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;

  /* Override invisible line width with something reasonable for
     svg. */
  if (line_width == 0)
    line_width = 1;

  svg_painter->current_mark_type = mark_type;
  svg_painter->current_mark_size_x = mark_size_x;
  svg_painter->current_mark_size_y = mark_size_y;
  svg_painter->current_color = color;
  svg_painter->current_line_width = line_width;
}

static void
svg_painter_draw_line (painter_t * painter,
                       double x1, double y1, double x2, double y2)
{
  GArray *segments = g_array_new (FALSE, FALSE, sizeof (seg_t));
  seg_t seg;

  /* Convert points to segments use function below */
  seg.x1 = x1;
  seg.y1 = y1;
  seg.x2 = x2;
  seg.y2 = y2;

  g_array_append_val (segments, seg);

  svg_painter_draw_segments (painter, segments);

  g_array_free (segments, TRUE);
}

static void
svg_painter_draw_segments (painter_t * painter, GArray * segments)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;
  seg_t *segs = (seg_t *) segments->data;
  int seg_idx;

  fprintf (svg_painter->SVG,
           "<g style=\"stroke:#%02x%02x%02x;stroke-width:%f;fill:none\">\n",
           svg_painter->current_color.red / 256,
           svg_painter->current_color.green / 256,
           svg_painter->current_color.blue / 256,
           1.0 * DEV (svg_painter->current_line_width));

  fprintf (svg_painter->SVG, "<path d=\"");
  for (seg_idx = 0; seg_idx < segments->len; seg_idx++)
    {
      if (seg_idx == 0)
        {
          fprintf (svg_painter->SVG, "M %f,%f ",
                   DEV (segs[seg_idx].x1), DEV (SVGY (segs[seg_idx].y1)));
        }
      else if ((segs[seg_idx].x1 != segs[seg_idx - 1].x2) ||
               (segs[seg_idx].y1 != segs[seg_idx - 1].y2))
        {
          // Close polyline and open a new one
          fprintf (svg_painter->SVG,
                   "M %f,%f ",
                   DEV (segs[seg_idx].x1), DEV (SVGY (segs[seg_idx].y1)));
        }
      fprintf (svg_painter->SVG,
               "L %f,%f ", DEV (segs[seg_idx].x2),
               DEV (SVGY (segs[seg_idx].y2)));
    }
  fprintf (svg_painter->SVG, "\"/>\n</g>\n");
}

static void
svg_painter_draw_marks (painter_t * painter, GArray * marks_array)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;
  mark_t *marks = (mark_t *) marks_array->data;
  FILE *SVG = svg_painter->SVG; /* Shortcut */
  int m_idx;
  int mark_type = svg_painter->current_mark_type;
  int rr = svg_painter->current_color.red / 256;
  int gg = svg_painter->current_color.green / 256;
  int bb = svg_painter->current_color.blue / 256;

  /* TBD: Compact this by defining the style once for each set of
     points! */
  for (m_idx = 0; m_idx < marks_array->len; m_idx++)
    {
      double x = marks[m_idx].x;
      double y = SVGY (marks[m_idx].y);

      /* Define postscript for the current mark */
      if (mark_type == MARK_TYPE_FCIRCLE)
        {
          fprintf (SVG,
                   "<circle cx=\"%f\" cy=\"%f\" r=\"%f\" fill=\"rgb(%d,%d,%d)\"/>\n",
                   DEV (x), DEV (y),
                   DEV (svg_painter->current_mark_size_x / 2), rr, gg, bb);
        }
      else if (mark_type == MARK_TYPE_CIRCLE)
        {
          fprintf (SVG,
                   "<circle cx=\"%f\" cy=\"%f\" r=\"%f\" stroke=\"rgb(%d,%d,%d)\"/> stroke_width=\"%f\"\n",
                   DEV (x), DEV (y),
                   DEV (svg_painter->current_mark_size_x / 2), rr, gg, bb,
                   1.0 * DEV (svg_painter->current_line_width));
        }
      /* TBD! */
#if 0
      else if (mark_type == MARK_TYPE_SQUARE)
        fprintf (SVG, "");
      else if (mark_type == MARK_TYPE_FSQUARE)
        fprintf (SVG, "");
#endif
    }

}

static void
svg_painter_draw_text (struct painter_t_struct *painter,
                       double x_pos, double y_pos,
                       const char *text, int just, int style)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;
  double text_size = svg_painter->current_text_size;
  char *anchor_names[] =
    { "text-anchor=\"middle\" alignment-baseline=\"middle\"",
    "text-anchor=\"start\" alignment-baseline=\"middle\"",
    "text-anchor=\"start\" alignment-baseline=\"middle\"",
    "text-anchor=\"middle\"",
    "text-anchor=\"end\"",
    "text-anchor=\"end\"",
    "text-anchor=\"end\"",
    "text-anchor=\"middle\"",
    "text-anchor=\"start\""
  };

  // This should be more configurable...
  if (style == T_TITLE)
    text_size = 18;
  else if (style == T_AXIS)
    text_size = 11;

  if (just == T_RIGHT)
    {
      /* The following is emperical for Helvetica fonts... */
      y_pos += 0.37 * text_size;
    }
  else if (just == T_LOWERLEFT)
    {
    }
  else if (just == T_UPPERLEFT)
    {
      y_pos += 0.8 * text_size;
    }
  else if (just == T_BOTTOM)
    {
    }
  else if (just == T_TOP)
    {
      y_pos += 0.74 * text_size;
    }

  // Meanwhile do it the long and hard way...
  fprintf (svg_painter->SVG,
           "<text %s font-size=\"%f\" fill=\"RGB(%d,%d,%d)\" x=\"%f\" y=\"%f\">%s</text>\n",
           anchor_names[just],
           DEV (svg_painter->current_text_size),
           svg_painter->current_color.red / 256,
           svg_painter->current_color.green / 256,
           svg_painter->current_color.blue / 256,
           DEV (x_pos), DEV (SVGY (y_pos)), text);
}

static void
svg_painter_set_attributes_style (painter_t * painter, int style)
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
  svg_painter_set_attributes (painter,
                              color, line_width, line_style, 0, 1.0, 1.0);
}

static void
svg_painter_group_start (struct painter_t_struct *painter,
                         const char *group_name)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;
  fprintf (svg_painter->SVG, "<g id=\"%s\">\n", group_name);
}

static void
svg_painter_group_end (struct painter_t_struct *painter,
                       const char *group_name)
{
  svg_painter_t *svg_painter = (svg_painter_t *) painter;

  fprintf (svg_painter->SVG, "</g>\n");
}
