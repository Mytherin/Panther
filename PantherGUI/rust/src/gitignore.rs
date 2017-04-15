
use ignore::WalkBuilder;

use std::os::raw::c_char;
use std::ffi::CString;
use std::ffi::CStr;
// FIXME: use catch_unwind to unwind panics on FFI boundaries
//use std::panic::catch_unwind;


fn list_files(directory: *const c_char, callback: extern fn(*mut c_char, *const c_char, bool), data: *mut c_char, relative_paths: bool, list_all_files: bool) -> i32 {
	// first convert the input data to a normal rust string
	let cstr = unsafe { CStr::from_ptr(directory) };
	let path = match cstr.to_str() {
		Ok(path) => path,
		Err(_) => {
			return -1;
		}
	};

	let mut walkbuilder = WalkBuilder::new(path);
	if !list_all_files {
		walkbuilder.max_depth(Some(1));
	}

	for result in walkbuilder.build() {
		// walk over the path
		match result {
			Ok(entry) => {
				if entry.depth() == 0 {
					continue;
				}
				// we found a file, we now call the "callback" function with the path
				// first do some conversions into C strings
				let is_directory = match entry.file_type() {
					Some(tpe) => tpe.is_dir(),
					None => false
				};
				let string_path;
				if relative_paths {
					string_path = match entry.file_name().to_str() {
						Some(result) => result,
						None => continue
					};
				} else {
					string_path = match entry.path().to_str() {
						Some(result) => result,
						None => continue
					};	
				}
				let cstr = match CString::new(string_path) {
					Ok(result) => result,
					Err(_) => continue
				};

				// leave ownership of the cstr
				let raw_data = cstr.into_raw();
				callback(data, raw_data, is_directory);
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


#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGListAllFiles(directory: *const c_char, callback: extern fn(*mut c_char, *const c_char, bool), data: *mut c_char, relative_paths: bool) -> i32 {
	return list_files(directory, callback, data, relative_paths, true);
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGListFiles(directory: *const c_char, callback: extern fn(*mut c_char, *const c_char, bool), data: *mut c_char, relative_paths: bool) -> i32 {
	return list_files(directory, callback, data, relative_paths, false);
}


