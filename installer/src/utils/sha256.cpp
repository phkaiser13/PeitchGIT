/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: sha256.cpp
* Implementation of the SHA256 hashing algorithm.
* SPDX-License-Identifier: Apache-2.0
*/

#include "sha256.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace phgit_installer::utils::crypto {

    // SHA-256 constants
    namespace {
        constexpr uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        // SHA-256 helper functions
        inline uint32_t rotr(uint32_t x, uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
            return (e & f) ^ (~e & g);
        }

        inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
            return (a & b) ^ (a & c) ^ (b & c);
        }

        inline uint32_t sig0(uint32_t x) {
            return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
        }

        inline uint32_t sig1(uint32_t x) {
            return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
        }

        inline uint32_t gamma0(uint32_t x) {
            return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
        }

        inline uint32_t gamma1(uint32_t x) {
            return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
        }
    }

    // Constructor - initialize SHA-256 state
    SHA256::SHA256() : m_bit_count(0), m_buffer_len(0) {
        // Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;
    }

    // Process a 512-bit block of data
    void SHA256::transform(const uint8_t* data) {
        uint32_t a, b, c, d, e, f, g, h;
        uint32_t w[64];
        uint32_t t1, t2;

        // Copy chunk into first 16 words w[0..15] of the message schedule array
        for (unsigned i = 0, j = 0; i < 16; ++i, j += 4) {
            w[i] = (static_cast<uint32_t>(data[j]) << 24) |
                   (static_cast<uint32_t>(data[j + 1]) << 16) |
                   (static_cast<uint32_t>(data[j + 2]) << 8) |
                   (static_cast<uint32_t>(data[j + 3]));
        }

        // Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array
        for (unsigned i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        // Initialize working variables for this chunk
        a = m_state[0];
        b = m_state[1];
        c = m_state[2];
        d = m_state[3];
        e = m_state[4];
        f = m_state[5];
        g = m_state[6];
        h = m_state[7];

        // Main loop (compression function)
        for (unsigned i = 0; i < 64; ++i) {
            t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
            t2 = sig0(a) + majority(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        // Add this chunk's hash to result so far
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
        m_state[5] += f;
        m_state[6] += g;
        m_state[7] += h;
    }

    // Update hash with new data
    void SHA256::update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            m_buffer[m_buffer_len++] = data[i];

            // Process full 512-bit blocks
            if (m_buffer_len == 64) {
                transform(m_buffer);
                m_bit_count += 512;
                m_buffer_len = 0;
            }
        }
    }

    // Update hash with string data
    void SHA256::update(const std::string& data) {
        update(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }

    // Finalize hash and return hex string
    std::string SHA256::final() {
        uint8_t tail[8];

        // Calculate final bit count
        uint64_t final_bit_count = m_bit_count + (m_buffer_len * 8);

        // Pre-processing: adding padding bits
        // Append the '1' bit (plus zero padding to make it a byte)
        update(reinterpret_cast<const uint8_t*>("\x80"), 1);

        // Append zeros to make message length ≡ 448 (mod 512)
        while (m_buffer_len != 56) {
            update(reinterpret_cast<const uint8_t*>("\0"), 1);
        }

        // Append original length in bits as 64-bit big-endian integer
        for (int i = 7; i >= 0; --i) {
            tail[7 - i] = static_cast<uint8_t>(final_bit_count >> (i * 8));
        }
        update(tail, 8);

        // Convert final hash to hex string
        std::stringstream ss;
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << std::setw(8) << std::setfill('0') << m_state[i];
        }

        return ss.str();
    }

    // Static method to compute SHA-256 hash of a file
    std::string SHA256::from_file(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return ""; // Return empty string on error
        }

        SHA256 sha;
        char buffer[4096];

        // Read file in chunks and update hash
        while (file.read(buffer, sizeof(buffer))) {
            sha.update(reinterpret_cast<const uint8_t*>(buffer), sizeof(buffer));
        }

        // Handle the last partial read (if any)
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            sha.update(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(bytes_read));
        }

        file.close();
        return sha.final();
    }

} // namespace phgit_installer::utils::crypto