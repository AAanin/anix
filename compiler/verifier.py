"""Bounded DAG verification over all unsigned 32-bit sensor valuations."""
import hashlib
import json
import z3

def verify(graph):
    nodes = graph['nodes']
    contracts = graph['inputs']
    allowed = graph['safe_actions']
    def u32(v):
        return type(v) is int and 0 <= v <= 0xffffffff
    if not 1 <= len(nodes) <= 4096 or len(contracts) > 256:
        raise ValueError('graph size')
    if not u32(graph['policy_id']) or type(graph['entry']) is not int or not 0 <= graph['entry'] < len(nodes):
        raise ValueError('header')
    for lo, hi in contracts:
        if not u32(lo) or not u32(hi) or lo > hi:
            raise ValueError('contract')
    if not allowed or not all(u32(a) for a in allowed):
        raise ValueError('safety actions')
    xs = [z3.Int('sensor_%d' % i) for i in range(len(contracts))]
    domain = z3.And(*[z3.And(x >= 0, x <= 0xffffffff) for x in xs])
    reach = [z3.BoolVal(False) for _ in nodes]
    reach[graph['entry']] = z3.BoolVal(True)
    for i, node in enumerate(nodes):
        if node['kind'] == 'action':
            if not u32(node['action']):
                raise ValueError('action')
            if node['action'] not in allowed:
                s = z3.Solver(); s.add(domain, reach[i])
                if s.check() != z3.unsat:
                    raise ValueError('unsafe reachable action')
        elif node['kind'] == 'decision':
            idx = node['input']
            if type(idx) is not int or not 0 <= idx < len(xs):
                raise ValueError('input index')
            lo, hi = node['lower'], node['upper']
            if not all(u32(v) for v in (lo, hi, node['threshold'])) or lo > hi:
                raise ValueError('decision bounds')
            x = xs[idx]; clo, chi = contracts[idx]
            valid = z3.And(x >= lo, x <= hi, x >= clo, x <= chi)
            conditions = [z3.And(valid, x <= node['threshold']),
                          z3.And(valid, x > node['threshold']), z3.Not(valid)]
            for name, condition in zip(('yes', 'no', 'shadow'), conditions):
                dst = node[name]
                if type(dst) is not int or not i < dst < len(nodes):
                    raise ValueError('edges must strictly advance')
                reach[dst] = z3.Or(reach[dst], z3.And(reach[i], condition))
        else:
            raise ValueError('node kind')
    terminal = z3.Or(*[reach[i] for i, n in enumerate(nodes) if n['kind'] == 'action'])
    s = z3.Solver(); s.add(domain, z3.Not(terminal))
    if s.check() != z3.unsat:
        raise ValueError('nontermination or dead end')
    s = z3.Solver(); s.add(domain, terminal)
    if s.check() != z3.sat:
        raise ValueError('no reachable terminal')
    canonical = json.dumps(graph, sort_keys=True, separators=(',', ':')).encode()
    return hashlib.sha256(b'ANIX-verifier-v1\0' + canonical).digest()
