/*======================================================================
//  parser.c - This is the parser for the syntax read by the gxgraph.
//  It is a combination of the syntaxes supported by giv and xgraph.
//
//  Dov Grobgeld <dov@orbotech.com>
//  Tue Feb  1 09:05:14 2005
//----------------------------------------------------------------------
*/

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include "gxgraph.h"
#include "parser.h"

#define NCASE(s) if (!g_strcasecmp(s, S_))


/*======================================================================
//
// General string handling in C. These functions should be replaced
// by something in glib.
//
//----------------------------------------------------------------------*/
int
string_count_words (const char *string)
{
  int nwords = 0;
  const char *p = string;
  int in_word = 0;
  while (*p)
    {
      if (!in_word)
	{
	  if ((*p != ' ') && (*p != '\n') && (*p != '\t'))
	    in_word = 1;
	}
      else if (in_word)
	{
	  if ((*p == ' ') || (*p == '\n') || (*p != '\t'))
	    {
	      in_word = 0;
	      nwords++;
	    }
	}
      p++;
    }
  if (in_word)
    nwords++;
  return nwords;
}

char *
string_strdup_word (const char *string, int idx)
{
  const char *p = string;
  int in_word = 0;
  int word_count = -1;
  int nchr = 1;
  char *word = NULL;
  const char *word_start = string;

  /* printf("p = 0x%x\n", p); */
  while (*p)
    {
      /* printf("%c\n", *p); fflush(stdout); */
      if (!in_word)
	{
	  if (*p != ' ' && *p != '\n' && *p != '\t')
	    {
	      in_word = 1;
	      word_count++;
	      nchr = 1;
	      word_start = p;
	    }
	}
      else if (in_word)
	{
	  if (*p == ' ' || *p == '\n' || *p == '\t')
	    {
	      if (idx == word_count)
		break;
	      in_word = 0;
	    }
	}
      nchr++;
      p++;
    }
  if (in_word)
    {
      word = g_new (char, nchr);
      strncpy (word, word_start, nchr - 1);
      word[nchr - 1] = 0;
    }
  return word;
}

char *
string_strdup_rest (const char *string, int idx)
{
  const char *p = string;
  int in_word = 0;
  int word_count = -1;
  int nchr = 1;
  char *word = NULL;
  const char *word_start = string;

  /* printf("p = 0x%x\n", p); */
  while (*p)
    {
      /* printf("%c\n", *p); fflush(stdout); */
      if (!in_word)
	{
	  if (*p != ' ' && *p != '\n' && *p != '\t')
	    {
	      in_word = 1;
	      word_count++;
	      nchr = 1;
	      word_start = p;
	    }
	}
      else if (in_word)
	{
	  if (*p == ' ' || *p == '\n' || *p == '\t')
	    {
	      if (idx == word_count)
		break;
	      in_word = 0;
	    }
	}
      nchr++;
      p++;
    }

  if (in_word)
    {
      nchr = strlen (word_start);
      word = g_new (char, nchr + 1);
      strncpy (word, word_start, nchr);
      word[nchr] = 0;
    }
  return word;
}

int
string_to_atoi (const char *string, int idx)
{
  char *word = string_strdup_word (string, idx);
  int value = atoi (word);
  g_free (word);

  return value;
}

gdouble
string_to_atof (const char *string, int idx)
{
  char *word = string_strdup_word (string, idx);
  gdouble value = atof (word);
  g_free (word);

  return value;
}

/*======================================================================
//  Classify a string.
//----------------------------------------------------------------------
*/
gint
gxgraph_parse_string (const char *string, char *fn, gint linenum)
{
  gint type = -1;
  gchar first_char = string[0];
  gchar *first_word;

  /* Shortcut for speeding up drawing */
  if (first_char >= '0' && first_char <= '9')
    return STRING_DRAW;

  first_word = string_strdup_word (string, 0);

  if (first_char == '#')
    type = STRING_COMMENT;
  else if (first_char == '$')
    {
      char *S_ = first_word;
      NCASE ("$lw")
      {
	type = STRING_CHANGE_LINE_WIDTH;
      }
      NCASE ("$color")
      {
	type = STRING_CHANGE_COLOR;
      }
      NCASE ("$outline_color")
      {
	type = STRING_CHANGE_OUTLINE_COLOR;
      }
      NCASE ("$marks")
      {
	type = STRING_CHANGE_MARKS;
      }
      NCASE ("$noline")
      {
	type = STRING_CHANGE_NO_LINE;
      }
      NCASE ("$scale_marks")
      {
	type = STRING_CHANGE_SCALE_MARKS;
      }
      NCASE ("$path")
      {
	type = STRING_PATH_NAME;
      }
      NCASE ("$mark_size")
      {
	type = STRING_CHANGE_MARK_SIZE;
      }
      NCASE ("$text_size")
      {
	type = STRING_CHANGE_TEXT_SIZE;
      }
      NCASE ("$nomark")
      {
	type = STRING_CHANGE_NO_MARK;
      }
      NCASE ("$line")
      {
	type = STRING_CHANGE_LINE;
      }
      NCASE ("$image")
      {
	type = STRING_IMAGE_REFERENCE;
      }
      NCASE ("$polygon")
      {
	type = STRING_CHANGE_POLYGON;
      }
      NCASE ("$marks_file")
      {
	type = STRING_MARKS_REFERENCE;
      }
      NCASE ("$low_contrast")
      {
	type = STRING_LOW_CONTRAST;
      }
      NCASE ("$title")
      {
	type = STRING_SET_TITLE;
      }
      NCASE ("$name")
      {
	type = STRING_SET_NAME;
      }
      if (type == -1)
	{
	  fprintf (stderr, "Unknown parameter %s in file %s line %d!\n", S_,
		   fn, linenum);
	}
    }
  else if (((first_char >= 'a' && first_char <= 'z')
	    || (first_char >= 'A' && first_char <= 'Z'))
	   && first_word[strlen (first_word) - 1] == ':')
    {
      char *S_ = first_word;

      while(1) /* So that we may break */
	{
	  NCASE("Title:") { type = STRING_SET_TITLE; break;}
	  NCASE("TitleText:") { type = STRING_SET_TITLE; break;}
	  NCASE("XUnitText:") { type = STRING_SET_XUNIT_TEXT; break; }
	  NCASE("YUnitText:") { type = STRING_SET_YUNIT_TEXT; break; }
	  NCASE("LargePixels:") { type = STRING_SET_LARGE_PIXELS; break; }

	  printf("Unsupported keyword=%s\n", S_);
	  // TBD - Recognize more xgraph keywords...
	  type = STRING_NOP;

	  break;
	}

    }
  else if (first_char == 'M' || first_char == 'm')
    {
      type = STRING_MOVE;
    }
  else if (first_char == 'T' || first_char == 't')
    {
      type = STRING_TEXT;
    }
  else if (first_char == '"')
    {
      type = STRING_SET_NAME;
    }
  else
    {
      type = STRING_DRAW;
    }

  g_free (first_word);

  return type;
}

gint
gxgraph_parse_mark_type (const char *S_, gchar * fn, gint linenum)
{
  NCASE ("circle") return MARK_TYPE_CIRCLE;
  NCASE ("fcircle") return MARK_TYPE_FCIRCLE;
  NCASE ("square") return MARK_TYPE_SQUARE;
  NCASE ("fsquare") return MARK_TYPE_FSQUARE;
  NCASE ("pixel") return MARK_TYPE_PIXEL;
  fprintf (stderr, "Unknown mark %s in file %s line %d\n", S_, fn, linenum);
  return MARK_TYPE_CIRCLE;
}

int split_string_to_double_pair(const char *S_,
				/* output */
				double *low,
				double *hi)
{
  char *comma_pos = strchr(S_, ',');
  char *first_copy;

  if (comma_pos == NULL)
    return -1;

  first_copy = g_strdup(S_);

  /* Change comma position to end of string so that I can use
     atof(). */
  first_copy[comma_pos - S_] = '\0';

  *low = atof(first_copy);
  *hi = atof(comma_pos+1);

  g_free(first_copy);

  return 0;
}
