# Copyright (C) 2025 Pedro Henrique / phkaiser13
# Makefile - Um wrapper conveniente e amigável para o CMake.
#
# Este Makefile não contém lógica de compilação. Ele serve como uma fachada,
# fornecendo comandos simples e memoráveis que se traduzem nos comandos
# mais verbosos necessários para operar o CMake corretamente.
#
# Isto melhora a experiência do desenvolvedor, especialmente em sistemas POSIX.
#
# SPDX-License-Identifier: Apache-2.0

# --- Variáveis ---
# Permite ao utilizador sobrepor o comando cmake (ex: make CMAKE=cmake3)
CMAKE ?= cmake
# Diretório de compilação
BUILD_DIR := build
# Nome do executável final
EXECUTABLE_NAME := gitph
# Caminho para o executável
EXECUTABLE_PATH := ${BUILD_DIR}/bin/${EXECUTABLE_NAME}

# Alvos Phony não representam ficheiros.
.PHONY: all build configure clean rebuild run install help

# --- Alvos ---

# Alvo por defeito: simplesmente constrói o projeto.
all: build

# Configura o projeto se ainda não estiver configurado.
# A existência do Makefile gerado pelo CMake é usada como um marcador.
configure: ${BUILD_DIR}/Makefile

${BUILD_DIR}/Makefile: CMakeLists.txt
	@echo "--- A configurar o projeto com CMake ---"
	@${CMAKE} -S . -B ${BUILD_DIR}

# Constrói o projeto. Depende da configuração primeiro.
build: configure
	@echo "--- A compilar o projeto ---"
	@${CMAKE} --build ${BUILD_DIR} --parallel

# Limpa todos os artefactos de compilação.
clean:
	@echo "--- A limpar os artefactos de compilação ---"
	@rm -rf ${BUILD_DIR} release

# Limpa e reconstrói tudo do zero.
rebuild: clean all

# Constrói e executa a aplicação com quaisquer argumentos passados.
# Exemplo: make run ARGS="--version"
run: build
	@echo "--- A executar o gitph ---"
	@${EXECUTABLE_PATH} ${ARGS}

# Instala a aplicação usando as regras definidas no CMake.
install: build
	@echo "--- A instalar o gitph ---"
	@${CMAKE} --install ${BUILD_DIR}

# Exibe informações de ajuda.
help:
	@echo "Wrapper Makefile do gitph"
	@echo "-------------------------"
	@echo "Uso: make [alvo]"
	@echo ""
	@echo "Alvos:"
	@echo "  all        (Padrão) Configura se necessário e constrói o projeto."
	@echo "  build      Constrói o projeto se já estiver configurado."
	@echo "  configure  Executa o CMake para gerar o sistema de compilação."
	@echo "  rebuild    Limpa todos os ficheiros de compilação e reconstrói."
	@echo "  run        Constrói e executa a aplicação principal. Use ARGS=\"...\" para passar argumentos."
	@echo "  install    Instala a aplicação no diretório configurado."
	@echo "  clean      Remove todos os artefactos de compilação."
	@echo "  help       Mostra esta mensagem de ajuda."
