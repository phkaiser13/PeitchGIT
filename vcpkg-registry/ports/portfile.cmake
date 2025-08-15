# Copyright (C) 2025 Pedro Henrique / phkaiser13
# SPDX-License-Identifier: Apache-2.0

include(vcpkg_common_functions)

# 1. Baixa o código-fonte do release do GitHub.
#    As variáveis REF e SHA512 serão atualizadas dinamicamente pelo workflow.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO phkaiser13/peitchgit
    # O workflow irá substituir esta tag de versão
    REF "v${phgit_VERSION}" 
    # O workflow irá substituir este hash
    SHA512 "${phgit_SHA512}" 
    HEAD_REF main
)

# 2. Configura o projeto com CMake.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        # Desabilita a compilação de testes, que não são necessários para o pacote.
        -DBUILD_TESTING=OFF
)

# 3. Compila o projeto.
vcpkg_cmake_build()

# 4. Instala os artefatos.
vcpkg_cmake_install()

# 5. Copia o executável para a pasta de ferramentas do vcpkg.
file(INSTALL
    "${CURRENT_PACKAGES_DIR}/bin/phgit${VCPKG_HOST_EXECUTABLE_SUFFIX}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/phgit"
)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug")


# 6. Instala o arquivo de licença.
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
