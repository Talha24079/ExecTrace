# ExecTrace Cloud Deployment Guide

## Quick Start (One Command)

```bash
./deploy.sh
```

That's it! ExecTrace will be running on `http://localhost:9090`

---

## Manual Deployment

### 1. Clone Repository
```bash
git clone <your-repo-url>
cd ExecTrace
```

### 2. Deploy with Docker
```bash
docker-compose up -d --build
```

### 3. Access Application
- **Login**: `http://your-server-ip:9090/login`
- **Workspace**: `http://your-server-ip:9090/workspace`
- **Dashboard**: `http://your-server-ip:9090/dashboard?key=<api-key>`

---

## Cloud Platform Deployment

### AWS EC2 / DigitalOcean / Linode

1. **SSH into your server**:
   ```bash
   ssh user@your-server-ip
   ```

2. **Install Docker** (if not installed):
   ```bash
   curl -fsSL https://get.docker.com -o get-docker.sh
   sh get-docker.sh
   sudo usermod -aG docker $USER
   ```

3. **Install Docker Compose**:
   ```bash
   sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
   sudo chmod +x /usr/local/bin/docker-compose
   ```

4. **Clone and Deploy**:
   ```bash
   git clone <your-repo>
   cd ExecTrace
   ./deploy.sh
   ```

5. **Configure Firewall**:
   ```bash
   sudo ufw allow 9090/tcp
   sudo ufw enable
   ```

---

### Google Cloud Run / Azure Container Instances

1. **Build Image**:
   ```bash
   docker build -t exectrace:latest .
   ```

2. **Tag for Registry**:
   ```bash
   # GCP
   docker tag exectrace:latest gcr.io/YOUR_PROJECT/exectrace:latest
   
   # Azure
   docker tag exectrace:latest yourregistry.azurecr.io/exectrace:latest
   ```

3. **Push to Registry**:
   ```bash
   # GCP
   docker push gcr.io/YOUR_PROJECT/exectrace:latest
   
   # Azure
   docker push yourregistry.azurecr.io/exectrace:latest
   ```

4. **Deploy**:
   ```bash
   # GCP Cloud Run
   gcloud run deploy exectrace --image gcr.io/YOUR_PROJECT/exectrace:latest --port 8080 --allow-unauthenticated
   
   # Azure Container Instances
   az container create --resource-group myResourceGroup --name exectrace --image yourregistry.azurecr.io/exectrace:latest --ports 8080
   ```

---

## Environment Variables (Optional)

No environment variables required! ExecTrace works out of the box.

**Optional customizations** in `docker-compose.yml`:
```yaml
environment:
  - PORT=8080  # Internal port (default: 8080)
```

---

## Data Persistence

Database files are automatically persisted in `./backend/data/`:
- `users.db` - User accounts
- `projects.db` - Projects and API keys
- `traces.db` - Performance traces

**Backup your data**:
```bash
tar -czf exectrace-backup-$(date +%Y%m%d).tar.gz backend/data/
```

---

## Health Check

```bash
curl http://localhost:9090/health
# Response: {"status":"ok","database":"initialized"}
```

---

## Troubleshooting

### Container won't start
```bash
docker logs exectrace-server
```

### Port already in use
```bash
# Change port in docker-compose.yml
ports:
  - "8080:8080"  # Change 9090 to any available port
```

### Database permission errors
```bash
sudo chown -R $USER:$USER backend/data/
```

### Reset everything
```bash
docker-compose down
rm -rf backend/data/*
docker-compose up -d --build
```

---

## Production Checklist

- ✅ Data persistence enabled (volume mount)
- ✅ Per-project isolation working
- ✅ Multi-user support functional
- ✅ Search and filtering available
- ✅ Dashboard sorting operational
- ✅ One-command deployment
- ✅ Health check endpoint
- ✅ Cross-platform (Windows/Linux SDK)

---

## Scaling

**Current setup**: Single container (suitable for small teams)

**For high traffic**:
- Use load balancer (nginx/HAProxy)
- Migrate to PostgreSQL (replace SQLite)
- Add Redis for session management
- Horizontal scaling with Kubernetes

---

## Support

For issues, check:
1. `docker logs exectrace-server`
2. Health endpoint: `curl http://localhost:9090/health`
3. Database files exist: `ls -la backend/data/`
