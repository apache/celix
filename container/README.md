# Celix dev container usage

This subdirectory contains a [Gitpod Containerfile](./Containerfile.gitpod) for developing on Celix using [Gitpod](https://gitpod.io/#https://github.com/apache/celix)
and a [Ubuntu Containerfile](./Containerfile.ubuntu) for local development on Celix.

The below steps only need to be executed if you want to develop locally, and not via Gitpod.

## Build the celix-dev image yourself

To always be able to develop on Celix with an up-to-date image, built the image yourself.

```bash
./container/build-ubuntu-container.sh
```

## Using the image

### Start locally with SSH daemon

```bash
cd <project-root>

# Start a local container with the SSH daemon running
./container/run-ubuntu-container.sh
```

Now connect to the container via the remote container option of your favoured IDE and start building/developing.

For example, you can now start with:

* CLion connected to the container using [JetBrains Gateway](https://www.jetbrains.com/help/clion/remote-development-a.html)
* CLion configured to connect to the container as [Remote Host](https://www.jetbrains.com/help/clion/remote-projects-support.html)

When finished with development and testing, press `CTRL + \` to stop the SSH daemon and exit the container.

### Start locally with only a bash shell

The start script allows passing of additional parameters, which will override the starting of the SSH daemon.
Execute the following commands to open a bash shell and build Celix from the command line:

```bash
cd <project-root>

# Start a local container and open a bash shell
./container/run-ubuntu-container.sh bash

# Build Apache Celix
mkdir -p build
cd build
../container/support-scripts/build-all.sh
make -j

# Run the unit tests for Apache Celix
ctest --output-on-failure
```
