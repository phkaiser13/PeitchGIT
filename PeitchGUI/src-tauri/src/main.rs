warning: unused import: `Manager`
 --> src\main.rs:7:13
  |
7 | use tauri::{Manager, AppHandle};
  |             ^^^^^^^
  |
  = note: `#[warn(unused_imports)]` on by default

warning: unused import: `Debouncer`
 --> src\main.rs:8:65
  |
8 | use notify_debouncer_full::{new_debouncer, DebounceEventResult, Debouncer, notify::RecursiveMode};
  |                                                                 ^^^^^^^^^

warning: unused import: `Diff`
 --> src\git_commands.rs:2:60
  |
2 | use git2::{Repository, ObjectType, Signature, DiffOptions, Diff};
  |                                                            ^^^^

error[E0599]: no method named `emit_all` found for struct `AppHandle` in the current scope
  --> src\main.rs:23:32
   |
23 |                     app_handle.emit_all("file-changed", ()).unwrap();
   |                                ^^^^^^^^
   |
help: there is a method `emit` with a similar name
   |
23 -                     app_handle.emit_all("file-changed", ()).unwrap();
23 +                     app_handle.emit("file-changed", ()).unwrap();
   |

error[E0599]: no method named `watch` found for mutable reference `&mut notify_debouncer_full::notify::ReadDirectoryChangesWatcher` in the current scope
   --> src\main.rs:30:29
    |
30  |         debouncer.watcher().watch(&repo_path, RecursiveMode::Recursive).unwrap();
    |                             ^^^^^
    |
    = help: items from traits can only be used if the trait is in scope
help: there is a method `unwatch` with a similar name, but with different arguments
   --> C:\Users\vitor\.cargo\registry\src\index.crates.io-1949cf8c6b5b557f\notify-6.1.1\src\lib.rs:348:5
    |
348 |     fn unwatch(&mut self, path: &Path) -> Result<()>;
    |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
help: trait `Watcher` which provides `watch` is implemented but not in scope; perhaps you want to import it
    |
5   + use notify_debouncer_full::notify::Watcher;
    |

error[E0599]: no method named `watch` found for mutable reference `&mut notify_debouncer_full::notify::ReadDirectoryChangesWatcher` in the current scope
   --> src\main.rs:34:29
    |
34  |         debouncer.watcher().watch(&git_dir, RecursiveMode::Recursive).unwrap();
    |                             ^^^^^
    |
    = help: items from traits can only be used if the trait is in scope
help: there is a method `unwatch` with a similar name, but with different arguments
   --> C:\Users\vitor\.cargo\registry\src\index.crates.io-1949cf8c6b5b557f\notify-6.1.1\src\lib.rs:348:5
    |
348 |     fn unwatch(&mut self, path: &Path) -> Result<()>;
    |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
help: trait `Watcher` which provides `watch` is implemented but not in scope; perhaps you want to import it
    |
5   + use notify_debouncer_full::notify::Watcher;
    |

For more information about this error, try `rustc --explain E0599`.
warning: `peitch-gui` (bin "peitch-gui") generated 3 warnings
error: could not compile `peitch-gui` (bin "peitch-gui") due to 3 previous errors; 3 warnings emitted
