"""Minimal UE locres (v2/v3) reader/writer. Keeps the original key hashes so entries stay valid."""
import struct

MAGIC = bytes.fromhex('0e147475674a03fc4a15909dc3377f1b')


def _rs(d, o):
    n, = struct.unpack_from('<i', d, o); o += 4
    if n < 0:
        return d[o:o - 2 * n].decode('utf-16-le')[:-1], o - 2 * n
    return (d[o:o + n - 1].decode('latin-1') if n else ''), o + n


def _ws(s):
    if not s:
        return struct.pack('<i', 0)
    if all(ord(c) < 128 for c in s):
        b = s.encode('ascii') + b'\0'
        return struct.pack('<i', len(b)) + b
    b = s.encode('utf-16-le') + b'\0\0'
    return struct.pack('<i', -(len(b) // 2)) + b


class Locres:
    """namespaces: list of [ns_hash, ns_name, [[key_hash, key, source_hash, text], ...]]"""

    def __init__(self, path):
        d = open(path, 'rb').read()
        assert d[:16] == MAGIC, 'not a v1+ locres'
        self.version = d[16]
        assert self.version in (2, 3), f'unsupported locres version {self.version}'
        o = 17
        stroff, = struct.unpack_from('<q', d, o); o += 8
        o += 4  # entry count
        nns, = struct.unpack_from('<I', d, o); o += 4
        raw = []
        for _ in range(nns):
            nh, = struct.unpack_from('<I', d, o); o += 4
            name, o = _rs(d, o)
            nk, = struct.unpack_from('<I', d, o); o += 4
            keys = []
            for _ in range(nk):
                kh, = struct.unpack_from('<I', d, o); o += 4
                k, o = _rs(d, o)
                sh, idx = struct.unpack_from('<Ii', d, o); o += 8
                keys.append([kh, k, sh, idx])
            raw.append([nh, name, keys])
        o = stroff
        count, = struct.unpack_from('<i', d, o); o += 4
        strings = []
        for _ in range(count):
            s, o = _rs(d, o)
            o += 4  # refcount
            strings.append(s)
        for ns in raw:
            for e in ns[2]:
                e[3] = strings[e[3]]
        self.namespaces = raw

    def table(self, ns_name):
        """dict key -> entry list (mutate entry[3] to change the text)."""
        for ns in self.namespaces:
            if ns[1] == ns_name:
                return {e[1]: e for e in ns[2]}
        raise KeyError(ns_name)

    def save(self, path):
        strings, index = [], {}
        body = bytearray()
        body += struct.pack('<I', len(self.namespaces))
        total = 0
        for nh, name, keys in self.namespaces:
            body += struct.pack('<I', nh) + _ws(name) + struct.pack('<I', len(keys))
            for kh, k, sh, text in keys:
                if text not in index:
                    index[text] = len(strings); strings.append([text, 0])
                strings[index[text]][1] += 1
                body += struct.pack('<I', kh) + _ws(k) + struct.pack('<Ii', sh, index[text])
                total += 1
        head_len = 16 + 1 + 8 + 4
        out = bytearray(MAGIC) + bytes([self.version])
        out += struct.pack('<q', head_len + len(body)) + struct.pack('<I', total) + body
        out += struct.pack('<i', len(strings))
        for s, rc in strings:
            out += _ws(s) + struct.pack('<i', rc)
        open(path, 'wb').write(out)
