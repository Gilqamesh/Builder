use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let exe = out_dir.join("typesystem_call_test_cxx");

    let mut cmd = Command::new("g++");
    cmd.arg("native/typesystem_call_test.cpp");
    cmd.arg("-std=c++23");
    cmd.arg("-o").arg(&exe);
    cmd.arg("-Inative");

    if let Ok(p) = env::var("DEP_TYPESYSTEM_CALL_INCLUDE") {
        cmd.arg(format!("-I{}", p));
    }

    cmd.arg("-ltypesystem_call");
    cmd.arg("-lgtest");
    cmd.arg("-lgtest_main");

    let status = cmd.status().expect("g++ failed");
    assert!(status.success());

    println!("cargo:rustc-env=CXX_BIN={}", exe.to_string_lossy());
}
