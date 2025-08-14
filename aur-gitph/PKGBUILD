# Maintainer: Pedro Henrique / phkaiser13 <seu-email@example.com>

# Metadados do pacote
pkgname='gitph'
_pkgname='peitchgit'
# O workflow irá atualizar o pkgver
pkgver=${GITPH_VERSION}
pkgrel=1
pkgdesc="The Polyglot Assistant for Git & DevOps Workflows"
arch=('x86_64' 'aarch64')
url="https://github.com/phkaiser13/peitchgit"
license=('Apache-2.0')

# Dependências de execução
depends=('curl' 'lua' 'nlohmann-json')
# Dependências de build
makedepends=('cmake' 'rust')

# Fonte do código
source=("${_pkgname}-${pkgver}.tar.gz::https://github.com/phkaiser13/${_pkgname}/archive/refs/tags/v${pkgver}.tar.gz")
# O workflow irá gerar e inserir o hash SHA256 aqui
sha256sums=('${GITPH_SHA256}')

# Função de compilação
build() {
  cd "${srcdir}/${_pkgname}-${pkgver}"
  
  # Processo de build padrão com CMake
  cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  
  cmake --build build
}

# Função de empacotamento
package() {
  cd "${srcdir}/${_pkgname}-${pkgver}"
  DESTDIR="${pkgdir}" cmake --install build
}
