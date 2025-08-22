/* Copyright (C) 2025 Pedro Henrique / phkaiser13
 * File: windows_installer.cpp
 * Refactored to be an interactive post-installation assistant.
 * Removed silent downloads, registry edits, PATH modifications, and archive extraction.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/windows/windows_installer.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <iostream>
#include <windows.h>
#include <shlobj.h>

namespace phgit_installer::platform {

    namespace fs = std::filesystem;

    WindowsInstaller::WindowsInstaller(
        platform::PlatformInfo info,
        const dependencies::DependencyManager& dep_manager,
        std::shared_ptr<utils::ApiManager> api_manager,
        std::shared_ptr<utils::ConfigManager> config)
        : m_platform_info(std::move(info)),
          m_dep_manager(dep_manager),
          m_api_manager(std::move(api_manager)),
          m_config(std::move(config)) {
        spdlog::debug("WindowsInstaller engine initialized (interactive assistant mode).");
    }

    void WindowsInstaller::run_installation() {
        spdlog::info("Iniciando tarefas de pós-instalação para Windows.");

        // Check Git
        auto git_status = m_dep_manager.get_status("git").value_or(dependencies::DependencyStatus{});
        if (!git_status.is_version_ok) {
            prompt_user_to_install_git();
        } else {
            spdlog::info("Git presente e versão compatível.");
        }

        // Optional deps: Terraform and Vault
        for (const auto& name : {"terraform", "vault"}) {
            auto status = m_dep_manager.get_status(name).value_or(dependencies::DependencyStatus{});
            if (!status.is_version_ok) {
                prompt_user_to_install_optional(name);
            } else {
                spdlog::info("Dependência opcional '{}' presente e versão compatível.", name);
            }
        }

        spdlog::info("Verificação de dependências concluída. Prosseguindo com verificações locais.");
        perform_installation();
    }

    void WindowsInstaller::perform_installation() {
        // Determine install directory (same logic as before, but do not modify system)
        bool for_all_users = m_platform_info.is_privileged;
        std::string install_dir;
        char path[MAX_PATH];
        if (for_all_users) {
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path)))
                install_dir = (fs::path(path) / "phgit").string();
        } else {
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
                install_dir = (fs::path(path) / "Programs" / "phgit").string();
        }

        if (install_dir.empty()) {
            spdlog::error("Não foi possível determinar o diretório de instalação. Abortando.");
            return;
        }

        spdlog::info("Target installation directory: {}", install_dir);
        fs::create_directories(install_dir);

        // Confirm application files (NSIS is expected to have already copied them)
        install_application_files(install_dir);

        // Do NOT perform system integration here — NSIS/WiX must handle PATH/registry/shortcuts.
        spdlog::info("System integration (PATH/registry/shortcuts) is handled by the package installer (NSIS/WiX).");
        spdlog::info("Windows post-installation assistant completed.");
    }

    bool WindowsInstaller::install_application_files(const std::string& install_path) {
        spdlog::info("Verificando arquivos do aplicativo em {}", install_path);
        bool ok = true;
        if (!fs::exists(fs::path(install_path) / "bin" / "phgit.exe")) {
             spdlog::warn("Main application executable 'phgit.exe' not found where expected.");
             ok = false;
        }
        if (!fs::exists(fs::path(install_path) / "config.json")) {
             spdlog::warn("Core 'config.json' not found where expected.");
             ok = false;
        }
        if (ok) spdlog::info("Arquivos principais verificados.");
        else spdlog::warn("Alguns arquivos esperados não foram encontrados. Confirme o pacote NSIS.");
        return ok;
    }

    void WindowsInstaller::prompt_user_to_install_git() {
        // Prompt the user and, if agreed, open the official Git for Windows download page.
        // This function intentionally does not download or install anything.
        std::cout << std::endl;
        std::cout << "Git for Windows não foi encontrado ou não atende à versão mínima." << std::endl;
        std::cout << "Recomendamos instalar o Git manualmente para garantir controle e segurança." << std::endl;
        std::cout << "Deseja abrir a página oficial de download do Git for Windows agora? (s/N): ";
        std::string response;
        std::getline(std::cin, response);

        if (!response.empty() && (response[0] == 's' || response[0] == 'S' || response[0] == 'y' || response[0] == 'Y')) {
            const char* url = "https://git-scm.com/download/win";
            spdlog::info("Abrindo navegador para: {}", url);
            HINSTANCE result = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
            if ((intptr_t)result <= 32) {
                spdlog::warn("Falha ao abrir o navegador automaticamente. Forneça manualmente: {}", url);
                std::cout << "Por favor, visite: " << url << std::endl;
            } else {
                spdlog::info("Navegador aberto com sucesso (se suportado pelo sistema).");
            }
        } else {
            spdlog::info("Usuário optou por não abrir a página de download do Git neste momento.");
        }
    }

    void WindowsInstaller::prompt_user_to_install_optional(const std::string& dependency_name) {
        // Prompt the user and, if agreed, open the official download/documentation page for the dependency.
        std::cout << std::endl;
        std::cout << "Dependência opcional '" << dependency_name << "' não encontrada ou desatualizada." << std::endl;
        std::cout << "Deseja abrir a página oficial de download/documentação para '" << dependency_name << "'? (s/N): ";
        std::string response;
        std::getline(std::cin, response);

        if (!response.empty() && (response[0] == 's' || response[0] == 'S' || response[0] == 'y' || response[0] == 'Y')) {
            const char* url = nullptr;
            if (dependency_name == "terraform") {
                url = "https://developer.hashicorp.com/terraform/downloads";
            } else if (dependency_name == "vault") {
                url = "https://developer.hashicorp.com/vault/downloads";
            } else {
                // Fallback to a search page for unknown dependencies
                url = "https://www.google.com/search?q=";
            }

            spdlog::info("Abrindo navegador para: {}", url);
            HINSTANCE result = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
            if ((intptr_t)result <= 32) {
                spdlog::warn("Falha ao abrir o navegador automaticamente. Forneça manualmente: {}", url);
                std::cout << "Por favor, visite: " << url << std::endl;
            } else {
                spdlog::info("Navegador aberto com sucesso (se suportado pelo sistema).");
            }
        } else {
            spdlog::info("Usuário optou por não abrir a página de '{}' neste momento.", dependency_name);
        }
    }

} // namespace phgit_installer::platform
