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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
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
	hash_map_pt m_shapes;
	simple_shape_pt m_defaultShape;
	linked_list_pt m_shapeComponents;
	bundle_context_pt context;
	GThread *main;
	char *file;
};


typedef struct paint_frame *paint_frame_pt;
celix_status_t paintFrame_create(bundle_context_pt context, apr_pool_t *pool, paint_frame_pt *frame);
celix_status_t paintFrame_exit(paint_frame_pt frame);

simple_shape_pt paintFrame_getShape(paint_frame_pt frame, char *name);
celix_status_t paintFrame_addShape(paint_frame_pt frame, bundle_context_pt context, simple_shape_pt shape);
celix_status_t paintFrame_removeShape(paint_frame_pt frame, simple_shape_pt sshape);


#endif /* PAINT_FRAME_H_ */
