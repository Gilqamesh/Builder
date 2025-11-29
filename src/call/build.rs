fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.flag("-std=c++23");

    build.file("native/call.cpp");

    build.include("native");

    build.compile("call");
    println!("cargo:include=native");
}
