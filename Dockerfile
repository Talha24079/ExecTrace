FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend /app/backend
COPY frontend /app/frontend

RUN cd backend && bash build.sh

EXPOSE 8080

CMD ["./backend/et-server"]
