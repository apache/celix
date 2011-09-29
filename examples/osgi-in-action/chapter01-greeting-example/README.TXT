The example activates the bundles `chapter01-greeting-example` and  `chapter01-greeting-example-client`.
The activator of the client bundle (client.c) tries to find the greeting service, but may fail at start-up,
because it is activated before the greeting service is installed.

When the text 'Greetings' does not appear, stop and start the `chapter01-greeting-example-client` bundle.
Now the `chapter01-greeting-example` is already installed and the text `Greetings` does appear after the 
restart of the client bundle.
