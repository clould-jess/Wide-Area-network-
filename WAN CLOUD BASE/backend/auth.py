from datetime import datetime, timedelta
from jose import jwt, JWTError
from passlib.context import CryptContext
from fastapi import Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer
from sqlalchemy.orm import Session
from settings import settings
from db import get_db
from models import User

pwd = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2 = OAuth2PasswordBearer(tokenUrl="/auth/login")

def hash_pw(p: str) -> str:
    p = (p or "").strip()
    if len(p.encode("utf-8")) > 72:
        # bcrypt limit: 72 bytes
        raise ValueError("Password too long (max 72 bytes)")
    return pwd.hash(p)

def verify_pw(p: str, h: str) -> bool:
    p = (p or "").strip()
    if len(p.encode("utf-8")) > 72:
        return False
    return pwd.verify(p, h)

def create_token(user: User) -> str:
    exp = datetime.utcnow() + timedelta(minutes=settings.JWT_EXPIRE_MINUTES)
    payload = {"sub": str(user.id), "role": user.role, "exp": exp}
    return jwt.encode(payload, settings.JWT_SECRET, algorithm="HS256")

def get_current_user(db: Session = Depends(get_db), token: str = Depends(oauth2)) -> User:
    try:
        payload = jwt.decode(token, settings.JWT_SECRET, algorithms=["HS256"])
        user_id = int(payload.get("sub", "0"))
    except (JWTError, ValueError):
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid token")

    user = db.get(User, user_id)
    if not user or not user.is_active:
        raise HTTPException(status_code=401, detail="User not found or inactive")
    return user

def require_role(*roles: str):
    def _guard(user: User = Depends(get_current_user)):
        if user.role not in roles:
            raise HTTPException(status_code=403, detail="Forbidden")
        return user
    return _guard
