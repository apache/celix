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
 * paint_frame.h
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
