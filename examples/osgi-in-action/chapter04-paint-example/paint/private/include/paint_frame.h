/*
 * paint_frame.h
 *
 *  Created on: Aug 22, 2011
 *      Author: operator
 */

#ifndef PAINT_FRAME_H_
#define PAINT_FRAME_H_

static const int BOX = 54;

#define PAINT_FRAME_SERVICE_NAME "paint"

struct paint_frame {
	GtkWidget *m_toolBar;
	char *m_selected;
	GtkWidget *m_mainPanel, *m_window, *m_drawingArea;
	GdkPixmap *m_pixMap;
	//SHAPE_COMPONENT *m_selectedComponent;
	HASH_MAP m_shapes;
	//private ActionListener m_reusableActionListener = new ShapeActionListener();
	SIMPLE_SHAPE m_defaultShape;
	LINKED_LIST m_shapeComponents;
	/* methods */
	void (*paintFrame_show)(void);
	void (*paintFrame_selectShape)(char *name);
	SIMPLE_SHAPE (*paintFrame_getShape)(char *name);
	void (*paintFrame_addShape)(BUNDLE_CONTEXT context, char* name, void *icon, SIMPLE_SHAPE shape);
	void (*paintFrame_removeShape)(char *name);
};


typedef struct paint_frame *PAINT_FRAME;
extern PAINT_FRAME paintFrame_create(BUNDLE_CONTEXT context, apr_pool_t *pool);




#endif /* PAINT_FRAME_H_ */
