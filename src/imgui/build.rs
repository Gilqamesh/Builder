fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    let files = [
        "native/imgui.cpp",
        "native/imgui_demo.cpp",
        "native/imgui_draw.cpp",
        "native/imgui_tables.cpp",
        "native/imgui_widgets.cpp",
        "native/backends/imgui_impl_glfw.cpp",
        "native/backends/imgui_impl_opengl3.cpp",
    ];
    for f in files { build.file(f); }

    build.include("native");
    build.include("native/backends");
    build.include("native/misc");

    build.compile("imgui");
    println!("cargo:include=native");
}
