/*======================================================================
//  svg_painter.h - painter for the svg format
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#ifndef SVGPAINTER_H
#define SVGPAINTER_H

painter_t *svg_painter_new (window_t * window, const char *filename);
void svg_painter_delete (painter_t * painter);

#endif /* SVGPAINTER */
