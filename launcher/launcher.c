
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "framework.h"
#include "properties.h"
#include "headers.h"
#include "bundle_context.h"
#include "bundle.h"

static void launcher_load_custom_bundles(void);
void shutdown(int signal);

int running = 0;

#include <stdio.h>

FRAMEWORK framework;

int main(void) {
	// Set signal handler
	(void) signal(SIGINT, shutdown);
    HASHTABLE config = loadProperties("config.properties");
    char * autoStart = getProperty(config, "cosgi.auto.start.1");
    framework = framework_create();
    fw_init(framework);

    // Start the system bundle
    framework_start(framework);

    char delims[] = " ";
    char * result;
    LINKED_LIST bundles = linkedList_create();
    result = strtok(autoStart, delims);
    while (result != NULL) {
    	char * location = strdup(result);
    	linkedList_addElement(bundles, location);
    	result = strtok(NULL, delims);
    }
    int i;
    // Update according to Felix Main
    // First install all bundles
    // Afterwards start them
    BUNDLE_CONTEXT context = bundle_getContext(framework->bundle);
    for (i = 0; i < linkedList_size(bundles); i++) {
    	char * location = (char *) linkedList_get(bundles, i);
    	BUNDLE current = bundleContext_installBundle(context, location);
		startBundle(current, 0);
    }

    framework_waitForStop(framework);
    return 0;
}

void shutdown(int signal) {
	framework_stop(framework);
	framework_waitForStop(framework);
}
