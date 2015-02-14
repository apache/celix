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
 * shape_component.h
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHAPE_COMPONENT_H_
#define SHAPE_COMPONENT_H_

#include "paint_frame.h"

typedef struct shape_component *shape_component_pt;

struct shape_component {
	char *shapeName;
	paint_frame_pt m_frame;
	gdouble x, y, w, h;
	void (*shapeComponent_paintComponent)(shape_component_pt shapeComponent, paint_frame_pt frame,
			GdkPixmap *pixMap, GtkWidget *widget);
};

extern shape_component_pt shapeComponent_create(paint_frame_pt frame, simple_shape_pt sshape,
		gdouble x, gdouble y);

#endif /* SHAPE_COMPONENT_H_ */
