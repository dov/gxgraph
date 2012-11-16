/*======================================================================
//  gtkpainter.h - 
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//
//  Copyright: See COPYING file that comes with this distribution
//----------------------------------------------------------------------
*/
#ifndef GTKPAINTER_H
#define GTKPAINTER_H

painter_t *gtk_painter_new (window_t * window);
void gtk_painter_delete (painter_t * painter);

#endif /* GTKPAINTER */
