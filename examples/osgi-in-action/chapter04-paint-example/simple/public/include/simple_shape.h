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
 * simple_shape.h
 *
 *  \date       Aug 22, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
