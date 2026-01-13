# Cloud Maintenance Manager (CMM)

## 📌 Project Overview
Cloud Maintenance Manager (CMM) is a cloud-based IT maintenance monitoring and management system designed to address the challenges of traditional on-premise IT maintenance.  
The system provides centralized monitoring, proactive fault detection, and remote management of IT infrastructure using cloud computing principles.

This project demonstrates how cloud servers can significantly reduce maintenance costs, downtime, and operational complexity, particularly in environments with limited IT resources such as public and private institutions in Cameroon.

---

## 🎯 Objectives
- Reduce dependency on physical IT infrastructure
- Shift IT maintenance from reactive to preventive
- Enable centralized and remote system monitoring
- Demonstrate the benefits of cloud computing for IT maintenance
- Provide a simulation platform for academic and practical evaluation

---

## 🏗️ System Architecture
The system follows a cloud-native, containerized architecture composed of:

- Monitoring Agent (Windows)  
  Collects system metrics from monitored machines.
- Cloud Backend API (FastAPI)  
  Receives, stores, and analyzes metrics.
- PostgreSQL Database  
  Stores users, servers, metrics, and alerts.
- Web-Based GUI  
  Provides visualization and interaction.
- Docker Containers  
  Ensure consistent deployment and scalability.

---

## ⚙️ Main Functionalities

### 1️⃣ User Authentication & Access Control
- Secure user registration and login
- Password hashing using bcrypt
- Role-based access control (admin, technician, viewer)
- JWT-based authentication

---

### 2️⃣ Server Registration & Asset Management
- Register IT assets (servers, machines)
- Store server metadata:
  - Server ID
  - Name
  - IP address
  - Owner
  - Environment type (cloud / on-premise)

---

### 3️⃣ Monitoring Agent
- Runs on Windows machines
- Collects system metrics:
  - CPU usage
  - RAM usage
  - Disk usage
  - System uptime
- Sends data periodically to the cloud backend

---

### 4️⃣ Centralized Metrics Collection
- Receives metrics from multiple agents
- Stores historical data in PostgreSQL
- Enables long-term monitoring and diagnostics

---

### 5️⃣ Alert & Incident Detection
- Automatic detection of abnormal conditions:
  - High CPU usage
  - High RAM usage
  - Low disk space
- Generates alerts with severity levels:
  - Info
  - Warning
  - Critical

---

### 6️⃣ Preventive vs Reactive Maintenance Simulation
- Simulate failures (CPU stress, downtime)
- Observe alert generation and system response
- Compare preventive maintenance outcomes with reactive maintenance

---

### 7️⃣ Web-Based Graphical User Interface (GUI)
- Accessible via browser
- Displays:
  - Registered servers
  - Latest system metrics
  - Alerts and incidents
- No local software installation required

---

### 8️⃣ API-Driven Design
- RESTful API endpoints for:
  - Authentication
  - Server management
  - Metrics ingestion
  - Alerts retrieval
- Easy integration with external systems

---

### 9️⃣ Containerized Deployment (Docker)
- Backend and database run in Docker containers
- Simplifies deployment and maintenance
- Reflects real cloud infrastructure practices

---

## 🧪 Simulation Capabilities
The system supports controlled simulations to:
- Demonstrate system failures
- Measure downtime and alert frequency
- Validate the effectiveness of cloud-based preventive maintenance

These simulations provide experimental evidence to support academic analysis.

---

## 🛠️ Technologies Used
- Backend: Python, FastAPI
- Database: PostgreSQL
- Security: bcrypt, JWT
- Frontend: HTML, Bootstrap (Web GUI)
- Agent: Python (psutil)
- Deployment: Docker, Docker Compose

---

## 🚀 Installation & Deployment

### Prerequisites
- Docker Desktop (Windows)
- WSL 2 enabled
- Python 3.x (for agent)

### Start the System
`bash
docker compose up --build
