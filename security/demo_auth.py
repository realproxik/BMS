import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from security.auth_system import AuthStore


def main() -> None:
    store = AuthStore("bms_auth.json")
    print("Example signup:")
    print(store.signup("demo", "password123"))
    print("Example login:")
    print(store.login("demo", "password123"))


if __name__ == "__main__":
    main()
