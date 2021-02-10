FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
		libboost-dev \
		autotools-dev \
		swig \
		python3 \
		python3-dev \
		libgdal-dev \
		pkg-config \
		gcc \
		openjdk-11-jdk \
		build-essential \
		libosmium2-dev \
		libgumbo-dev \
		libwebkit2gtk-4.0-dev \
		libopenmpi-dev \
		libboost-all-dev \
		librange-v3-dev \
		wget \
		unzip \
		libxml++2.6-dev && rm -rf /var/lib/apt/lists/*

RUN mkdir /libpqxx && \
	cd /libpqxx && \
	wget https://github.com/jtv/libpqxx/archive/7.3.1.zip && \
	unzip 7.3.1.zip && \
	cd libpqxx-7.3.1/ && \
	./configure && \
	make && \
	make install

COPY . /src
WORKDIR /src

ENV PKG_CONFIG_PATH="/src/m4/"

RUN rm -rf /libpqxx && \
	ln -s /lib/x86_64-linux-gnu/pkgconfig/python-3.8.pc /lib/x86_64-linux-gnu/pkgconfig/python.pc && \
	./autogen.sh && \
	mkdir build && \
	cd build && \
	../configure

WORKDIR /src/build


