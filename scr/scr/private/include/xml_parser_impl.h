/*
 * xml_parser_impl.h
 *
 *  Created on: Jun 27, 2012
 *      Author: alexander
 */

#ifndef XML_PARSER_IMPL_H_
#define XML_PARSER_IMPL_H_

#include "component_metadata.h"

typedef struct xml_parser *xml_parser_t;

celix_status_t xmlParser_create(apr_pool_t *pool, xml_parser_t *parser);
celix_status_t xmlParser_parseComponent(xml_parser_t parser, char *componentEntry, component_t *component);

#endif /* XML_PARSER_IMPL_H_ */
