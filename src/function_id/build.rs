fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/function_id.cpp");

    build.include("native");

    build.compile("function_id");
    println!("cargo:include=native");
}
