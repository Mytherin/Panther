
use ignore::Walk;

use std::os::raw::c_char;
use std::ffi::CString;
use std::ffi::CStr;
// FIXME: use catch_unwind to unwind panics on FFI boundaries
//use std::panic::catch_unwind;



#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGListFiles(directory: *const c_char, callback: extern fn(*mut c_char, *const c_char), data: *mut c_char) -> i32 {
	// first convert the input data to a normal rust string
	let cstr = unsafe { CStr::from_ptr(directory) };
    let path = match cstr.to_str() {
    	Ok(path) => path,
    	Err(_) => {
    		return -1;
    	}
    };
	for result in Walk::new(path) {
		// walk over the path
	    match result {
	        Ok(entry) => {
	        	// we found a file, we now call the "callback" function with the path
	        	// first do some conversions into C strings
	        	let string_path = match entry.path().to_str() {
	        		Some(result) => result,
	        		None => continue
	        	};
	        	let cstr = match CString::new(string_path) {
	        		Ok(result) => result,
	        		Err(_) => continue
	        	};
	        	// leave ownership of the cstr
	        	let raw_data = cstr.into_raw();
	        	callback(data, raw_data);
	        	// take ownership back
	        	unsafe {
	        		CString::from_raw(raw_data);
	        	}
	        },
	        Err(_) => {}
	    }
	}
	return 0;
}

