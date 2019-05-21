### Build image ###
FROM ubuntu:19.04 as build

# install dependencies
RUN mkdir /agentsim-dev && \
	mkdir /agentsim-dev/build && \
	mkdir /agentsim-dev/agentsim && \
	apt-get update && apt-get -y \
	install curl \
	cmake build-essential git \
	libhdf5-dev \
	libblas-dev \
	liblapack-dev \
	python-dev


# copy agent sim project
COPY . /agentsim-dev/agentsim
WORKDIR /agentsim-dev/agentsim
# install submodules
RUN git submodule update --init --recursive
# build agentsim project
RUN cd ../build && \
	cmake ../agentsim -DCMAKE_BUILD_TYPE=Release && \
	make

### Run image ###
FROM ubuntu:19.04

# install dependencies
RUN apt-get update && apt-get -y \
	install curl \
	libhdf5-dev \
	libblas-dev \
	liblapack-dev

# create non-root user
RUN groupadd -r app && useradd -r -g app app

# copy the server to the root dir
COPY --from=build --chown=app:app /agentsim-dev/build/agentsim_server.exe /usr/bin/agentsim_server.exe
COPY --from=build --chown=app:app /agentsim-dev/build/bin/. /bin/
COPY --from=build --chown=app:app /agentsim-dev/build/lib/. /lib/
COPY --from=build --chown=app:app /agentsim-dev/build/src/. /src/
COPY --from=build --chown=app:app /agentsim-dev/build/data/. /data/
COPY --from=build --chown=app:app /agentsim-dev/build/readdy/. /readdy/
COPY --from=build --chown=app:app /agentsim-dev/build/dep/. /dep/
COPY --from=build --chown=app:app /agentsim-dev/build/CMakeFiles/. /CMakeFiles/

USER app

#expose port 9002 for server
EXPOSE 9002

CMD ["agentsim_server.exe"]
