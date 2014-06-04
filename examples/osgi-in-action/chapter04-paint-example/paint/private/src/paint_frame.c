/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * paint_frame.c
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


/*
 * This class represents the main application class, which is a JFrame subclass
 * that manages a toolbar of shapes and a drawing canvas. This class does not
 * directly interact with the underlying OSGi framework; instead, it is injected
 * with the available <tt>SimpleShape</tt> instances to eliminate any
 * dependencies on the OSGi application programming interfaces.
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <celixbool.h>
#include <glib.h>
#include <gdk/gdk.h>
#include "bundle_context.h"
#include "bundle.h"
#include "utils.h"
#include "hash_map.h"
#include "simple_shape.h"
#include "linked_list_iterator.h"
#include "linked_list.h"
#include "paint_frame.h"
#include "shape_component.h"
#include "default_shape.h"
#include "celix_errno.h"

static paint_frame_pt this = NULL;

struct shape_info {
	char *name;
	simple_shape_pt shape;
	GtkWidget *button;
};

typedef struct shape_info *shape_info_pt;
static celix_status_t paintFrame_redraw(paint_frame_pt frame, GdkModifierType state);
static celix_status_t paintFrame_show(paint_frame_pt frame);
static void paintFrame_destroy(GtkWidget *widget, gpointer data);
static void paintFrame_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static void paintFrame_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void paintFrame_buttonClicked(GtkWidget *button, gpointer data);
static gboolean paintFrame_mousePressed( GtkWidget *widget, GdkEventButton *event, gpointer data);
static gpointer paintFrame_gtkmain(gpointer a_data);
static void paintFrame_destroyWidgets(paint_frame_pt frame);

/**
 * Default constructor that populates the main window.
 **/
celix_status_t paintFrame_create(bundle_context_pt context, apr_pool_t *pool, paint_frame_pt *frame) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *mypool = NULL;
	apr_pool_create(&mypool, pool);
	this = malloc(sizeof(*this));
	if (!this) {
		this = NULL;
		status = CELIX_ENOMEM;
	} else {
		char *builderFile;
		bundle_pt bundle;
		GError *error = NULL;
		*frame = this;
		

		(*frame)->showing = false;
		(*frame)->pool = mypool;
		(*frame)->pixMap = NULL;
		(*frame)->m_selected = NULL;
		(*frame)->context = context;
		(*frame)->m_shapes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*frame)->m_defaultShape = defaultShape_create((*frame)->context);
		linkedList_create((*frame)->pool, &(*frame)->m_shapeComponents);


		status = bundleContext_getBundle(context, &bundle);
		if (status == CELIX_SUCCESS) {
			status = bundle_getEntry(bundle, "gtktest.glade", mypool, &builderFile);
			if (status == CELIX_SUCCESS) {
				(*frame)->file = builderFile;

				gdk_threads_init();
				gtk_init(NULL, NULL);

				if( g_thread_supported()) {
					// (*frame)->main = g_thread_new("main", paintFrame_gtkmain, (*frame));
					(*frame)->main = g_thread_create(paintFrame_gtkmain, (*frame), TRUE, &error);
					if ((*frame)->main == NULL){
						g_printerr ("Failed to create thread: %s\n", error->message);
						status = CELIX_BUNDLE_EXCEPTION;
					}
				} else {
					g_printerr("g_thread NOT supported\n");
				}
			}
		}
	}

	return status;
}

celix_status_t paintFrame_exit(paint_frame_pt frame) {
	frame->showing = false;

	paintFrame_destroyWidgets(frame);

	gdk_threads_enter();

	gtk_main_quit();

	gdk_threads_leave();

	g_thread_join(frame->main);

	return CELIX_SUCCESS;
}

static celix_status_t shapeInfo_create(paint_frame_pt frame, char* name, GtkWidget *button, simple_shape_pt shape, shape_info_pt *info){
	*info = malloc(sizeof(**info));
	(*info)->shape = shape;
	(*info)->name = name;
	(*info)->button = button;
	return CELIX_SUCCESS;
}
static celix_status_t paintFrame_show(paint_frame_pt frame) {
	gtk_widget_show(frame->drawingArea);
	gtk_widget_show(frame->toolbar);
	gtk_widget_show(frame->window);

	return CELIX_SUCCESS;
}

/**
 * Injects an available <tt>SimpleShape</tt> into the drawing frame.
 *
 * @param name The name of the injected <tt>SimpleShape</tt>.
 * @param icon The icon associated with the injected <tt>SimpleShape</tt>.
 * @param shape The injected <tt>SimpleShape</tt> instance.
 **/
celix_status_t paintFrame_addShape(paint_frame_pt frame, bundle_context_pt context, simple_shape_pt shape) {
	celix_status_t status = CELIX_SUCCESS;
	GError *gerror = NULL;
	GtkWidget *button = NULL;
	GtkWidget *im = NULL;

	gdk_threads_enter();
	
	button = gtk_button_new();
	gtk_widget_set_name((GtkWidget *) button, shape->name);
	im = gtk_image_new_from_file(shape->icon_path);
	gtk_button_set_image((GtkButton *) button, im);
	gtk_toolbar_append_widget((GtkToolbar *) frame->toolbar, (GtkWidget *) button, "", "");
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (paintFrame_buttonClicked), frame);
	gtk_widget_show(button);
	if (hashMap_get(frame->m_shapes, shape->name) == NULL) {
		shape_info_pt info = NULL;
		shapeInfo_create(frame, shape->name, button, shape, &info);
		hashMap_put(frame->m_shapes, shape->name, info);
	}
	paintFrame_redraw(frame, 0);
	paintFrame_show(frame);
	gdk_threads_leave();

	return status;
}

/**
 * Removes a no longer available <tt>SimpleShape</tt> from the drawing frame.
 *
 * @param name The name of the <tt>SimpleShape</tt> to remove.
 **/
celix_status_t paintFrame_removeShape(paint_frame_pt frame, simple_shape_pt sshape) {
	celix_status_t status = CELIX_SUCCESS;
	shape_info_pt shape = NULL;
	gdk_threads_enter();
	shape = (shape_info_pt) hashMap_remove(this->m_shapes, sshape->name);
	if (shape != NULL) {
		this->m_selected = NULL;
		gtk_widget_destroy(GTK_WIDGET(shape->button));
		gtk_widget_show_all(this->toolbar);
		paintFrame_redraw(this, 0);
		paintFrame_show(this);
	}
	gdk_threads_leave();

	return status;
}

/**
 * Retrieves the available <tt>SimpleShape</tt> associated with the given
 * name.
 *
 * @param name The name of the <tt>SimpleShape</tt> to retrieve.
 * @return The corresponding <tt>SimpleShape</tt> instance if available or
 *         <tt>null</tt>.
 **/
simple_shape_pt paintFrame_getShape(paint_frame_pt frame, char *name) {
	shape_info_pt info = (shape_info_pt) hashMap_get(frame->m_shapes, name);
	if (info == NULL) {
		return frame->m_defaultShape;
	} else {
		return info->shape;
	}
}

static void paintFrame_destroy(GtkWidget *widget, gpointer data) {
	paint_frame_pt frame = data;
	bundle_pt bundle = NULL;

	frame->showing = false;

//	bundleContext_getBundleById(frame->context, 0, &bundle);
//	bundle_stop(bundle, 0);
}

static void paintFrame_destroyWidgets(paint_frame_pt frame) {
	gdk_threads_enter();

	if (frame->pixMap != NULL) {
		gdk_pixmap_unref(frame->pixMap);
	}
	if (frame->toolbar != NULL) {
		gtk_widget_destroy(frame->toolbar);
	}
	if (frame->drawingArea != NULL) {
		gtk_widget_destroy(frame->drawingArea);
	}
	if (frame->window != NULL) {
		gtk_widget_destroy(frame->window);
	}

	frame->pixMap = NULL;
	frame->toolbar = NULL;
	frame->window = NULL;
	frame->drawingArea = NULL;

	gdk_threads_leave();
}

static void paintFrame_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	paint_frame_pt frame = data;
	GtkAllocation allocation;

	if (frame->pixMap != NULL) {
		gdk_pixmap_unref(frame->pixMap);
	}

	gtk_widget_get_allocation(widget, &allocation);
	frame->pixMap = gdk_pixmap_new(gtk_widget_get_window(widget), allocation.width, allocation.height, -1);
	paintFrame_redraw(frame, 0);
}

static void paintFrame_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
	paint_frame_pt frame = data;
	gdk_draw_pixmap(gtk_widget_get_window(widget),
			gtk_widget_get_style(widget)->fg_gc[gtk_widget_get_state(widget)],
			frame->pixMap, event->area.x, event->area.y, event->area.x,
			event->area.y, event->area.width, event->area.height);
}

static void paintFrame_buttonClicked(GtkWidget *button, gpointer data) {
	paint_frame_pt frame = data;
	frame->m_selected = (char *) gtk_widget_get_name(button);
}

static gboolean paintFrame_mousePressed( GtkWidget *widget, GdkEventButton *event, gpointer data) {
	paint_frame_pt frame = data;
	if (event->button == 1 && frame->pixMap != NULL) {
		if (frame->m_selected == NULL){
			printf("no button selected yet\n");
		} else {
			shape_component_pt sc = shapeComponent_create(frame, paintFrame_getShape(frame, frame->m_selected), event->x, event->y);
			linkedList_addFirst(frame->m_shapeComponents, sc);
			(*sc->shapeComponent_paintComponent)(sc, frame, frame->pixMap, widget);
		}
	}
	return TRUE;
}

static celix_status_t paintFrame_redraw(paint_frame_pt frame, GdkModifierType state) {
	if (frame->pixMap != NULL && frame->showing) {
		GdkRectangle update_rect;
		GtkAllocation allocation;
		linked_list_iterator_pt it = NULL;

		update_rect.x = 0;
		update_rect.y = 0;
		gtk_widget_get_allocation(frame->drawingArea, &allocation);
		update_rect.width = allocation.width;
		update_rect.height = allocation.height;
		gdk_draw_rectangle (this->pixMap,
				gtk_widget_get_style(frame->drawingArea)->white_gc,
				TRUE,
				update_rect.x, update_rect.y,
				update_rect.width, update_rect.height);
		gtk_widget_draw(frame->drawingArea, &update_rect);
		it = linkedListIterator_create(this->m_shapeComponents, 0);
		while (linkedListIterator_hasNext(it)) {
			shape_component_pt sc = linkedListIterator_next(it);
			(*sc->shapeComponent_paintComponent)(sc, this, this->pixMap, frame->drawingArea);
		}
	}

	return CELIX_SUCCESS;
}

static gpointer paintFrame_gtkmain(gpointer a_data) {
	GtkBuilder *builder;
	paint_frame_pt frame = (paint_frame_pt) a_data;

	gdk_threads_enter();
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, frame->file, NULL);

	frame->window = GTK_WIDGET(gtk_builder_get_object (builder, "window1"));
	frame->toolbar = GTK_WIDGET(gtk_builder_get_object (builder, "toolbar1"));
	frame->drawingArea = GTK_WIDGET(gtk_builder_get_object (builder, "drawingarea1"));
	g_object_unref(G_OBJECT(builder));

	gtk_window_set_title(GTK_WINDOW(frame->window), "OSGi in Action, Paint-Example");

	gtk_widget_set_events (frame->drawingArea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

	g_signal_connect(G_OBJECT(frame->window), "destroy", G_CALLBACK(paintFrame_destroy), frame);
	g_signal_connect(G_OBJECT(frame->drawingArea), "expose_event", G_CALLBACK(paintFrame_expose), frame);
	g_signal_connect(G_OBJECT(frame->drawingArea), "configure_event", G_CALLBACK(paintFrame_configure), frame);
	g_signal_connect(G_OBJECT(frame->drawingArea), "button_press_event", G_CALLBACK(paintFrame_mousePressed), frame);


	paintFrame_show(frame);
	frame->showing = true;

	gtk_main();
	gdk_threads_leave();

	return NULL;
}
