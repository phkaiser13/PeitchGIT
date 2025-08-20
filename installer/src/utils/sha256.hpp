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

} // namespace phgit_installer::utils::crypto