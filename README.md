# Simularium

Simularium is a spatial simulation framework and visualization environment for biological simulations.

---

## Description

Simularium is a spatial simulation framework and visualization environment for biological simulations.

## Installation

Agent-Viz integrates existing spatial simulation software:

* [ReaDDy](https://readdy.github.io/) : Molecular Dynamics
* [Cytosim](https://gitlab.com/f.nedelec/cytosim): Cytoskeletal Dynamics

### Dependencies
* blosc
* hdf5
* blas
* lapack
* openssl

On **Mac**, install [homebrew](https://brew.sh/) and run:
`brew install cmake hdf5 openssl mkcert`

On **Arch Linux**, run:
`sudo pacman -S cmake hdf5 blosc blas lapack openssl mkcert`

On **Ubuntu 19.04**, install [mkcert](https://github.com/FiloSottile/mkcert) and run:
`apt-get update && apt-get install -y
build-essential cmake curl git libblas-dev libhdf5-dev liblapack-dev
python-dev libssl-dev libcurl4-openssl-dev libblosc1`

#### Docker
1. Install [docker](https://docs.docker.com/v17.09/engine/installation/)
2. Clone the repository locally: `git clone --recursive *repository-address*`
3. To build the container, run: `sudo docker build -t agentsim-dev .`
4. To run the container, run: `sudo docker run -it -p 9002:9002 agentsim-dev:latest agentsim_server.exe --no-exit`
5. Mount the tls certificate to /etc/ssl/tls.crt
6. Mount the tls key to /etc/ssl/tls.key

#### Native
1. Clone the repository locally: `git clone --recursive *repository-address*`
2. Create a new directory for the build: e.g. '~/Documents/build/agentviz'
3. Navigate to the new directory in a terminal and run:
	`sudo [path to repository]/local_build.sh`
4. From the same terminal window, run: `sudo chown -R $USER *`
5. From the same terminal window, run: `source [path to repository]/setup_env.sh`
6. To run the server, run: `./agentsim_server.exe --no-exit`
7. To rebuild/update after source changes, run `make` from the build directory

### Tests
This project uses the [google test framework](https://github.com/google/googletest)
To run the tests, navigate to the build directory and run: `./agentsim_tests`

### AWS Authentication
Agent-Viz stores various runtime files on Amazon Web Services (AWS) S3. The following steps are necessary to allow a local build to
upload to S3.

1. Install the [aws-cli](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-install.html)
2. [Configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-configure.html) the aws-cli using your [private key](https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_access-keys.html#Using_CreateAccessKey_CLIAPI); the region is **us-east-2**

Agent-Viz will use the credentials configured above to upload, download, and otherwise interact with other components of the application setup on AWS. The application should function normally without these credentials, but will be unable to upload files to the AWS S3 repository.
## Documentation

If you have more extensive technical documentation (whether generated or not), ensure they are published to the following address:
For full package documentation please visit
[organization.github.io/projectname](https://organization.github.io/projectname/index.html).

## Quick Start

Describe the most common operations, step by step, that a first time user would want to know, here.

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for information related to developing the code.
