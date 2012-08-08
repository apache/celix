/*
 * simple_shape.h
 *
 *  Created on: Aug 22, 2011
 *      Author: operator
 */

#ifndef SIMPLE_SHAPE_H_
#define SIMPLE_SHAPE_H_

#include <gdk/gdk.h>

struct simple_shape {
	char *icon_path;
	char *name;
	void (*simpleShape_draw) (struct simple_shape *shape, GdkPixmap *pixMap, GtkWidget *widget, gdouble x, gdouble y);
};

typedef struct simple_shape *SIMPLE_SHAPE;

#define SIMPLE_SHAPE_SERVICE_NAME "simple_shape"


#endif /* SIMPLE_SHAPE_H_ */
