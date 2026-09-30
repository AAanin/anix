import copy
import json
import struct
import unittest
import zlib
from pathlib import Path
from compiler.builder import build
from compiler.verifier import verify

class CompilerTests(unittest.TestCase):
    def setUp(self):
        self.g = json.loads(Path('examples/policy.json').read_text())
    def test_wire(self):
        b = build(self.g)
        self.assertEqual(len(b), 204)
        self.assertEqual(b[:4], b'ANIX')
        self.assertEqual(struct.unpack_from('<I', b, 20)[0], zlib.crc32(b[68:]))
        self.assertEqual(b[24:56], verify(self.g))
    def test_rejections(self):
        for key, value in [('yes', 0), ('shadow', 99), ('input', 3), ('lower', -1)]:
            g = copy.deepcopy(self.g); g['nodes'][0][key] = value
            with self.assertRaises(ValueError): build(g)
        self.g['safe_actions'] = [10, 20]
        with self.assertRaises(ValueError): build(self.g)
    def test_constant_policy(self):
        g = {'policy_id': 0, 'entry': 0, 'inputs': [], 'safe_actions': [0],
             'nodes': [{'kind': 'action', 'action': 0}]}
        self.assertEqual(len(build(g)), 100)

    def test_maximum_and_oversize(self):
        g = {'policy_id': 0, 'entry': 0, 'inputs': [], 'safe_actions': [1],
             'nodes': [{'kind': 'action', 'action': 1}] * 4096}
        self.assertEqual(len(build(g)), 68 + 32 * 4096)
        g['nodes'].append({'kind': 'action', 'action': 1})
        with self.assertRaises(ValueError): build(g)

    def test_deterministic(self):
        self.assertEqual(build(self.g), build(copy.deepcopy(self.g)))

if __name__ == '__main__': unittest.main()
