FROM ubuntu:19.04

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

# build agentsim project
RUN cd /agentsim-dev/build && \
	cmake ../agentsim -DCMAKE_BUILD_TYPE=Release && \
	make

#expose port 9002 for server
EXPOSE 9002

#move the server to the root dir
RUN cp /agentsim-dev/build/agentsim_server.exe /usr/bin/agentsim_server.exe
RUN cp -r /agentsim-dev/build/bin/. /bin/
RUN cp -r /agentsim-dev/build/lib/. /lib/
RUN cp -r /agentsim-dev/build/src/. /src/
RUN cp -r /agentsim-dev/build/data/. /data/
RUN cp -r /agentsim-dev/build/readdy/. /readdy/
RUN cp -r /agentsim-dev/build/dep/. /dep/
RUN cp -r /agentsim-dev/build/CMakeFiles/. /CMakeFiles/
