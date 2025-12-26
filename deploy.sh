#!/bin/bash
# ExecTrace Quick Deploy Script
# Usage: ./deploy.sh [port]

set -e

PORT=${1:-9090}

echo "=== ExecTrace Cloud Deployment ==="
echo "Port: $PORT"
echo ""

# Stop existing containers
echo "[1/4] Stopping existing containers..."
docker-compose down 2>/dev/null || true

# Build fresh image
echo "[2/4] Building Docker image..."
docker-compose build

# Start services
echo "[3/4] Starting services..."
docker-compose up -d

# Wait for health check
echo "[4/4] Waiting for server to be ready..."
sleep 3

# Test health endpoint
if curl -s http://localhost:$PORT/health > /dev/null; then
    echo ""
    echo "✅ ExecTrace is running!"
    echo ""
    echo "Access your instance at:"
    echo "  - Login: http://localhost:$PORT/login"
    echo "  - Workspace: http://localhost:$PORT/workspace"
    echo ""
    echo "Logs: docker logs -f exectrace-server"
else
    echo ""
    echo "❌ Health check failed. Check logs:"
    echo "docker logs exectrace-server"
    exit 1
fi
