fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/function.cpp");

    build.include("native");
    if let Ok(path) = std::env::var("DEP_FUNCTION_IR_INCLUDE") {
        build.include(path);
    }
    if let Ok(path) = std::env::var("DEP_TYPESYSTEM_INCLUDE") {
        build.include(path);
    }

    build.compile("function");
    println!("cargo:include=native");
}
