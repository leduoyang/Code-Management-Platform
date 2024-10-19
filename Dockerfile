# Use Ubuntu 20.04 as the base image
FROM ubuntu:20.04

# Set environment variables to avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages (build-essential for C/C++ compilation)
RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y gdb && \
    apt-get install -y net-tools && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the source code (assuming the source files are in the same directory as the Dockerfile)
COPY . .

CMD ["tail", "-f", "/dev/null"]