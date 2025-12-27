#!/bin/bash

# ExecTrace One-Command Deployment Script

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== ExecTrace Deployment ===${NC}"

# Check for Docker
if ! command -v docker &> /dev/null; then
    echo "Docker could not be found. Please install Docker first."
    exit 1
fi

# Check for Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo "docker-compose could not be found. Please install Docker Compose first."
    exit 1
fi

echo -e "${GREEN}==> Stopping existing containers...${NC}"
docker-compose down

echo -e "${GREEN}==> Building and starting...${NC}"
docker-compose up -d --build

echo -e "${GREEN}==> Waiting for health check...${NC}"
sleep 5
if curl -s http://localhost:9090/health > /dev/null; then
    echo -e "${GREEN}✅ Deployment Successful!${NC}"
    echo "Dashboard available at: http://localhost:9090"
else
    echo "⚠️  Server started but health check failed. Check logs:"
    echo "docker logs exectrace-server"
fi
