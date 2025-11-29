fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/function_primitive_lang.cpp");

    build.include("native");
    if let Ok(path) = std::env::var("DEP_FUNCTION_INCLUDE") {
        build.include(path);
    }

    build.compile("function_primitive_lang");
    println!("cargo:include=native");
}
