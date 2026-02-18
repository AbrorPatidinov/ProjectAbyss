use std::ffi::CStr;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn abyss_io_print_str(ptr: *const c_char) {
    if ptr.is_null() {
        return;
    }
    let c_str = unsafe { CStr::from_ptr(ptr) };
    if let Ok(s) = c_str.to_str() {
        println!("{}", s);
    }
}

#[no_mangle]
pub extern "C" fn abyss_io_print_int(val: i64) {
    println!("{}", val);
}

// --- ADDED THIS ---
#[no_mangle]
pub extern "C" fn abyss_io_print_float(val: f64) {
    println!("{}", val);
}

#[no_mangle]
pub extern "C" fn abyss_math_pow(base: i64, exp: i64) -> i64 {
    base.pow(exp as u32)
}

#[no_mangle]
pub extern "C" fn abyss_math_abs(val: i64) -> i64 {
    val.abs()
}
