#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>

class SHA256 {
public:
    SHA256() { reset(); }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            buffer[bufferLength] = data[i];
            bufferLength++;
            if (bufferLength == 64) {
                processBlock(buffer);
                bitLength += 512;
                bufferLength = 0;
            }
        }
    }

    void update(const std::string& data) {
        update(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    }

    std::string final() {
        uint32_t i = bufferLength;

        if (bufferLength < 56) {
            buffer[i++] = 0x80;
            while (i < 56)
                buffer[i++] = 0x00;
        }
        else {
            buffer[i++] = 0x80;
            while (i < 64)
                buffer[i++] = 0x00;
            processBlock(buffer);
            memset(buffer, 0, 56);
        }

        bitLength += bufferLength * 8;
        buffer[63] = bitLength;
        buffer[62] = bitLength >> 8;
        buffer[61] = bitLength >> 16;
        buffer[60] = bitLength >> 24;
        buffer[59] = bitLength >> 32;
        buffer[58] = bitLength >> 40;
        buffer[57] = bitLength >> 48;
        buffer[56] = bitLength >> 56;
        processBlock(buffer);

        std::string result;
        for (uint32_t i = 0; i < 8; ++i) {
            result += static_cast<char>((state[i] >> 24) & 0xFF);
            result += static_cast<char>((state[i] >> 16) & 0xFF);
            result += static_cast<char>((state[i] >> 8) & 0xFF);
            result += static_cast<char>(state[i] & 0xFF);
        }

        reset();
        return result;
    }

    static std::string hash(const std::string& input) {
        SHA256 sha;
        sha.update(input);
        return sha.final();
    }

private:
    uint8_t buffer[64];
    uint32_t bufferLength;
    uint64_t bitLength;
    uint32_t state[8];

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    void reset() {
        bufferLength = 0;
        bitLength = 0;
        state[0] = 0x6a09e667;
        state[1] = 0xbb67ae85;
        state[2] = 0x3c6ef372;
        state[3] = 0xa54ff53a;
        state[4] = 0x510e527f;
        state[5] = 0x9b05688c;
        state[6] = 0x1f83d9ab;
        state[7] = 0x5be0cd19;
    }

    uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

    void processBlock(const uint8_t block[64]) {
        uint32_t w[64];
        for (uint32_t i = 0; i < 16; ++i) {
            w[i] = (block[i * 4] << 24) |
                (block[i * 4 + 1] << 16) |
                (block[i * 4 + 2] << 8) |
                (block[i * 4 + 3]);
        }

        for (uint32_t i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        for (uint32_t i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
};
