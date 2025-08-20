// CORREÇÃO: 'invoke' e 'listen' agora são importados de pacotes diferentes.
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import "./styles.css";

// --- Nossos Tipos (Interfaces) ---

interface DiffLine {
    line_type: 'Context' | 'Addition' | 'Deletion';
    content: string;
}
interface FileDiff {
    lines: DiffLine[];
}
type FileStatusType = "New" | "Modified" | "Deleted" | "Renamed" | "Typechange" | "Staged";
interface FileStatus {
  path: string;
  status: FileStatusType;
}
interface RepoStatus {
  working_dir_files: FileStatus[];
  staged_files: FileStatus[];
}
interface CommitInfo {
    id: string;
    message: string;
    author: string;
    date: string;
    parents: string[];
}

// --- Variável Global ---
const REPO_PATH = "C:/Users/vitor.lemes/Downloads/PeitchGIT";

// --- Funções de Renderização ---

function showStatusView() {
    const mainPanel = document.querySelector<HTMLDivElement>(".main-panel")!;
    mainPanel.innerHTML = `
        <h1>Status do Repositório</h1>
        <section>
          <h2>Arquivos Preparados (Staged)</h2>
          <ul id="staged-files-list" class="file-list"></ul>
        </section>
        <section id="commit-section">
            <h2>Commit</h2>
            <textarea id="commit-message" placeholder="Escreva sua mensagem de commit aqui..." rows="4"></textarea>
            <button id="commit-btn" disabled>Commit</button>
        </section>
        <section>
          <h2>Alterações (Changes)</h2>
          <ul id="modified-files-list" class="file-list"></ul>
        </section>
    `;
    fetchAllData();
}

function showDiffView(filePath: string, diff: FileDiff) {
    const mainPanel = document.querySelector<HTMLDivElement>(".main-panel")!;
    const diffLinesHTML = diff.lines.map(line => {
        const lineClass = line.line_type.toLowerCase();
        const escapedContent = line.content.replace(/</g, "&lt;").replace(/>/g, "&gt;");
        return `<div class="diff-line ${lineClass}">${escapedContent}</div>`;
    }).join("");
    mainPanel.innerHTML = `
        <div class="diff-panel">
            <div class="diff-header">
                <button id="back-to-status-btn">&larr; Voltar</button>
                <strong>${filePath}</strong>
            </div>
            <div class="diff-content">${diffLinesHTML}</div>
        </div>
    `;
}

function createFileItemHTML(file: FileStatus, isStaged: boolean): string {
  const statusClass = `status-${file.status.toLowerCase()}`;
  const actionButton = isStaged
    ? `<button class="action-button unstage-btn" data-file="${file.path}">Unstage (-)</button>`
    : `<button class="action-button stage-btn" data-file="${file.path}">Stage (+)</button>`;
  return `
    <li class="file-item">
      <span class="status-badge ${statusClass}">${file.status.charAt(0)}</span>
      <span class="file-path" data-file="${file.path}">${file.path}</span>
      ${actionButton}
    </li>
  `;
}

function renderStatus(status: RepoStatus) {
  const stagedFilesList = document.querySelector<HTMLUListElement>("#staged-files-list")!;
  const modifiedFilesList = document.querySelector<HTMLUListElement>("#modified-files-list")!;
  const commitButton = document.querySelector<HTMLButtonElement>("#commit-btn")!;
  stagedFilesList.innerHTML = status.staged_files.length ? status.staged_files.map(file => createFileItemHTML(file, true)).join("") : "<li>Nenhum arquivo preparado.</li>";
  modifiedFilesList.innerHTML = status.working_dir_files.length ? status.working_dir_files.map(file => createFileItemHTML(file, false)).join("") : "<li>Nenhuma alteração no diretório.</li>";
  commitButton.disabled = status.staged_files.length === 0;
}

function renderHistory(history: CommitInfo[]) {
    const historyList = document.querySelector<HTMLUListElement>("#history-list")!;
    historyList.innerHTML = history.length ? history.map(commit => `<li class="commit-item"><div class="commit-message">${commit.message}</div><div class="commit-details"><strong>${commit.author}</strong> em <em>${commit.date}</em></div><div class="commit-id">${commit.id.substring(0, 7)}</div></li>`).join("") : "<li>Nenhum commit encontrado.</li>";
}

// --- Funções de Lógica e Eventos ---

async function handleMainPanelClick(event: Event) {
    const target = event.target as HTMLElement;
    if (target.matches('#back-to-status-btn')) {
        showStatusView();
        return;
    }
    const filePath = target.dataset.file;
    if (!filePath) return;
    if (target.matches('.stage-btn')) {
        await invoke("stage_file", { repoPath: REPO_PATH, filePath });
        fetchAllData();
    } else if (target.matches('.unstage-btn')) {
        await invoke("unstage_file", { repoPath: REPO_PATH, filePath });
        fetchAllData();
    } else if (target.matches('.file-path')) {
        try {
            const diff: FileDiff = await invoke("get_file_diff", { repoPath: REPO_PATH, filePath });
            showDiffView(filePath, diff);
        } catch (error) {
            console.error(`Erro ao buscar diff para ${filePath}:`, error);
        }
    }
}

async function handleCommit() {
    const commitMessageInput = document.querySelector<HTMLTextAreaElement>("#commit-message")!;
    const message = commitMessageInput.value.trim();
    if (!message) {
        alert("Por favor, insira uma mensagem de commit.");
        return;
    }
    try {
        await invoke("commit_files", { repoPath: REPO_PATH, message });
        alert("Commit criado com sucesso!");
        commitMessageInput.value = "";
        fetchAllData();
    } catch (error) {
        console.error("Erro ao criar commit:", error);
        alert(`Erro ao criar commit: ${error}`);
    }
}

async function fetchAllData() {
  try {
    const [status, history] = await Promise.all([
        invoke<RepoStatus>("get_git_status", { repoPath: REPO_PATH }),
        invoke<CommitInfo[]>("get_commit_history", { repoPath: REPO_PATH })
    ]);
    // Apenas renderiza o status se a view de status estiver ativa
    if(document.querySelector("#staged-files-list")) {
        renderStatus(status);
    }
    renderHistory(history);
  } catch (error) {
    console.error("Erro ao buscar dados do repositório:", error);
    document.querySelector<HTMLDivElement>("#app")!.innerHTML = `<h1>Erro</h1><p>${error}</p>`;
  }
}

// --- Inicialização da Aplicação ---
document.addEventListener("DOMContentLoaded", async () => {
  const appDiv = document.querySelector<HTMLDivElement>("#app")!;
  appDiv.innerHTML = `
    <div class="container">
        <div class="main-panel"></div>
        <div class="side-panel">
            <h2>Histórico</h2>
            <ul id="history-list"></ul>
        </div>
    </div>
  `;
  
  // Delegação de eventos principal
  document.querySelector('.container')?.addEventListener('click', (event) => {
      const target = event.target as HTMLElement;
      if (target.matches('#commit-btn')) {
          handleCommit();
      } else if (target.closest('.main-panel')) {
          handleMainPanelClick(event);
      }
  });

  // Listener para o vigia de arquivos
  await listen('file-changed', () => {
      console.log('Evento "file-changed" recebido do Rust!');
      // Apenas busca os dados se a view de status estiver visível
      if(document.querySelector("#staged-files-list")) {
        fetchAllData();
      }
  });

  showStatusView()
});