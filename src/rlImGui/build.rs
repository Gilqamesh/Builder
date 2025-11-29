fn main() {
    let mut b = cc::Build::new();
    b.cpp(true);
    b.flag("-std=c++23");

    b.file("native/rlImGui.cpp");
    b.include("native");
    b.include("native/extras");

    if let Ok(p) = std::env::var("DEP_IMGUI_INCLUDE") {
        b.include(p);
    }
    if let Ok(p) = std::env::var("DEP_RAYLIB_WRAPPER_INCLUDE") {
        b.include(p);
    }

    b.compile("rlImGui");
    println!("cargo:include=native");
}
