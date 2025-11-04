#!/bin/bash
set -euo pipefail

# Configuration
EDGE_HOST="${EDGE_HOST:-edge-device}"
EDGE_USER="${EDGE_USER:-admin}"
DEPLOYMENT_DIR="/opt/vehicle/edge"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "🚀 Starting vehicle edge deployment..."

# Check if SSH connection is possible
if ! ssh -q "${EDGE_USER}@${EDGE_HOST}" exit; then
    echo -e "${RED}Error: Cannot connect to edge device${NC}"
    exit 1
fi

# Create deployment directory if it doesn't exist
ssh "${EDGE_USER}@${EDGE_HOST}" "sudo mkdir -p ${DEPLOYMENT_DIR}"

# Copy deployment files
echo "📦 Copying deployment files..."
rsync -avz --progress ../edge/ "${EDGE_USER}@${EDGE_HOST}:${DEPLOYMENT_DIR}"

# Apply Docker compose if exists
if [ -f "../edge/docker-compose.yml" ]; then
    echo "🐳 Deploying Docker services..."
    scp ../edge/docker-compose.yml "${EDGE_USER}@${EDGE_HOST}:${DEPLOYMENT_DIR}"
    ssh "${EDGE_USER}@${EDGE_HOST}" "cd ${DEPLOYMENT_DIR} && docker compose pull && docker compose up -d"
fi

echo -e "${GREEN}✅ Edge deployment completed successfully!${NC}"