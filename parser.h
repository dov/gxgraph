/*======================================================================
//  parser.h - Parser for gxgraph
//
//  Dov Grobgeld <dov@orbotech.com>
//  Tue Feb  1 09:12:31 2005
//----------------------------------------------------------------------
*/
#ifndef PARSER_H
#define PARSER_H

#include <glib.h>

enum
{
  STRING_NOP,
  STRING_DRAW,
  STRING_COMMENT,
  STRING_MOVE,
  STRING_TEXT,
  STRING_CHANGE_COLOR,
  STRING_CHANGE_OUTLINE_COLOR,
  STRING_CHANGE_LINE_WIDTH,
  STRING_CHANGE_MARKS,
  STRING_CHANGE_NO_LINE,
  STRING_CHANGE_SCALE_MARKS,
  STRING_CHANGE_MARK_SIZE,
  STRING_CHANGE_TEXT_SIZE,
  STRING_CHANGE_LINE,
  STRING_CHANGE_NO_MARK,
  STRING_CHANGE_POLYGON,
  STRING_IMAGE_REFERENCE,
  STRING_MARKS_REFERENCE,
  STRING_LOW_CONTRAST,
  STRING_PATH_NAME,
  STRING_SET_NAME,
  STRING_SET_XUNIT_TEXT,
  STRING_SET_YUNIT_TEXT,
  STRING_SET_LARGE_PIXELS,
  STRING_SET_TITLE
};

gint gxgraph_parse_string (const char *string, char *fn, gint linenum);
gint gxgraph_parse_mark_type (const char *S_, gchar * fn, gint linenum);
char *string_strdup_rest (const char *string, int idx);
int string_to_atoi (const char *string, int idx);
gdouble string_to_atof (const char *string, int idx);
char *string_strdup_word (const char *string, int idx);
int string_count_words (const char *string);
int    split_string_to_double_pair(const char *string,
				   /* output */
				   double *low,
				   double *hi);
void string_shorten_whitespace(char *string);

#endif /* PARSER */
