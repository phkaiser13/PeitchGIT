#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::path::PathBuf;
use std::time::Duration;
use tauri::{AppHandle, Emitter};
use notify_debouncer_full::{new_debouncer, DebounceEventResult, notify::{RecursiveMode, Watcher}};

mod git_commands;

fn start_file_watcher(app_handle: AppHandle, repo_path: PathBuf) {
    std::thread::spawn(move || {
        let mut debouncer = new_debouncer(Duration::from_secs(2), None, move |res: DebounceEventResult| {
            match res {
                Ok(_) => {
                    println!("Mudança detectada no sistema de arquivos, emitindo evento.");
                    // Agora que Emitter está no escopo, este comando funcionará.
                    app_handle.emit("file-changed", ()).unwrap();
                }
                Err(e) => println!("Erro no watcher: {:?}", e),
            }
        }).unwrap();

        debouncer.watcher().watch(&repo_path, RecursiveMode::Recursive).unwrap();
        
        let git_dir = repo_path.join(".git");
        debouncer.watcher().watch(&git_dir, RecursiveMode::Recursive).unwrap();

        loop {
            std::thread::sleep(Duration::from_secs(1));
        }
    });
}

fn main() {
    tauri::Builder::default()
        .setup(|app| {
            let repo_path_str = "C:/Users/vitor.lemes/Downloads/PeitchGIT"; 
            start_file_watcher(app.handle().clone(), PathBuf::from(repo_path_str));
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            git_commands::get_git_status,
            git_commands::stage_file,
            git_commands::unstage_file,
            git_commands::commit_files,
            git_commands::get_commit_history,
            git_commands::get_file_diff,
            git_commands::get_branches,
            git_commands::checkout_branch
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}