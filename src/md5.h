#pragma once

#include <string>
#include <cstdint>

// Minimal public-domain MD5 implementation (single-file).
// Source adapted for brevity; suitable for small binary buffers.
// Returns lowercase hex string of MD5 digest.

struct MD5 {
    uint32_t h[4];
    uint8_t buffer[64];
    uint64_t bitlen;
    uint32_t buflen;

    MD5() { init(); }
    void init() {
        h[0] = 0x67452301;
        h[1] = 0xefcdab89;
        h[2] = 0x98badcfe;
        h[3] = 0x10325476;
        buflen = 0;
        bitlen = 0;
    }
    static uint32_t rotl(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }
    static uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    static uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    static uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    static uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

    void transform(const uint8_t chunk[64]) {
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t w[16];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t)chunk[i*4] | ((uint32_t)chunk[i*4+1] << 8) | ((uint32_t)chunk[i*4+2] << 16) | ((uint32_t)chunk[i*4+3] << 24);
        }
        const uint32_t K[] = {
            0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
            0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
            0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
            0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
            0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
            0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
            0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
            0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
        };
        const uint32_t S[] = {
            7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
            5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
            4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
            6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
        };
        for (int i = 0; i < 64; ++i) {
            uint32_t f, g;
            if (i < 16) { f = F(b,c,d); g = i; }
            else if (i < 32) { f = G(b,c,d); g = (5*i + 1) & 15; }
            else if (i < 48) { f = H(b,c,d); g = (3*i + 5) & 15; }
            else { f = I(b,c,d); g = (7*i) & 15; }
            uint32_t temp = d;
            d = c;
            c = b;
            uint32_t x = a + f + K[i] + w[g];
            b = b + rotl(x, S[i]);
            a = temp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    }

    void update(const uint8_t *data, size_t len) {
        while (len > 0) {
            uint32_t take = (buflen + len > 64) ? (64 - buflen) : (uint32_t)len;
            memcpy(buffer + buflen, data, take);
            buflen += take;
            data += take;
            len -= take;
            if (buflen == 64) {
                transform(buffer);
                bitlen += 512;
                buflen = 0;
            }
        }
    }

    std::string finalize() {
        // pad
        uint64_t bits = bitlen + (uint64_t)buflen * 8;
        buffer[buflen++] = 0x80;
        if (buflen > 56) {
            while (buflen < 64) buffer[buflen++] = 0;
            transform(buffer);
            buflen = 0;
        }
        while (buflen < 56) buffer[buflen++] = 0;
        for (int i = 0; i < 8; ++i) buffer[buflen++] = (uint8_t)((bits >> (8*i)) & 0xff);
        transform(buffer);
        char out[33];
        for (int i = 0; i < 4; ++i) {
            uint32_t val = h[i];
            for (int j = 0; j < 4; ++j) {
                uint8_t b = (val >> (8*j)) & 0xff;
                static const char hex[] = "0123456789abcdef";
                out[i*8 + j*2] = hex[(b >> 4) & 0xF];
                out[i*8 + j*2 + 1] = hex[b & 0xF];
            }
        }
        out[32] = '\0';
        return std::string(out);
    }
};

static inline std::string md5_hex(const uint8_t *data, size_t len) {
    MD5 ctx;
    ctx.init();
    ctx.update(data, len);
    return ctx.finalize();
}

static inline std::string md5_hex(const std::string &s) {
    return md5_hex(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}
