/*
 * XmlParser.c
 *
 *  Created on: Jun 27, 2012
 *      Author: alexander
 */
#include <stdbool.h>

#include <apr_general.h>

#include <celix_errno.h>

#include <libxml/xmlreader.h>

#include "xml_parser_impl.h"
#include "service_metadata.h"

struct xml_parser {
	apr_pool_t *pool;
};

celix_status_t xmlParser_create(apr_pool_t *pool, xml_parser_t *parser) {
	*parser = apr_palloc(pool, sizeof(**parser));
	(*parser)->pool = pool;
	return CELIX_SUCCESS;
}

celix_status_t xmlParser_parseComponent(xml_parser_t parser, char *componentEntry, component_t *component) {
	celix_status_t status = CELIX_SUCCESS;

	xmlTextReaderPtr reader = xmlReaderForFile(componentEntry, NULL, 0);
	if (reader != NULL) {
		bool pendingProperty = false;
		component_t currentComponent;
		service_t currentService;
		int read = xmlTextReaderRead(reader);
		while (read == 1) {
			int type = xmlTextReaderNodeType(reader);
			if (type == 1) {
				pendingProperty = false;
				const char *localname = xmlTextReaderConstLocalName(reader);
				if (strcmp(localname, "component") == 0) {
					printf("Component:\n");

					component_create(parser->pool, &currentComponent);

					char *name = xmlTextReaderGetAttribute(reader, "name");
					char *enabled = xmlTextReaderGetAttribute(reader, "enabled");
					char *immediate = xmlTextReaderGetAttribute(reader, "immediate");
					char *factory = xmlTextReaderGetAttribute(reader, "factory");
					char *configurationPolicy = xmlTextReaderGetAttribute(reader, "configuration-policy");
					char *activate = xmlTextReaderGetAttribute(reader, "activate");
					char *deactivate = xmlTextReaderGetAttribute(reader, "deactivate");
					char *modified = xmlTextReaderGetAttribute(reader, "modified");
					if (name != NULL) {
						printf("\tName: %s\n", name);
						component_setName(currentComponent, name);
					}
					if (enabled != NULL) {
						printf("\tEnabled: %s\n", enabled);
						component_setEnabled(currentComponent, strcmp("true", enabled) == 0);
					}
					if (immediate != NULL) {
						printf("\tImmediate: %s\n", immediate);
						component_setEnabled(currentComponent, strcmp("true", enabled) == 0);
					}
					if (factory != NULL) {
						printf("\tFactory: %s\n", factory);
						component_setFactoryIdentifier(currentComponent, factory);
					}
					if (configurationPolicy != NULL) {
						printf("\tConfiguration policy: %s\n", configurationPolicy);
						component_setConfigurationPolicy(currentComponent, configurationPolicy);
					}
					if (activate != NULL) {
						printf("\tActivate: %s\n", activate);
						component_setActivate(currentComponent, activate);
					}
					if (deactivate != NULL) {
						printf("\tDeactivate: %s\n", deactivate);
						component_setDeactivate(currentComponent, deactivate);
					}
					if (modified != NULL) {
						printf("\tModified: %s\n", modified);
						component_setModified(currentComponent, modified);
					}

					// components.add(currentComponent);

				} else if (strcmp(localname, "implementation") == 0) {
					printf("Implementation:\n");
					char *class = xmlTextReaderGetAttribute(reader, "class");
					printf("\tClass: %s\n", class);
					component_setImplementationClassName(currentComponent, class);
				} else if (strcmp(localname, "property") == 0) {
					printf("Property:\n");
					char *name = xmlTextReaderGetAttribute(reader, "name");
					char *value = xmlTextReaderGetAttribute(reader, "value");
					printf("\tName: %s\n", name);
					if (value != NULL) {
						printf("\tValue: %s\n", value);
					} else {
						pendingProperty = true;
					}
				} else if (strcmp(localname, "properties") == 0) {
					printf("Properties:\n");
				} else if (strcmp(localname, "service") == 0) {
					printf("Service:\n");

					service_create(parser->pool, &currentService);

					char *serviceFactory = xmlTextReaderGetAttribute(reader, "servicefactory");
					if (serviceFactory != NULL) {
						printf("\tService factory: %s\n", serviceFactory);
						serviceMetadata_setServiceFactory(currentService, strcmp("true", serviceFactory) == 0);
					}

					componentMetadata_setService(currentComponent, currentService);
				} else if (strcmp(localname, "provide") == 0) {
					printf("Provide:\n");
					char *interface = xmlTextReaderGetAttribute(reader, "interface");
					printf("\tInterface: %s\n", interface);
					serviceMetadata_addProvide(currentService, interface);
				} else if (strcmp(localname, "reference") == 0) {
					printf("Reference:\n");
					reference_metadata_t reference;
					referenceMetadata_create(parser->pool, &reference);

					char *name = xmlTextReaderGetAttribute(reader, "name");
					char *interface = xmlTextReaderGetAttribute(reader, "interface");
					char *cardinality = xmlTextReaderGetAttribute(reader, "cardinality");
					char *policy = xmlTextReaderGetAttribute(reader, "policy");
					char *target = xmlTextReaderGetAttribute(reader, "target");
					char *bind = xmlTextReaderGetAttribute(reader, "bind");
					char *updated = xmlTextReaderGetAttribute(reader, "updated");
					char *unbind = xmlTextReaderGetAttribute(reader, "unbind");

					if (name != NULL) {
						printf("\tName: %s\n", name);
						referenceMetadata_setName(reference, name);
					}
					printf("\tInterface: %s\n", interface);
					referenceMetadata_setInterface(reference, interface);
					if (cardinality != NULL) {
						printf("\tCardinality: %s\n", cardinality);
						referenceMetadata_setCardinality(reference, cardinality);
					}
					if (policy != NULL) {
						printf("\tPolicy: %s\n", policy);
						referenceMetadata_setPolicy(reference, policy);
					}
					if (target != NULL) {
						printf("\tCardinality: %s\n", target);
						referenceMetadata_setCardinality(reference, cardinality);
					}
					if (bind != NULL) {
						printf("\tBind: %s\n", bind);
						referenceMetadata_setBind(reference, bind);
					}
					if (updated != NULL) {
						printf("\tUpdated: %s\n", updated);
						referenceMetadata_setUpdated(reference, updated);
					}
					if (unbind != NULL) {
						printf("\tUnbind: %s\n", unbind);
						referenceMetadata_setUnbind(reference, unbind);
					}

					componentMetadata_addDependency(currentComponent, reference);
				} else {
					printf("Unsupported element:\n");
				}
			} else if (type == 3) {
				if (pendingProperty) {
					const char *value = xmlTextReaderConstValue(reader);
					if (value != NULL) {
						printf("\tValue text: %s\n", value);
						pendingProperty = false;
					}
				}
			}

			read = xmlTextReaderRead(reader);
		}
		if (currentComponent != NULL) {
			reference_metadata_t *refs;
			int size;
			componentMetadata_getDependencies(currentComponent, &refs, &size);
			int i;
			for (i = 0; i < size; i++) {
				reference_metadata_t ref = refs[i];
				char *in = NULL;
				referenceMetadata_getInterface(ref, &in);
				printf("AS: %s\n", in);
			}
			printf("ASD\n");
		}
	}

	return CELIX_SUCCESS;
}
