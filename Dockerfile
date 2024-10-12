# Use the official Debian base image
FROM debian:latest

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libjson-c-dev \
    libmosquitto-dev \
    libssl-dev \
    libcurl4-gnutls-dev \
    iputils-ping \
    bash \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy all files from the current directory to the /app directory in the container
COPY . /app

# Make the dev.sh script executable (if it's not already)
RUN chmod +x /app/dev.sh

# Set the entry point to run the dev.sh script
ENTRYPOINT ["/bin/bash", "/app/dev.sh"]


