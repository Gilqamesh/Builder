fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/function_visualizer_editor.cpp");

    build.include("native");
    if let Ok(p) = std::env::var("DEP_IMGUI_INCLUDE") {
        build.include(p);
    }
    if let Ok(p) = std::env::var("DEP_RLIMGUI_INCLUDE") {
        build.include(p);
    }

    build.compile("function_visualizer_editor");
    println!("cargo:include=native");
}
