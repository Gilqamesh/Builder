fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/function_ir.cpp");

    build.include("native");
    if let Ok(path) = std::env::var("DEP_FUNCTION_ID_INCLUDE") {
        build.include(path);
    }

    build.compile("function_ir");
    println!("cargo:include=native");
}
