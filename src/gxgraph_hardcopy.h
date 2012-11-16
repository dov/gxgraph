/*======================================================================
//  gxgraph_hardcopy.h - Hardcopy dialog
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//  Sat Jan 22 23:19:15 2005
//----------------------------------------------------------------------
*/
#ifndef GXGRAPH_HARDCOPY_H
#define GXGRAPH_HARDCOPY_H

#include "gxgraph.h"

GtkWidget *gxgraph_hardcopy_dialog_new (GtkWidget * parent,
					window_t * window);
void gxgraph_hardcopy_dialog_destroy (GtkWidget * gxgraph_hardcopy);

#endif /* GXGRAPH_HARDCOPY */
