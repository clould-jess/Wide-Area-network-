from fastapi import FastAPI, Depends
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from pydantic import BaseModel, Field
from sqlalchemy.orm import Session
from datetime import datetime

from settings import settings
from db import engine, Base, get_db
from models import User, Server, Metric, Alert
from auth import hash_pw, verify_pw, create_token, require_role

Base.metadata.create_all(bind=engine)

app = FastAPI(title="Cloud Maintenance Manager (Docker + GUI)")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ---------- Schemas ----------
class RegisterUser(BaseModel):
    email: str
    password: str
    role: str = "admin"  # admin/tech/viewer

class LoginUser(BaseModel):
    email: str
    password: str

class ServerIn(BaseModel):
    server_id: str
    name: str
    ip: str
    environment: str = Field(..., examples=["cloud", "on-prem"])
    owner: str

class MetricIn(BaseModel):
    server_id: str
    timestamp: datetime
    cpu_percent: float
    ram_percent: float
    disk_percent: float
    uptime_seconds: int

# ---------- Helpers ----------
def make_alert(db: Session, server_id: str, level: str, message: str):
    db.add(Alert(server_id=server_id, level=level, message=message))
    db.commit()

# ---------- GUI ----------
@app.get("/gui", response_class=HTMLResponse)
def gui():
    return """
<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>Cloud Maintenance Manager GUI</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body { background:#0b1220; color:#e8eefc; }
    .card { background:#0f1a33; border:1px solid #23345c; }
    .muted { color:#9fb3dd; }
    .btn-glow { box-shadow: 0 0 18px rgba(76, 140, 255, .35); }
    pre { white-space: pre-wrap; word-break: break-word; }
  </style>
</head>
<body>
<div class="container py-4">
  <div class="d-flex justify-content-between align-items-center mb-3">
    <h3 class="m-0">Cloud Maintenance Manager</h3>
    <span class="muted">GUI Dashboard</span>
  </div>

  <div class="card p-3 mb-3">
    <div class="row g-2 align-items-end">
      <div class="col-md-4">
        <label class="form-label muted">Email</label>
        <input id="email" class="form-control" placeholder="admin@org.cm">
      </div>
      <div class="col-md-4">
        <label class="form-label muted">Password</label>
        <input id="password" type="password" class="form-control" placeholder="Admin123!">
      </div>
      <div class="col-md-4 d-grid">
        <button class="btn btn-primary btn-glow" onclick="login()">Login</button>
      </div>
    </div>
    <div class="mt-2 muted small">
      Token is stored in your browser (localStorage). Use Logout to clear it.
    </div>
    <div class="mt-2">
      <button class="btn btn-outline-light btn-sm" onclick="logout()">Logout</button>
      <span id="status" class="ms-2 muted"></span>
    </div>
  </div>

  <div class="row g-3">
    <div class="col-lg-6">
      <div class="card p-3">
        <div class="d-flex justify-content-between align-items-center">
          <h5 class="m-0">Servers</h5>
          <button class="btn btn-outline-info btn-sm" onclick="loadServers()">Refresh</button>
        </div>
        <pre id="servers" class="mt-2 mb-0 small"></pre>
      </div>
    </div>

    <div class="col-lg-6">
      <div class="card p-3">
        <div class="d-flex justify-content-between align-items-center">
          <h5 class="m-0">Alerts (last 200)</h5>
          <button class="btn btn-outline-warning btn-sm" onclick="loadAlerts()">Refresh</button>
        </div>
        <pre id="alerts" class="mt-2 mb-0 small"></pre>
      </div>
    </div>

    <div class="col-12">
      <div class="card p-3">
        <div class="row g-2 align-items-end">
          <div class="col-md-6">
            <label class="form-label muted">Latest metrics for server_id</label>
            <input id="metricServerId" class="form-control" placeholder="srv-001">
          </div>
          <div class="col-md-6 d-grid">
            <button class="btn btn-outline-success btn-sm" onclick="loadLatest()">Get Latest Metrics</button>
          </div>
        </div>
        <pre id="latest" class="mt-2 mb-0 small"></pre>
      </div>
    </div>
  </div>
</div>

<script>
const API = location.origin;
function getToken(){ return localStorage.getItem("cmm_token"); }
function setStatus(msg){ document.getElementById("status").textContent = msg; }

async function login(){
  const email = document.getElementById("email").value.trim();
  const password = document.getElementById("password").value;
  const r = await fetch(`${API}/auth/login`, {
    method:"POST",
    headers: {"Content-Type":"application/json"},
    body: JSON.stringify({email,password})
  });
  const data = await r.json();
  if(data.ok){
    localStorage.setItem("cmm_token", data.token);
    setStatus(`Logged in as ${data.role}`);
    await loadServers(); await loadAlerts();
  } else {
    setStatus("Login failed: " + (data.message || "unknown"));
  }
}

function logout(){
  localStorage.removeItem("cmm_token");
  setStatus("Logged out");
}

async function apiGet(path){
  const token = getToken();
  const r = await fetch(`${API}${path}`, { headers: { "Authorization": `Bearer ${token}` } });
  return r.json();
}

async function loadServers(){
  try{
    const data = await apiGet("/servers");
    document.getElementById("servers").textContent = JSON.stringify(data, null, 2);
  }catch(e){ document.getElementById("servers").textContent = "Error: " + e; }
}

async function loadAlerts(){
  try{
    const data = await apiGet("/alerts");
    document.getElementById("alerts").textContent = JSON.stringify(data, null, 2);
  }catch(e){ document.getElementById("alerts").textContent = "Error: " + e; }
}

async function loadLatest(){
  const id = document.getElementById("metricServerId").value.trim();
  if(!id){ document.getElementById("latest").textContent = "Enter server_id"; return; }
  try{
    const data = await apiGet(`/metrics/latest/${encodeURIComponent(id)}`);
    document.getElementById("latest").textContent = JSON.stringify(data, null, 2);
  }catch(e){ document.getElementById("latest").textContent = "Error: " + e; }
}

setStatus(getToken() ? "Token found (you can refresh data)" : "Not logged in");
</script>
</body>
</html>
"""

# ---------- Auth ----------
@app.post("/auth/register")
def register(payload: RegisterUser, db: Session = Depends(get_db)):
    if db.query(User).filter(User.email == payload.email).first():
        return {"ok": False, "message": "Email already exists"}
    try:
        pw_hash = hash_pw(payload.password)
    except ValueError as e:
        return {"ok": False, "message": str(e)}
    u = User(email=payload.email, password_hash=pw_hash, role=payload.role)
    db.add(u); db.commit()
    return {"ok": True, "message": "User created"}

@app.post("/auth/login")
def login(payload: LoginUser, db: Session = Depends(get_db)):
    u = db.query(User).filter(User.email == payload.email).first()
    if not u or not verify_pw(payload.password, u.password_hash):
        return {"ok": False, "message": "Invalid credentials"}
    return {"ok": True, "token": create_token(u), "role": u.role}

# ---------- Servers ----------
@app.post("/servers", dependencies=[Depends(require_role("admin", "tech"))])
def add_server(payload: ServerIn, db: Session = Depends(get_db)):
    if db.query(Server).filter(Server.server_id == payload.server_id).first():
        return {"ok": False, "message": "server_id already exists"}
    s = Server(**payload.model_dump())
    db.add(s); db.commit()
    return {"ok": True}

@app.get("/servers", dependencies=[Depends(require_role("admin", "tech", "viewer"))])
def list_servers(db: Session = Depends(get_db)):
    items = db.query(Server).order_by(Server.id.desc()).all()
    return [{"server_id": s.server_id, "name": s.name, "ip": s.ip, "environment": s.environment, "owner": s.owner} for s in items]

# ---------- Metrics ----------
@app.post("/metrics")
def ingest_metrics(payload: MetricIn, db: Session = Depends(get_db)):
    s = db.query(Server).filter(Server.server_id == payload.server_id).first()
    if not s:
        return {"ok": False, "message": "Unknown server_id (register server first)"}

    db.add(Metric(
        server_id_fk=s.id,
        timestamp=payload.timestamp,
        cpu_percent=payload.cpu_percent,
        ram_percent=payload.ram_percent,
        disk_percent=payload.disk_percent,
        uptime_seconds=payload.uptime_seconds
    ))
    db.commit()

    if payload.cpu_percent >= 85:
        make_alert(db, payload.server_id, "warning", f"High CPU: {payload.cpu_percent}%")
    if payload.ram_percent >= 90:
        make_alert(db, payload.server_id, "warning", f"High RAM: {payload.ram_percent}%")
    if payload.disk_percent >= 90:
        make_alert(db, payload.server_id, "critical", f"Disk nearly full: {payload.disk_percent}%")

    return {"ok": True}

@app.get("/metrics/latest/{server_id}", dependencies=[Depends(require_role("admin", "tech", "viewer"))])
def latest_metrics(server_id: str, db: Session = Depends(get_db)):
    s = db.query(Server).filter(Server.server_id == server_id).first()
    if not s:
        return {}
    m = db.query(Metric).filter(Metric.server_id_fk == s.id).order_by(Metric.timestamp.desc()).first()
    if not m:
        return {}
    return {
        "server_id": server_id,
        "timestamp": m.timestamp,
        "cpu_percent": m.cpu_percent,
        "ram_percent": m.ram_percent,
        "disk_percent": m.disk_percent,
        "uptime_seconds": m.uptime_seconds,
    }

# ---------- Alerts ----------
@app.get("/alerts", dependencies=[Depends(require_role("admin", "tech", "viewer"))])
def list_alerts(db: Session = Depends(get_db)):
    items = db.query(Alert).order_by(Alert.timestamp.desc()).limit(200).all()
    return [{"server_id": a.server_id, "level": a.level, "message": a.message, "timestamp": a.timestamp} for a in items]

@app.get("/")
def root():
    return {"status":"ok","message":"CMM API running. Open /docs or /gui"}
