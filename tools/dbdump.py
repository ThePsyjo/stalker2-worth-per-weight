"""Dump a Zen-format ModLocalizationDatabase package: key -> {name index: text}. Usage: dbdump.py <pkg.uasset>"""
import struct, sys


def read(path):
    d = open(path, 'rb').read()

    def fs(o):
        n, = struct.unpack_from('<i', d, o); o += 4
        if n < 0:
            return d[o:o - 2 * n].decode('utf-16-le')[:-1], o - 2 * n
        return d[o:o + n - 1].decode('latin-1'), o + n
    o = struct.unpack_from('<I', d, 4)[0] + 2
    _, num = struct.unpack_from('<ii', d, o); o += 8
    out = {}
    for _ in range(num):
        k, o = fs(o); o += 2
        _, n2 = struct.unpack_from('<ii', d, o); o += 8
        texts = {}
        for _ in range(n2):
            ni, _ = struct.unpack_from('<II', d, o); o += 8
            texts[ni], o = fs(o)
        out[k] = texts
    return out


if __name__ == '__main__':
    for k, t in read(sys.argv[1]).items():
        print(k, '|', t.get(4, ''))
