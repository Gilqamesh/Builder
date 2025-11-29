fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/typesystem_call.cpp");

    build.include("native");
    if let Ok(path) = std::env::var("DEP_CALL_INCLUDE") {
        build.include(path);
    }
    if let Ok(path) = std::env::var("DEP_TYPESYSTEM_INCLUDE") {
        build.include(path);
    }

    build.compile("typesystem_call");
    println!("cargo:include=native");
}
