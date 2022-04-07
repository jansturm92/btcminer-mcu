#  Original work Copyright (C) 2011 by jedi95 <jedi95@gmail.com> and
#                                   CFSworks <CFSworks@gmail.com>
#  Modified work Copyright (C) 2022 Jan Sturm
#
#  This program is free software: you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, either version 3 of the License, or (at your option) any later
#  version.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with
#  this program.  If not, see <http://www.gnu.org/licenses/>.
import struct

# fmt: off
K = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
]
# fmt: on

A0 = 0x6A09E667
B0 = 0xBB67AE85
C0 = 0x3C6EF372
D0 = 0xA54FF53A
E0 = 0x510E527F
F0 = 0x9B05688C
G0 = 0x1F83D9AB
H0 = 0x5BE0CD19


def rotateright(i, p):
    """i>>>p"""
    p &= 0x1F  # p mod 32
    return i >> p | ((i << (32 - p)) & 0xFFFFFFFF)


def addu32(*i):
    return sum(list(i)) & 0xFFFFFFFF


def calculate_midstate(header: bytes) -> bytes:
    """
    Calculates the SHA256 midstate for the first 64-byte chunk of block header data,
    which can be reused in further SHA256 computations

    This optimization is possible since SHA256 operates on chunks of 64 bytes.
    Changing the nonce in the second chunk does not affect the state of the hash function after hashing the first chunk.

    :param header: 80 byte block header in little endian
    :return: 32 byte midstate
    """
    w = list(struct.unpack(f">{'I' * 16}", header[:64]))

    a, b, c, d, e, f, g, h = A0, B0, C0, D0, E0, F0, G0, H0

    for k in K:
        s0 = rotateright(a, 2) ^ rotateright(a, 13) ^ rotateright(a, 22)
        s1 = rotateright(e, 6) ^ rotateright(e, 11) ^ rotateright(e, 25)
        ma = (a & b) ^ (a & c) ^ (b & c)
        ch = (e & f) ^ ((~e) & g)

        h = addu32(h, w[0], k, ch, s1)
        d = addu32(d, h)
        h = addu32(h, ma, s0)

        a, b, c, d, e, f, g, h = h, a, b, c, d, e, f, g

        s0 = rotateright(w[1], 7) ^ rotateright(w[1], 18) ^ (w[1] >> 3)
        s1 = rotateright(w[14], 17) ^ rotateright(w[14], 19) ^ (w[14] >> 10)
        w.append(addu32(w[0], s0, w[9], s1))
        w.pop(0)

    a = addu32(a, A0)
    b = addu32(b, B0)
    c = addu32(c, C0)
    d = addu32(d, D0)
    e = addu32(e, E0)
    f = addu32(f, F0)
    g = addu32(g, G0)
    h = addu32(h, H0)

    return struct.pack("<IIIIIIII", a, b, c, d, e, f, g, h)
