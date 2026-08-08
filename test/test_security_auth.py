import os
import tempfile
import unittest

from security.auth_system import AuthStore


class AuthStoreTests(unittest.TestCase):
    def setUp(self):
        self.temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".json")
        self.temp_file.close()
        self.auth = AuthStore(self.temp_file.name)

    def tearDown(self):
        if os.path.exists(self.temp_file.name):
            os.unlink(self.temp_file.name)

    def test_signup_and_login(self):
        result = self.auth.signup("alice", "secret")
        self.assertTrue(result["success"])
        self.assertTrue(self.auth.login("alice", "secret"))
        self.assertFalse(self.auth.login("alice", "wrong-password"))

    def test_duplicate_signup_is_rejected(self):
        self.auth.signup("alice", "secret")
        result = self.auth.signup("alice", "secret")
        self.assertFalse(result["success"])
        self.assertIn("already exists", result["message"])


if __name__ == "__main__":
    unittest.main()
