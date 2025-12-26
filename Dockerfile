FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Crow dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY backend /app/backend
COPY frontend /app/frontend
COPY docker-entrypoint.sh /app/docker-entrypoint.sh

# Make entrypoint executable
RUN chmod +x /app/docker-entrypoint.sh

# Build server
RUN cd backend && \
    g++ -std=c++17 -DCROW_USE_BOOST \
    -I include/crow_include \
    src/server.cpp \
    -o et-server \
    -pthread -lboost_system

EXPOSE 8080
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./backend/et-server"]
