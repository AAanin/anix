import argparse
import json
import struct
import zlib
from pathlib import Path
if __package__:
    from .verifier import verify
else:
    from verifier import verify

def build(graph):
    proof = verify(graph)
    base = len(graph['inputs']) * 8
    payload = b''.join(struct.pack('<II', *c) for c in graph['inputs'])
    for node in graph['nodes']:
        if node['kind'] == 'action':
            words = [0, node['action'], 0, 0, 0, 0, 0, 0]
        else:
            words = [1, node['input'], node['threshold'], node['lower'], node['upper']]
            words += [base + node[k]*32 for k in ('yes', 'no', 'shadow')]
        payload += struct.pack('<8I', *words)
    return (b'ANIX' + struct.pack('<5I', 1, graph['policy_id'], base+graph['entry']*32,
            len(payload), zlib.crc32(payload)) + proof +
            struct.pack('<3I', len(graph['nodes']), len(graph['inputs']), 1) + payload)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('graph'); parser.add_argument('output')
    args = parser.parse_args()
    Path(args.output).write_bytes(build(json.loads(Path(args.graph).read_text())))
