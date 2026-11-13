"""Directly exercise the runner's STATUS counter checks without a game.

Positive counts stop a real run immediately, so a falling sequence cannot be
reached through the fake-game integration test. Seed the saved counter to
exercise that defensive branch explicitly.
"""
import ast
import pathlib
import types
import unittest


class FakeClient:
    def __enter__(self):
        return self

    def __exit__(self, *args):
        pass

    def settimeout(self, value):
        pass

    def sendall(self, value):
        pass


class CounterChecks(unittest.TestCase):
    def setUp(self):
        source = pathlib.Path(__file__).with_name("run_fixture.py").read_text(encoding="utf-8")
        tree = ast.parse(source)
        main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main")
        request = next(node for node in main.body if isinstance(node, ast.FunctionDef) and node.name == "request")
        module = ast.fix_missing_locations(ast.Module(body=[request], type_ignores=[]))
        self.result = {}
        self.reply = "OK assert_fires=0"

        class ObservedInvariant(Exception):
            pass

        self.observed = ObservedInvariant
        self.globals = {
            "deadline": 100, "time": types.SimpleNamespace(monotonic=lambda: 1),
            "socket": types.SimpleNamespace(create_connection=lambda *args, **kwargs: FakeClient()),
            "port": 1234, "token": "test", "result": self.result,
            "read_response": lambda client: self.reply, "ObservedInvariant": ObservedInvariant,
        }
        exec(compile(module, "run_fixture.py:request", "exec"), self.globals)

    def test_decrease_rejected(self):
        self.result["last_assert_fires"] = 3
        self.reply = "OK assert_fires=2"
        with self.assertRaisesRegex(RuntimeError, "counter decreased"):
            self.globals["request"]("STATUS")
        self.assertEqual(self.result["last_assert_fires"], 3)

    def test_positive_stops_and_is_recorded(self):
        self.reply = "OK assert_fires=2"
        with self.assertRaises(self.observed):
            self.globals["request"]("STATUS")
        self.assertEqual(self.result["max_assert_fires"], 2)

    def test_zero_is_accepted(self):
        self.assertEqual(self.globals["request"]("STATUS"), self.reply)
        self.assertEqual(self.result["max_assert_fires"], 0)


if __name__ == "__main__":
    unittest.main()
