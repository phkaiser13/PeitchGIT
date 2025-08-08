
-----

# 📖 gitph - O Assistente Git Poliglota

**gitph** é uma ferramenta de linha de comando (CLI) moderna e extensível, projetada para otimizar e simplificar o seu fluxo de trabalho com Git e DevOps. Construído sobre uma arquitetura poliglota única, ele combina a performance do C/C++ e Rust com a agilidade do Go e a flexibilidade do Lua para oferecer um conjunto de funcionalidades robusto e de alto desempenho.

## ✨ Funcionalidades Principais

  - **Interface Dupla:** Use o `gitph` diretamente na linha de comando para automação ou entre no **modo interativo (TUI)** para uma experiência guiada por menus.
  - **Comandos Git Simplificados:** Atalhos inteligentes para operações complexas, como `gitph SND`, que automaticamente adiciona, faz o commit e o push das suas alterações.
  - **Motor de Sincronização Avançado:** Um poderoso motor de sincronização bidirecional que manipula o repositório Git a baixo nível para operações complexas que o Git CLI comum não suporta.
  - **Automação de DevOps:** Integre-se com as suas ferramentas de IaC e gestão de segredos com *wrappers* inteligentes para **Terraform** e **Vault**.
  - **Integração com Issues:** Visualize detalhes de issues do GitHub diretamente do seu terminal.
  - **Visualizador de CI/CD:** Faça o parse de ficheiros de workflow (ex: GitHub Actions) e visualize a pipeline.
  - **Extremamente Extensível:** Adicione os seus próprios aliases e crie hooks (como validações de pré-push) usando a simplicidade da linguagem de scripting **Lua**.

## 🛠️ A Arquitetura Poliglota

O `gitph` é um estudo de caso em arquitetura de software poliglota. Cada linguagem foi escolhida para a tarefa em que ela se destaca, criando um sistema coeso e de alto desempenho.

| Linguagem  | Ícone | Responsabilidade |
|------------|:----:|------------------|
| **C** | <img src="https://img.icons8.com/color/48/000000/c-programming.png" width="24"/> | **O Orquestrador.** Núcleo da aplicação desenvolvido em C puro para garantir portabilidade e controle absoluto. Responsável pela CLI, carregamento dinâmico de módulos e orquestração do ciclo de vida da aplicação. |
| **C++** | <img src="https://img.icons8.com/color/48/000000/c-plus-plus-logo.png" width="24"/> | **Bibliotecas de Alto Desempenho.** Utilizado para componentes internos com alta demanda de performance e segurança, como o logger thread-safe e o visualizador de pipelines CI/CD, explorando os recursos modernos da linguagem. |
| **Rust** | <img src="https://img.icons8.com/color/48/000000/rust-programming-language.png" width="24"/> | **Segurança e Performance Crítica.** Aplicado em módulos sensíveis que exigem máxima eficiência e segurança de memória, como o motor de sincronização de repositórios (`sync_engine`) e o cliente assíncrono de APIs (`issue_tracker`). |
| **Go** | <img src="https://img.icons8.com/color/48/000000/golang.png" width="24"/> | **Concorrência Simples e Efetiva.** Ideal para integração com APIs, automação DevOps e *wrappers* de ferramentas externas. Sua sintaxe enxuta e goroutines tornam o Go perfeito para o `api_client` e o `devops_automation`. |
| **Lua** | <img src="https://img.icons8.com/color/48/000000/lua-language.png" width="24"/> | **Scripting Extensível.** Empregado como motor de scripts para permitir extensibilidade e personalização por parte do usuário, facilitando a criação de hooks, aliases e automações no `gitph`. |
| **Protobuf** | <img src="https://img.icons8.com/color/48/000000/google-logo.png" width="24"/> | **Contrato de Dados.** Utilizado como linguagem neutra de definição de estruturas, permitindo comunicação eficiente entre componentes escritos em diferentes linguagens, como o parser em Go e o visualizador em C++. |


## 🚀 Começar

### Pré-requisitos

Para compilar o `gitph`, irá precisar das seguintes ferramentas:

  - `gcc`/`g++` (ou `clang`)
  - `cmake` (versão 3.15+)
  - `go` (versão 1.18+)
  - `rustc` e `cargo`
  - `liblua5.4-dev` (headers de desenvolvimento)
  - `libcurl-dev` (headers de desenvolvimento)

Para uma configuração rápida em sistemas baseados em Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake golang rustc liblua5.4-dev libcurl4-openssl-dev
```

### Compilação

O projeto usa **CMake** como sistema de build principal, com um `Makefile` para conveniência.

1.  **Clone o repositório:**

    ```bash
    git clone https://github.com/seu-usuario/gitph.git
    cd gitph
    ```

2.  **Configure e compile (Método Fácil):**

    ```bash
    make
    ```

    Ou, passo a passo:

    ```bash
    make configure  # Apenas na primeira vez
    make build
    ```

3.  **Após a compilação:**

      - O executável principal estará em `build/bin/gitph`.
      - Todos os módulos (`.so` ou `.dll`) estarão em `build/bin/modules/`.

## 💻 Como Usar

### Modo Linha de Comando (CLI)

Execute comandos diretamente para automação e scripting.

```bash
# Ver o status do repositório
./build/bin/gitph status

# Adicionar, fazer commit e push de todas as alterações com um único comando
./build/bin/gitph SND

# Ver os detalhes de uma issue do GitHub
./build/bin/gitph issue-get <user/repo> <issue_id>
```

### Modo Interativo (TUI)

Execute `gitph` sem argumentos para entrar num menu interativo, ideal para explorar as funcionalidades.

```bash
./build/bin/gitph
```

## 🔌 Extensibilidade com Lua

Pode adicionar os seus próprios scripts `.lua` no diretório `src/plugins/` para personalizar o `gitph`.

**Exemplo: `src/plugins/custom_aliases.lua`**

```lua
-- src/plugins/custom_aliases.lua

-- Cria um atalho 'st' para o comando 'status'
if gitph.register_alias then
  gitph.register_alias("st", "status")
  gitph.log("INFO", "Alias 'st' para 'status' registado!")
end
```

## 🤝 Contribuir

Contribuições são bem-vindas\! Se quiser melhorar o `gitph`, por favor, siga estes passos:

1.  Faça um Fork do projeto.
2.  Crie uma nova branch (`git checkout -b feature/NovaFuncionalidade`).
3.  Faça commit das suas alterações (`git commit -m 'Adiciona NovaFuncionalidade'`).
4.  Faça Push para a sua branch (`git push origin feature/NovaFuncionalidade`).
5.  Abra um Pull Request.

## 📜 Licença

Este projeto está licenciado sob a **GNU General Public License v3.0**. Veja o ficheiro [LICENSE](https://www.google.com/search?q=LICENSE) para mais detalhes.
