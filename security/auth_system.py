import hashlib
import json
import os
from typing import Dict, Optional


class AuthStore:
    """A simple signup/login store using SHA-1 hashed passwords."""

    def __init__(self, storage_path: Optional[str] = None):
        self.storage_path = storage_path or os.path.join(os.getcwd(), "bms_auth.json")
        self._users: Dict[str, str] = {}
        self._load()

    def _load(self) -> None:
        if not os.path.exists(self.storage_path):
            return
        try:
            with open(self.storage_path, "r", encoding="utf-8") as handle:
                data = json.load(handle)
        except (json.JSONDecodeError, OSError):
            self._users = {}
            return
        if isinstance(data, dict):
            self._users = {str(k): str(v) for k, v in data.items()}
        else:
            self._users = {}

    def _save(self) -> None:
        with open(self.storage_path, "w", encoding="utf-8") as handle:
            json.dump(self._users, handle, indent=2)

    @staticmethod
    def _hash_password(password: str) -> str:
        return hashlib.sha1(password.encode("utf-8")).hexdigest()

    def signup(self, username: str, password: str) -> Dict[str, object]:
        username = username.strip()
        if not username or not password:
            return {"success": False, "message": "Username and password are required."}
        if username in self._users:
            return {"success": False, "message": f"User '{username}' already exists."}
        self._users[username] = self._hash_password(password)
        self._save()
        return {"success": True, "message": f"User '{username}' created successfully."}

    def login(self, username: str, password: str) -> bool:
        username = username.strip()
        if not username:
            return False
        stored_hash = self._users.get(username)
        if not stored_hash:
            return False
        return stored_hash == self._hash_password(password)
