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
 * shape_component.c
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <celixbool.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "bundle_context.h"
#include "utils.h"
#include "linked_list.h"
#include "hash_map.h"
#include "simple_shape.h"
#include "shape_component.h"

static const int BOX = 54;

extern void shapeComponent_paintComponent(shape_component_pt shapeComponent, paint_frame_pt frame,
		GdkPixmap *pixMap, GtkWidget *widget);

shape_component_pt shapeComponent_create(paint_frame_pt frame, simple_shape_pt sshape,
		gdouble x, gdouble y) {
	shape_component_pt shape = malloc(sizeof(*shape));
	shape->m_frame = frame;
	shape->shapeName = strdup(sshape->name);
	shape->x = x - BOX /2;
	shape->y = y - BOX /2;
	shape->w = BOX;
	shape->h = BOX;
	/* methods */
	shape->shapeComponent_paintComponent = shapeComponent_paintComponent;
	return shape;
}

void shapeComponent_paintComponent(shape_component_pt shapeComponent, paint_frame_pt frame,
		GdkPixmap *pixMap, GtkWidget *widget) {
	simple_shape_pt shape = paintFrame_getShape(frame, shapeComponent->shapeName);
	if (shape == NULL) {
		g_printerr("cannot find shape %s\n", shapeComponent->shapeName);
	} else {
		(*shape->simpleShape_draw)(shape, pixMap, widget,
				shapeComponent->x, shapeComponent->y);
	}
}
