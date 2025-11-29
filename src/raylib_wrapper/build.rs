fn main() {
    if let Ok(path) = std::env::var("RAYLIB_INCLUDE_DIR") {
        println!("cargo:include={path}");
    }
}
