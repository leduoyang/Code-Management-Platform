FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y gdb && \
    apt-get install -y net-tools && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

COPY . .

CMD ["tail", "-f", "/dev/null"]