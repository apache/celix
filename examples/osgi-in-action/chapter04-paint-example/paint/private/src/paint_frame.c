/*
 * paint_frame.c
 *
 *  Created on: Aug 22, 2011
 *      Author: operator
 */


/**
 * This class represents the main application class, which is a JFrame subclass
 * that manages a toolbar of shapes and a drawing canvas. This class does not
 * directly interact with the underlying OSGi framework; instead, it is injected
 * with the available <tt>SimpleShape</tt> instances to eliminate any
 * dependencies on the OSGi application programming interfaces.
 **/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "bundle_context.h"
#include "bundle.h"
#include "utils.h"
#include "hash_map.h"
#include "simple_shape.h"
#include "linked_list_iterator.h"
#include "linkedlist.h"
#include "paint_frame.h"
#include "shape_component.h"
#include "default_shape.h"

static PAINT_FRAME this = NULL;

struct gtk_thread_data {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;
};
struct shape_info {
	char *name;
	SIMPLE_SHAPE m_shape;
	GtkWidget *button;
};

typedef struct shape_info *SHAPE_INFO;


static SHAPE_INFO shapeInfo_create(char* name, GtkWidget *button, SIMPLE_SHAPE shape){
	SHAPE_INFO info = malloc(sizeof(*info));
	info->m_shape = shape;
	info->name = name;
	info->button = button;
	return info;
}
static void paintFrame_show() {
	gtk_widget_show (this->m_drawingArea);
	gtk_widget_show (this->m_mainPanel);
	gtk_widget_show (this->m_toolBar);
	gtk_widget_show (this->m_window);
}

/* main function */
static gpointer _gtkthread(gpointer a_data) {
	gdk_threads_enter();
	gtk_main ();
	gdk_threads_leave();
	return NULL;
}

static void redraw(GtkWidget *widget, GdkModifierType state){
	if (this->m_pixMap != NULL) {
		GdkRectangle update_rect;

		update_rect.x = 0;
		update_rect.y = 0;
		update_rect.width = widget->allocation.width;
		update_rect.height = widget->allocation.height;
		gdk_draw_rectangle (this->m_pixMap,
				widget->style->white_gc,
				TRUE,
				update_rect.x, update_rect.y,
				update_rect.width, update_rect.height);
		gtk_widget_draw(widget, &update_rect);
		LINKED_LIST_ITERATOR it = linkedListIterator_create(this->m_shapeComponents, 0);
		while (linkedListIterator_hasNext(it)){
			SHAPE_COMPONENT sc = linkedListIterator_next(it);
			(*sc->shapeComponent_paintComponent)(sc, this, this->m_pixMap, widget);
		}
	}
}


static void button_handler( GtkWidget *widget,
                   	   gpointer   data )
{
    this->m_selected = (gchar *) data;
}
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}
static gboolean delete(GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
    return TRUE;
}
/******************************** mouse event handler *******************/
/**
 * notification of a mouse motion
 */
static gboolean
motion_notify_event( GtkWidget *widget, GdkEventMotion *event )
{
  int x, y;
  GdkModifierType state;
  if (event->is_hint) {
    gdk_window_get_pointer (event->window, &x, &y, &state);
  } else {
      x = event->x;
      y = event->y;
      state = event->state;
    }
  return TRUE;
}
/******************************** drawable event handler *******************/
/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event )
{
	gdk_draw_drawable(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			this->m_pixMap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	return TRUE;
}
/* configuration events  */
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event )
{
  if (this->m_pixMap) {
    g_object_unref(this->m_pixMap);
  }

  this->m_pixMap = gdk_pixmap_new(widget->window,
              widget->allocation.width,
              widget->allocation.height,
              -1);
  redraw(widget, 0);
  return TRUE;
}

static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event )
{
  if (event->button == 1 && this->m_pixMap != NULL) {
	  if (this->m_selected == NULL){
		 printf("no button selected yet\n");
	  } else {
	    SHAPE_COMPONENT sc = shapeComponent_create(this, this->m_selected, event->x, event->y);
	    linkedList_addFirst(this->m_shapeComponents, sc);
	    (*sc->shapeComponent_paintComponent)(sc, this, this->m_pixMap, widget);
	  }
  }
  return TRUE;
}


/**
 * This method sets the currently selected shape to be used for drawing on the
 * canvas.
 *
 * @param name The name of the shape to use for drawing on the canvas.
 **/
void paintFrame_selectShape(char *name) {
	gdk_threads_enter();
	this->m_selected = name;
	gdk_threads_leave();
}

/**
 * Retrieves the available <tt>SimpleShape</tt> associated with the given
 * name.
 *
 * @param name The name of the <tt>SimpleShape</tt> to retrieve.
 * @return The corresponding <tt>SimpleShape</tt> instance if available or
 *         <tt>null</tt>.
 **/
SIMPLE_SHAPE paintFrame_getShape(char *name) {
	SHAPE_INFO info = (SHAPE_INFO) hashMap_get(this->m_shapes, name);
	if (info == NULL) {
		return this->m_defaultShape;
	} else {
		return info->m_shape;
	}
}

/**
 * Injects an available <tt>SimpleShape</tt> into the drawing frame.
 *
 * @param name The name of the injected <tt>SimpleShape</tt>.
 * @param icon The icon associated with the injected <tt>SimpleShape</tt>.
 * @param shape The injected <tt>SimpleShape</tt> instance.
 **/
void paintFrame_addShape(BUNDLE_CONTEXT context, char* name, void *icon, SIMPLE_SHAPE shape) {
	gdk_threads_enter();
	char png_file[30], *path;
	GError *gerror = NULL;
	BUNDLE bundle;
	apr_pool_t *pool;
	/* Add the buttons to the graph navigation panel. */
	GtkWidget *button = gtk_button_new();
	sprintf(png_file, "%s.png", name);
	bundleContext_getBundle(context, &bundle);
	bundleContext_getMemoryPool(context, &pool);
	celix_status_t status = bundle_getEntry(bundle, png_file, pool, &path);
	if (status == CELIX_SUCCESS) {
		GtkWidget *im = gtk_image_new_from_file(path);
		gtk_button_set_image((GtkButton *) button, im);
		gsize rd = 0, wr = 0;
		gchar *gfn = g_locale_to_utf8(name, strlen(name), &rd, &wr, &gerror);
		//gtk_widget_set_size_request(button, 10, 10);
		gtk_button_set_label((GtkButton *) button, gfn);
		gtk_box_pack_start (GTK_BOX(this->m_toolBar), button, TRUE, TRUE, 0);
	} else {
		printf("Cannot find resource %s\n", png_file);
	}
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (button_handler), name);
	gtk_widget_show(button);
	if (hashMap_get(this->m_shapes, name) == NULL) {
		hashMap_put(this->m_shapes, name, shapeInfo_create(name, button, shape));
	}
	redraw(this->m_drawingArea, 0);
	paintFrame_show();
	gdk_threads_leave();
}

/**
 * Removes a no longer available <tt>SimpleShape</tt> from the drawing frame.
 *
 * @param name The name of the <tt>SimpleShape</tt> to remove.
 **/
void paintFrame_removeShape(char *name) {
	gdk_threads_enter();
	SHAPE_INFO shape = (SHAPE_INFO) hashMap_remove(this->m_shapes, name);
	this->m_selected = NULL;
	gtk_container_remove(GTK_CONTAINER(this->m_toolBar), shape->button);
	gtk_widget_show_all(this->m_toolBar);
	redraw(this->m_drawingArea, 0);
	paintFrame_show();
	gdk_threads_leave();
}


/**
 * Default constructor that populates the main window.
 **/
PAINT_FRAME paintFrame_create(BUNDLE_CONTEXT context, apr_pool_t *pool) {
	GError *error = NULL;
	GThread *l_th;
	struct gtk_thread_data *data = apr_palloc(pool, sizeof(*data));
	data->pool = pool;
	data->context = context;
	this = malloc(sizeof(*this));
	/* set the methods */
	this->paintFrame_addShape = paintFrame_addShape;
	this->paintFrame_getShape = paintFrame_getShape;
	this->paintFrame_removeShape = paintFrame_removeShape;
	this->paintFrame_selectShape = paintFrame_selectShape;
	this->paintFrame_show = paintFrame_show;
	this->m_selected = NULL;
	this->m_pixMap = NULL;
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(NULL, NULL);
	this->m_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (this->m_window), 10);
	gtk_window_set_title (GTK_WINDOW (this->m_window), "OSGi in Action, Paint-Example");

	g_signal_connect (G_OBJECT (this->m_window), "delete_event",
			G_CALLBACK (delete), NULL);

	g_signal_connect (G_OBJECT (this->m_window), "destroy",
			G_CALLBACK (destroy), NULL);
	/* Create the graph navigation panel and add it to the window. */
	this->m_toolBar = gtk_hbox_new (FALSE, 0);
	this->m_mainPanel = gtk_vbox_new(FALSE, 0);

	gtk_container_add  (GTK_CONTAINER (this->m_window), this->m_mainPanel);


	this->m_drawingArea = gtk_drawing_area_new();

	gtk_signal_connect (GTK_OBJECT (this->m_drawingArea), "expose_event",
			(GtkSignalFunc) expose_event, NULL);
	gtk_signal_connect (GTK_OBJECT(this->m_drawingArea),"configure_event",
			(GtkSignalFunc) configure_event, NULL);
	gtk_signal_connect (GTK_OBJECT (this->m_drawingArea), "motion_notify_event",
			(GtkSignalFunc) motion_notify_event, NULL);
	gtk_signal_connect (GTK_OBJECT (this->m_drawingArea), "button_press_event",
			(GtkSignalFunc) button_press_event, NULL);

	gtk_widget_set_events (this->m_drawingArea, GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

	gtk_widget_set_size_request(this->m_drawingArea, 200, 200);

	/*Add the graph navigation panel to the main panel. */
	gtk_box_pack_start (GTK_BOX(this->m_mainPanel), this->m_toolBar, TRUE, TRUE, 0);
	/* Add the draw-able area to the main panel. */
	gtk_box_pack_start (GTK_BOX(this->m_mainPanel), this->m_drawingArea, TRUE, TRUE, 0);

	this->m_shapes = hashMap_create(string_hash, NULL, string_equals, NULL);
	linkedList_create(data->pool, &this->m_shapeComponents);
	this->m_defaultShape = defaultShape_create(data->context);
	paintFrame_show();
	if( g_thread_supported()) {
		l_th = g_thread_create(_gtkthread, data, TRUE, &error);
		if (l_th == NULL){
			g_printerr ("Failed to create thread: %s\n", error->message);
			return NULL;
		}
	} else {
		g_printerr("g_thread NOT supported\n");
	}
	return this;
}


