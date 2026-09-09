import importlib.util
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("check_repo", ROOT / "tools/check_repo.py")
CHECK_REPO = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(CHECK_REPO)


class ProjectContractTest(unittest.TestCase):
    def test_repository_contract(self):
        self.assertEqual(CHECK_REPO.main(), 0)


if __name__ == "__main__":
    unittest.main()
