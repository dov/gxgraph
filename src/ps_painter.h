/*======================================================================
//  pspainter.h - 
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#ifndef PS_PAINTER_H
#define PS_PAINTER_H

painter_t *ps_painter_new (window_t * window, const char *filename);
void ps_painter_delete (painter_t * painter);

#endif /* PS_PAINTER */
