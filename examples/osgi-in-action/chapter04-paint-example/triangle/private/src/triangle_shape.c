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
 * simple_shape.c
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "bundle_context.h"
#include "bundle.h"
#include "hash_map.h"
#include "simple_shape.h"
#include "triangle_shape.h"
#define TRIANGLE_FILE "triangle.png"

void triangleShape_draw(SIMPLE_SHAPE shape, GdkPixmap *pixMap, GtkWidget *widget, gdouble x, gdouble y);

SIMPLE_SHAPE triangleShape_create(BUNDLE_CONTEXT context) {
	BUNDLE bundle;
	apr_pool_t *pool;
	SIMPLE_SHAPE shape = (SIMPLE_SHAPE) malloc(sizeof(*shape));
	bundleContext_getBundle(context, &bundle);
	bundleContext_getMemoryPool(context, &pool);
	shape->name = "Triangle";
	shape->icon_path = NULL;
	celix_status_t status = bundle_getEntry(bundle, TRIANGLE_FILE, pool, &shape->icon_path);
	shape->simpleShape_draw = triangleShape_draw;
	if (status == CELIX_SUCCESS){
		// no error
	} else {
		printf("Could not find resource %s\n", TRIANGLE_FILE);
	}
	return shape;
}

void triangleShape_draw(SIMPLE_SHAPE shape, GdkPixmap *pixMap, GtkWidget *widget, gdouble x, gdouble y){
	GdkRectangle update_rect;
	GdkPixbuf *curr_pix_buf;
	GError *gerror = NULL;
	gsize rd = 0, wr = 0;
	if (shape->icon_path == NULL) {
		printf("error message: icon path unknown\n");
	} else {
	gchar *gfn = g_locale_to_utf8(shape->icon_path, strlen(shape->icon_path), &rd, &wr, &gerror);
	curr_pix_buf = gdk_pixbuf_new_from_file(gfn, &gerror);
	if(!curr_pix_buf) {
		g_printerr("error message: %s\n", (gchar *) gerror->message);
	}
	update_rect.x = x - 5;
	update_rect.y = y - 5;
	update_rect.width = gdk_pixbuf_get_width(curr_pix_buf);
	update_rect.height = gdk_pixbuf_get_height(curr_pix_buf);
	gdk_pixbuf_render_to_drawable(
			curr_pix_buf,
			pixMap,
			gtk_widget_get_style(widget)->fg_gc[gtk_widget_get_state(widget)],
			0, 0,
			update_rect.x, update_rect.y,
			update_rect.width,
			update_rect.height,
			GDK_RGB_DITHER_NONE,
			0, 0);
	gtk_widget_queue_draw_area (widget,
			update_rect.x, update_rect.y,
			update_rect.width, update_rect.height);
	}
}



