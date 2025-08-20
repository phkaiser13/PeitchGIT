use serde::{Serialize, Deserialize};
use git2::{Repository, ObjectType, Signature, DiffOptions}; // CORREÇÃO: Removi o "Diff" daqui.
use std::path::Path;
use tauri::command;
use chrono::prelude::*;


// --- Estruturas de Dados (sem alterações) ---

#[derive(Serialize, Deserialize, Debug, Clone)]
pub enum DiffLineType {
    Context,
    Addition,
    Deletion,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct DiffLine {
    line_type: DiffLineType,
    content: String,
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub struct FileDiff {
    lines: Vec<DiffLine>,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
pub enum FileStatusType {
    New,
    Modified,
    Deleted,
    Renamed,
    Typechange,
    Staged,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct FileStatus {
    path: String,
    status: FileStatusType,
}

#[derive(Serialize, Deserialize, Debug, Default)]
pub struct RepoStatus {
    working_dir_files: Vec<FileStatus>,
    staged_files: Vec<FileStatus>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CommitInfo {
    id: String,
    message: String,
    author: String,
    date: String,
}

// --- Comandos Tauri ---

#[command]
pub fn get_file_diff(repo_path: &str, file_path: &str) -> Result<FileDiff, String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;

    // Pega a árvore do último commit (HEAD) para comparar
    let head_commit = repo.head().and_then(|head| head.peel_to_commit()).ok();
    let old_tree = head_commit.as_ref().and_then(|commit| commit.tree().ok());

    // Cria o diff entre a árvore do HEAD e o diretório de trabalho atual,
    // mas limitado ao arquivo que nos interessa.
    let mut opts = DiffOptions::new();
    opts.pathspec(file_path);

    let diff = repo.diff_tree_to_workdir_with_index(old_tree.as_ref(), Some(&mut opts))
        .map_err(|e| format!("Falha ao criar o diff: {}", e))?;

    let mut file_diff = FileDiff::default();

    // Itera sobre o diff e formata a saída para o frontend
    diff.foreach(
        &mut |_, _| true, // file_cb
        None, // binary_cb
        None, // hunk_cb
        Some(&mut |_, _, line| { // line_cb
            let line_type = match line.origin() {
                '+' => DiffLineType::Addition,
                '-' => DiffLineType::Deletion,
                _   => DiffLineType::Context,
            };
            // Converte o conteúdo da linha de bytes para String, ignorando caracteres inválidos
            let content = String::from_utf8_lossy(line.content()).to_string();

            file_diff.lines.push(DiffLine { line_type, content });
            true
        }),
    ).map_err(|e| format!("Falha ao processar as linhas do diff: {}", e))?;

    Ok(file_diff)
}

#[command]
pub fn get_git_status(repo_path: &str) -> Result<RepoStatus, String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;

    let statuses = repo.statuses(None)
        .map_err(|e| format!("Falha ao ler o status dos arquivos: {}", e))?;

    let mut repo_status = RepoStatus::default();

    for entry in statuses.iter() {
        if let Some(path_str) = entry.path() {
            let path = path_str.to_string();
            let git_status = entry.status();

            if git_status.is_index_new() || git_status.is_index_modified() || git_status.is_index_deleted()
                || git_status.is_index_renamed() || git_status.is_index_typechange() {
                repo_status.staged_files.push(FileStatus {
                    path: path.clone(),
                    status: FileStatusType::Staged,
                });
            }

            if git_status.is_wt_new() {
                repo_status.working_dir_files.push(FileStatus { path, status: FileStatusType::New });
            } else if git_status.is_wt_modified() {
                repo_status.working_dir_files.push(FileStatus { path, status: FileStatusType::Modified });
            } else if git_status.is_wt_deleted() {
                repo_status.working_dir_files.push(FileStatus { path, status: FileStatusType::Deleted });
            } else if git_status.is_wt_renamed() {
                repo_status.working_dir_files.push(FileStatus { path, status: FileStatusType::Renamed });
            } else if git_status.is_wt_typechange() {
                repo_status.working_dir_files.push(FileStatus { path, status: FileStatusType::Typechange });
            }
        }
    }
    Ok(repo_status)
}

#[command]
pub fn stage_file(repo_path: &str, file_path: &str) -> Result<(), String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;
    let mut index = repo.index()
        .map_err(|e| format!("Falha ao pegar o index: {}", e))?;
    index.add_path(Path::new(file_path))
        .map_err(|e| format!("Falha ao adicionar o arquivo '{}': {}", file_path, e))?;
    index.write()
        .map_err(|e| format!("Falha ao salvar o index: {}", e))?;
    Ok(())
}

#[command]
pub fn unstage_file(repo_path: &str, file_path: &str) -> Result<(), String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;
    let head = repo.head()
        .map_err(|e| format!("Falha ao encontrar HEAD: {}", e))?
        .peel(ObjectType::Commit)
        .map_err(|e| format!("Falha ao 'descascar' para commit: {}", e))?;
    repo.reset_default(Some(&head), &[Path::new(file_path)])
        .map_err(|e| format!("Falha ao fazer unstage do arquivo '{}': {}", file_path, e))?;
    Ok(())
}

#[command]
pub fn commit_files(repo_path: &str, message: &str) -> Result<(), String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;
    let signature = Signature::now("PeitchGit User", "user@peitchgit.com")
        .map_err(|e| format!("Falha ao criar assinatura: {}", e))?;
    let mut index = repo.index().map_err(|e| format!("Falha ao pegar o index: {}", e))?;
    let oid = index.write_tree().map_err(|e| format!("Falha ao escrever a árvore do index: {}", e))?;
    let tree = repo.find_tree(oid).map_err(|e| format!("Falha ao encontrar a árvore: {}", e))?;
    let parent_commit = repo.head()
        .map_err(|_| "Repositório sem commits iniciais.".to_string())?
        .peel_to_commit()
        .map_err(|e| format!("Falha ao encontrar o commit pai: {}", e))?;
    repo.commit(
        Some("HEAD"),
        &signature,
        &signature,
        message,
        &tree,
        &[&parent_commit],
    ).map_err(|e| format!("Falha ao criar o commit: {}", e))?;
    Ok(())
}

#[command]
pub fn get_commit_history(repo_path: &str) -> Result<Vec<CommitInfo>, String> {
    let repo = Repository::open(Path::new(repo_path))
        .map_err(|e| format!("Falha ao abrir o repositório: {}", e))?;

    let mut revwalk = repo.revwalk()
        .map_err(|e| format!("Falha ao criar o 'revwalk': {}", e))?;

    revwalk.push_head()
        .map_err(|e| format!("Falha ao encontrar o 'HEAD': {}", e))?;

    let mut history = Vec::new();

    for oid in revwalk {
        let oid = oid.map_err(|e| format!("Erro ao iterar no histórico: {}", e))?;
        let commit = repo.find_commit(oid)
            .map_err(|e| format!("Não foi possível encontrar o commit '{}': {}", oid, e))?;

        let timestamp = commit.time().seconds();
        
        // <-- CORREÇÃO 2: Usando a função recomendada e mais moderna para data/hora.
        let datetime = DateTime::from_timestamp(timestamp, 0).unwrap_or_default();
        let date_str = datetime.format("%d/%m/%Y %H:%M").to_string();

        let commit_info = CommitInfo {
            id: oid.to_string(),
            message: commit.message().unwrap_or("").to_string(),
            author: commit.author().name().unwrap_or("").to_string(),
            date: date_str,
        };

        history.push(commit_info);

        if history.len() >= 25 {
            break;
        }
    }
    Ok(history)
}