/*======================================================================
//  gxgraph_hardcopy.c - Hardcopy dialog
//
//  Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
//  Sat Jan 22 23:10:41 2005
//----------------------------------------------------------------------
*/
#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gxgraph.h"
#include "ps_painter.h"
#include "svg_painter.h"

typedef enum
{
  OUTPUT_DEVICE_PRINT,
  OUTPUT_DEVICE_POSTSCRIPT,
  OUTPUT_DEVICE_SVG
} output_device_t;

static void cb_response (GtkWidget * dialog,
			 gint response, gpointer user_data);
static GtkWidget *make_menu_item (gchar * name,
				  GtkWidget * menu, gpointer data);

static int cb_menu_item_assign_variable (GtkWidget * widget, int val);

// Can make this static as there can only be one hardcopy dialog.
static output_device_t output_device = 0;
static char *output_device_name = NULL;

GtkWidget *
gxgraph_hardcopy_dialog_new (GtkWidget * parent, window_t * window)
{
  GtkWidget *dialog;
  GtkWidget *dialog_vbox;
  GtkWidget *table, *button, *entry;
  int row;

  dialog = gtk_dialog_new_with_buttons ("Hardcopy",
					GTK_WINDOW (parent),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  g_signal_connect (GTK_OBJECT (dialog),
		    "response", G_CALLBACK (cb_response), window);

  dialog_vbox = GTK_DIALOG (dialog)->vbox;

  /* Create an interface like in xgraph */
  /*
     Output device: [PostScript] [SVG]
     Disposition:   [Print] [File]
     File or device name:
   */
  table = gtk_table_new (2, 2, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), table, FALSE, FALSE, 0);
  row = 0;
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Output device:"),
		    0, 1, row, row + 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) 0, 3, 0);
  button = gtk_option_menu_new ();
  // Populate widget
  {
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;

    g_object_set_data (G_OBJECT (menu), "varpointer", &output_device);

    item = make_menu_item ("Print",
			   menu, GINT_TO_POINTER (OUTPUT_DEVICE_PRINT));
    gtk_menu_append (GTK_MENU (menu), item);
    if (output_device == OUTPUT_DEVICE_PRINT)
      gtk_menu_item_select (GTK_MENU_ITEM (item));

    item = make_menu_item ("Export PostScript",
			   menu, GINT_TO_POINTER (OUTPUT_DEVICE_POSTSCRIPT));
    gtk_menu_append (GTK_MENU (menu), item);
    if (output_device == OUTPUT_DEVICE_POSTSCRIPT)
      gtk_menu_item_select (GTK_MENU_ITEM (item));

    item = make_menu_item ("Export SVG",
			   menu, GINT_TO_POINTER (OUTPUT_DEVICE_SVG));
    if (output_device == OUTPUT_DEVICE_SVG)
      gtk_menu_item_select (GTK_MENU_ITEM (item));
    gtk_menu_append (GTK_MENU (menu), item);

    /* attach the menu to the button */
    gtk_option_menu_set_menu (GTK_OPTION_MENU (button), menu);
  }
  gtk_table_attach (GTK_TABLE (table),
		    button,
		    1, 2, row, row + 1,
		    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  // Entry label for filename or device name...
  row++;
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Device or filename:"),
		    0, 1, row, row + 1,
		    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 3, 0);
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table),
		    entry,
		    1, 2, row, row + 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) 0, 0, 0);
  g_object_set_data (G_OBJECT (dialog), "filename_entry", entry);

  /* Show it all */
  gtk_widget_show_all (dialog_vbox);

  return dialog;
}

void
gxgraph_hardcopy_destroy (GtkWidget * widget)
{
  gtk_widget_destroy (widget);
}

static void
cb_response (GtkWidget * dialog, gint response, gpointer user_data)
{
  window_t *window = user_data;
  gchar *psdevice_name = NULL;

  if (response == GTK_RESPONSE_OK)
    {
      const gchar *filename =
	gtk_entry_get_text (GTK_ENTRY (g_object_get_data (G_OBJECT (dialog),
							  "filename_entry")));
      painter_t *painter;

      if (output_device_name)
	g_free (output_device_name);
      output_device_name = g_strdup (filename);

      switch (output_device)
	{
	case OUTPUT_DEVICE_PRINT:
	  if (strlen (filename))
	    psdevice_name = g_strdup_printf ("|lp -d'%s'", filename);
	  else
	    psdevice_name = g_strdup ("|lp");
	  filename = psdevice_name;
	  painter = ps_painter_new (window, filename);
	  break;
	case OUTPUT_DEVICE_POSTSCRIPT:
	  painter = ps_painter_new (window, filename);
	  break;
	case OUTPUT_DEVICE_SVG:
	  painter = svg_painter_new (window, filename);
	  break;
	}

      gxgraph_draw_window (window, painter);

      switch (output_device)
	{
	case OUTPUT_DEVICE_PRINT:
	  g_free (psdevice_name);
	  ps_painter_delete (painter);
	  break;
	case OUTPUT_DEVICE_POSTSCRIPT:
	  ps_painter_delete (painter);
	  break;
	case OUTPUT_DEVICE_SVG:
	  svg_painter_delete (painter);
	  break;
	}
    }

  if (response != GTK_RESPONSE_APPLY)
    {
    }
  gxgraph_hardcopy_destroy (dialog);
}

/*======================================================================
// Widget construction utilities.
//----------------------------------------------------------------------*/
static GtkWidget *
make_menu_item (gchar * name, GtkWidget * menu, gpointer data)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_label (name);
  g_object_set_data (G_OBJECT (item), "varpointer",
		     g_object_get_data (G_OBJECT (menu), "varpointer"));
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      G_CALLBACK (cb_menu_item_assign_variable), data);
  gtk_widget_show (item);

  return (item);
}

static int
cb_menu_item_assign_variable (GtkWidget * widget, int val)
{
  int *varpointer = g_object_get_data (G_OBJECT (widget),
				       "varpointer");
  *varpointer = val;

  return 0;
}
