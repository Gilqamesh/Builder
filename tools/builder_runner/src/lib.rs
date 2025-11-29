use std::process::Command;

pub fn run_embedded_binary() {
    let bin = std::env::var("CXX_BIN").expect("CXX_BIN missing");
    let status = Command::new(&bin).status().expect("spawn failed");
    if !status.success() {
        std::process::exit(status.code().unwrap_or(1));
    }
}
