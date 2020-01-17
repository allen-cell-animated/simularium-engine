### Build image ###
FROM ubuntu:19.04 as build

# install dependencies
RUN mkdir /agentsim-dev && \
	mkdir /agentsim-dev/build && \
	mkdir /agentsim-dev/lib && \
	mkdir /agentsim-dev/agentsim && \
	apt-get update && apt-get install -y \
	build-essential \
	cmake \
	curl \
	git \
    openssl \
	libblas-dev \
	libhdf5-dev \
	liblapack-dev \
	python-dev \
	libssl-dev libcurl4-openssl-dev \
	libblosc1 \
    libglew-dev mesa-common-dev freeglut3-dev

# copy agent sim project
COPY . /agentsim-dev/agentsim
WORKDIR /agentsim-dev/agentsim

# install submodules
RUN git submodule update --init --recursive

# build agentsim project
RUN cd ../build && \
	cmake ../agentsim -DBUILD_ONLY="s3;awstransfer;transfer" -DCMAKE_BUILD_TYPE=Release && \
	make && \
    openssl dhparam -out /dh.pem 2048 && \
	find /agentsim-dev/build | grep -i so$ | xargs -i cp {} /agentsim-dev/lib/

### Run image ###
FROM ubuntu:19.04
WORKDIR /

# install dependencies
RUN apt-get update && apt-get install -y \
	build-essential \
	curl \
	libblas-dev \
	libhdf5-dev \
	liblapack-dev \
	&& rm -rf /var/lib/apt/lists/*

# create non-root user
RUN groupadd -r app && useradd -r -g app app

# copy the server to the root dir
COPY --from=build --chown=app:app /agentsim-dev/build/agentsim_server.exe /usr/bin/agentsim_server.exe
COPY --from=build --chown=app:app /dh.pem /dh.pem
COPY --from=build --chown=app:app /agentsim-dev/build/bin/. /bin/
RUN echo " "
COPY --from=build --chown=app:app /agentsim-dev/lib/. /usr/lib/
RUN echo " "
COPY --from=build --chown=app:app /agentsim-dev/lib/. /usr/local/lib/
RUN echo " "
COPY --from=build --chown=app:app /usr/. /usr/
RUN echo " "

RUN mkdir /trajectory && chown -R app /trajectory
USER app

#expose port 9002 for server
EXPOSE 9002

CMD ["agentsim_server.exe"]
