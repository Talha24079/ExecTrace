#!/bin/bash
set -e

echo "========================================="
echo "    ExecTrace Server Starting"
echo "========================================="
echo ""
echo "🚀 Server running on port 8080 (internal)"
echo "📍 Access URL: http://localhost:9090"
echo ""
echo "💾 Database location: /app/backend/data"
echo "📊 Working directory: $(pwd)"
echo ""
echo "========================================="
echo ""

# Execute the server
exec "$@"
