use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let exe = out_dir.join("function_visualizer_cxx");

    let mut cmd = Command::new("g++");
    cmd.arg("native/function_visualizer.cpp");
    cmd.arg("-std=c++23");
    cmd.arg("-o").arg(&exe);
    cmd.arg("-Inative");

    if let Ok(p) = env::var("DEP_RLIMGUI_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_IMGUI_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_FUNCTION_ALU_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_FUNCTION_COMPOUND_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_FUNCTION_IR_FILE_REPOSITORY_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_FUNCTION_REPOSITORY_INCLUDE") { cmd.arg(format!("-I{}", p)); }
    if let Ok(p) = env::var("DEP_FUNCTION_VISUALIZER_EDITOR_INCLUDE") { cmd.arg(format!("-I{}", p)); }

    cmd.arg("-lrlImGui");
    cmd.arg("-limgui");
    cmd.arg("-lfunction_alu");
    cmd.arg("-lfunction_compound");
    cmd.arg("-lfunction_ir_file_repository");
    cmd.arg("-lfunction_repository");
    cmd.arg("-lfunction_visualizer_editor");

    cmd.arg("-lraylib");
    cmd.arg("-lglfw");
    cmd.arg("-lGL");
    cmd.arg("-ldl");
    cmd.arg("-lpthread");

    let status = cmd.status().expect("g++ failed");
    assert!(status.success());

    println!("cargo:rustc-env=CXX_BIN={}", exe.to_string_lossy());
}
