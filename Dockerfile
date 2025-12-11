# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the source code
COPY . /app

# Create data directory (optional, but good practice to ensure ownership)
RUN mkdir -p /app/data

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. && \
    make

# Expose port 8080
EXPOSE 8080

# Run the server
# Pass "../data" to use the mounted volume for persistence
CMD ["./build/et-server", "../data"]
