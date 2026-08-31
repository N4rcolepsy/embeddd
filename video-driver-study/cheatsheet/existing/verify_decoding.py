"""Check the documented arithmetic, not compilation or execution of the C file."""
import json


def decode(b):
    u = (b[0] << 12) | (b[1] << 4) | (b[2] >> 4)
    return u - (1 << 20) if u & (1 << 19) else u


for raw in range(1 << 20):
    packed = (raw << 4).to_bytes(3, "big")
    expected = raw if raw < (1 << 19) else raw - (1 << 20)
    assert decode(packed) == expected

for raw in [0, 1, 256000, 524287, 524288, 792576, 1048575]:
    for ignored in range(16):
        packed = ((raw << 4) | ignored).to_bytes(3, "big")
        expected = raw if raw < (1 << 19) else raw - (1 << 20)
        assert decode(packed) == expected

for raw in range(4096):
    for reserved in range(16):
        hi, lo = ((reserved << 12) | raw).to_bytes(2, "big")
        assert ((hi & 0x0F) << 8) | lo == raw

assert 25.0 + (1885.0 - 1885.0) / -9.05 == 25.0
assert abs(256000 * (9.80665 / 256000) - 9.80665) < 1e-12

print(json.dumps({
    "signed20_patterns": 1048576,
    "signed20_reserved_nibble_cases": 112,
    "temperature12_reserved_nibble_cases": 65536,
    "nominal_scale_checks": 2,
    "result": "PASS",
    "scope": "Python arithmetic model only; separate C results: examples/memory_io/verification.json; no hardware test"
}, indent=2))
