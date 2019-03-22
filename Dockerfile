FROM ubuntu:18.04

# install dependencies
RUN mkdir /agentsim-dev && \
	mkdir /agentsim-dev/build && \
	mkdir /agentsim-dev/agentsim && \
	apt-get update && apt-get -y \
	install curl \
	cmake build-essential git \
	libhdf5-dev \
	libblas-dev \
	liblapack-dev


# copy agent sim project
COPY . /agentsim-dev/agentsim

# build agentsim project
RUN cd /agentsim-dev/build && \
	cmake ../agentsim -DCMAKE_BUILD_TYPE=Release && \
	make

#expose port 9002 for server
EXPOSE 9002

#move the server to the root dir
RUN cp /agentsim-dev/build/agentsim_server.exe /usr/bin/agentsim_server.exe
RUN cp -r /agentsim-dev/build/. /
