use std::env;
use std::path::Path;

fn main() {
    let dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    println!(
        "cargo:rustc-link-search={}",
        Path::new(&dir)
            .parent()
            .unwrap()
            .join("lib")
            .join("build")
            .display()
    );
}
