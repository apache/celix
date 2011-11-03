/*
 * paint_frame.h
 *
 *  Created on: Aug 22, 2011
 *      Author: operator
 */

#ifndef PAINT_FRAME_H_
#define PAINT_FRAME_H_

#define PAINT_FRAME_SERVICE_NAME "paint"

struct paint_frame {
	apr_pool_t *pool;
	GtkWidget *window;
	GtkWidget *drawingArea;
	GtkWidget *toolbar;
	GdkPixmap *pixMap;
	bool showing;

	char *m_selected;
	HASH_MAP m_shapes;
	SIMPLE_SHAPE m_defaultShape;
	LINKED_LIST m_shapeComponents;
	BUNDLE_CONTEXT context;
	GThread *main;
};


typedef struct paint_frame *PAINT_FRAME;
celix_status_t paintFrame_create(BUNDLE_CONTEXT context, apr_pool_t *pool, PAINT_FRAME *frame);
celix_status_t paintFrame_exit(PAINT_FRAME frame);

SIMPLE_SHAPE paintFrame_getShape(PAINT_FRAME frame, char *name);
celix_status_t paintFrame_addShape(PAINT_FRAME frame, BUNDLE_CONTEXT context, SIMPLE_SHAPE shape);
celix_status_t paintFrame_removeShape(PAINT_FRAME frame, SIMPLE_SHAPE sshape);


#endif /* PAINT_FRAME_H_ */
