/*
 * shape_component.c
 *
 *  Created on: Aug 22, 2011
 *      Author: operator
 */

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "bundle_context.h"
#include "utils.h"
#include "linkedlist.h"
#include "hash_map.h"
#include "simple_shape.h"
#include "paint_frame.h"
#include "shape_component.h"

extern void shapeComponent_paintComponent(SHAPE_COMPONENT shapeComponent, PAINT_FRAME frame,
		GdkPixmap *pixMap, GtkWidget *widget);

SHAPE_COMPONENT shapeComponent_create(PAINT_FRAME frame, char *shapeName,
		gdouble x, gdouble y) {
	SHAPE_COMPONENT shape = malloc(sizeof(*shape));
	shape->m_frame = frame;
	strcpy(shape->shapeName, shapeName);
	shape->x = x - BOX /2;
	shape->y = y - BOX /2;
	shape->w = BOX;
	shape->h = BOX;
	/* methods */
	shape->shapeComponent_paintComponent = shapeComponent_paintComponent;
	return shape;
}

void shapeComponent_paintComponent(SHAPE_COMPONENT shapeComponent, PAINT_FRAME frame,
		GdkPixmap *pixMap, GtkWidget *widget) {
	SIMPLE_SHAPE shape = frame->paintFrame_getShape(shapeComponent->shapeName);
	if (shape == NULL) {
		g_printerr("cannot find shape %s\n", shapeComponent->shapeName);
	} else {
		(*shape->simpleShape_draw)(shape, pixMap, widget,
				shapeComponent->x, shapeComponent->y);
	}
}
