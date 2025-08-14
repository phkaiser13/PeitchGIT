/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: sha256.hpp
* This file provides a self-contained, platform-independent, and dependency-free
* implementation of the SHA256 hashing algorithm. It is based on a well-known public
* domain implementation, adapted for use within the phgit installer project. Its purpose
* is to provide a reliable and lightweight way to verify the integrity of downloaded
* files without introducing heavy cryptographic library dependencies like OpenSSL.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace phgit_installer::utils::crypto {

    class SHA256 {
    public:
        SHA256();
        void update(const uint8_t* data, size_t length);
        void update(const std::string& data);
        std::string final();
        static std::string from_file(const std::string& filepath);

    private:
        void transform(const uint8_t* data);

        uint8_t m_buffer[64];
        uint32_t m_state[8];
        uint64_t m_bit_count;
        size_t m_buffer_len;
    };

    // Implementation details directly in the header for simplicity, as it's self-contained.

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

        inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
        inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
        inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
        inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
        inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    }

    SHA256::SHA256() : m_bit_count(0), m_buffer_len(0) {
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;
    }

    void SHA256::transform(const uint8_t* data) {
        uint32_t a, b, c, d, e, f, g, h, w[64], t1, t2;
        for (unsigned i = 0, j = 0; i < 16; ++i, j += 4) {
            w[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        }
        for (unsigned i = 16; i < 64; ++i) {
            w[i] = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
        }

        a = m_state[0]; b = m_state[1]; c = m_state[2]; d = m_state[3];
        e = m_state[4]; f = m_state[5]; g = m_state[6]; h = m_state[7];

        for (unsigned i = 0; i < 64; ++i) {
            t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
            t2 = sig0(a) + majority(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d;
        m_state[4] += e; m_state[5] += f; m_state[6] += g; m_state[7] += h;
    }

    void SHA256::update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            m_buffer[m_buffer_len++] = data[i];
            if (m_buffer_len == 64) {
                transform(m_buffer);
                m_bit_count += 512;
                m_buffer_len = 0;
            }
        }
    }

    void SHA256::update(const std::string& data) {
        update(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
    }

    std::string SHA256::final() {
        uint8_t tail[64];
        size_t pad_len;

        uint64_t final_bit_count = m_bit_count + (m_buffer_len * 8);
        pad_len = (m_buffer_len < 56) ? (56 - m_buffer_len) : (120 - m_buffer_len);

        update(reinterpret_cast<const uint8_t*>("\x80"), 1);
        update(reinterpret_cast<const uint8_t*>("\0"), pad_len - 1);

        for (int i = 7; i >= 0; --i) {
            tail[i] = static_cast<uint8_t>(final_bit_count);
            final_bit_count >>= 8;
        }
        update(tail, 8);

        std::stringstream ss;
        for (uint32_t i : m_state) {
            ss << std::hex << std::setw(8) << std::setfill('0') << i;
        }
        return ss.str();
    }

    std::string SHA256::from_file(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        SHA256 sha;
        char buffer[4096];
        while (file.read(buffer, sizeof(buffer))) {
            sha.update(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
        }
        sha.update(reinterpret_cast<uint8_t*>(buffer), file.gcount());
        return sha.final();
    }

} // namespace phgit_installer::utils::crypto
