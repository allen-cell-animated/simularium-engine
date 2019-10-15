# Agent Viz

## Description
Agent-Viz is a spatial simulation framework and visualization environment for biological simulations.

### Packages
Agent-Viz integrates existing spatial simulation software:

* [ReaDDy](https://readdy.github.io/) : Molecular Dynamics
* [Cytosim](https://gitlab.com/f.nedelec/cytosim): Cytoskeletal Dynamics

## Dependencies
* blosc
* hdf5
* blas
* lapack
* openssl

On **Mac**, install [homebrew](https://brew.sh/) and run:
`brew install cmake hdf5 openssl`

On **Arch Linux**, run:
`sudo pacman -S cmake hdf5 blosc blas lapack openssl`

On **Ubuntu 19.04**, run:
`apt-get update && apt-get install -y
build-essential cmake curl git libblas-dev libhdf5-dev liblapack-dev
python-dev libssl-dev libcurl4-openssl-dev libblosc1`

## Building
### TLS Setup
1. Install [makecrt](https://github.com/FiloSottile/mkcert)
2. generate a certificate for localhost
3. In a terminal window, run: `source [repo-path]/setup.sh`
4. From the same terminal window, run: `./agentsim_server.exe`

### Docker
1. Install [docker](https://docs.docker.com/v17.09/engine/installation/)
2. Clone the repository locally: `git clone --recursive *repository-address*`
3. To build the container, run: `sudo docker build -t agentsim-dev .`
4. To run the container, run: `sudo docker run -it -p 9002:9002 agentsim-dev:latest agentsim_server.exe --no-exit`

### Native
1. Clone the repository locally: `git clone --recursive *repository-address*`
2. Create a new directory for the build: e.g. '~/Documents/build/agentviz'
3. Navigate to the new directory in a terminal and run:
	`sudo cmake [path to repository] -DBUILD_ONLY="s3;awstransfer;transfer" -DCMAKE_BUILD_TYPE=Release`
4. Run: `sudo chown -R $USER`
5. Run: `make`
6. To run the server, run: `./agentsim_server.exe --no-exit`
7. To run the tests, run: `./agentsim_tests`

## Tests
This project uses the [google test framework](https://github.com/google/googletest)
