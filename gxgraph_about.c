/*======================================================================
//  gxgraph_about.c - About dialog
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//  Sun Jan 23 00:35:08 2005
//----------------------------------------------------------------------
*/

#include <gtk/gtk.h>
#include "version.h"
#define _(s) s

void
gxgraph_about_dialog (GtkWidget * parent)
{
  GtkWidget *about_window;
  GtkWidget *vbox, *hbox;
  GtkWidget *label;
  //    GtkWidget *image;

  gchar *markup;

  about_window = gtk_dialog_new_with_buttons ("About gxgraph",
					      GTK_WINDOW (parent),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);

  gtk_window_set_resizable (GTK_WINDOW (about_window), FALSE);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->vbox), vbox, FALSE,
		      FALSE, 0);

  /*
     image = gtk_image_new_from_pixbuf (gxgraph_get_logo());
     gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
   */

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  markup =
    g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">gxgraph "
		     VERSION "</span>\n\n" "%s\n\n"
		     "This program is released under the GPL license\n"
		     "<span>%s</span>\n",
		     _("An interactive plotting program"),
		     _("&#x00a9; Dov Grobgeld, 2005\n"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  hbox = gtk_hbox_new (TRUE, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 60);

  gtk_widget_show_all (about_window);
  gtk_dialog_run (GTK_DIALOG (about_window));
  gtk_widget_destroy (about_window);


}
