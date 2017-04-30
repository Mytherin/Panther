
use ignore::gitignore::{Gitignore, GitignoreBuilder};

use std::path::Path;
use std::ptr;
use std::os::raw::c_char;
use std::ffi::CStr;

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGCreateGlobForDirectory(directory: *const c_char) -> *mut Gitignore {
    let cstr = unsafe { CStr::from_ptr(directory) };
	let mut path = match cstr.to_str() {
		Ok(path) => Path::new(path),
		Err(_) => {
			return ptr::null_mut();
		}
	};
    let mut builder = GitignoreBuilder::new(path);
    loop {
    	builder.add(path.join(".gitignore"));
	    path = match path.parent() {
	    	Some(p) => p,
	    	None => break
	    };
    }

    let git_ignore = match builder.build() {
		Ok(ig) => ig,
		Err(_) => {
			return ptr::null_mut();
		}
    };
	let mut ig = Box::new(git_ignore);
	let ptr: *mut _ = &mut *ig;
	// forget the object we have created so rust does not clean it up
	::std::mem::forget(ig);
	return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGFileIsIgnored(ptr: *mut Gitignore, glob_text: *const c_char, is_dir: bool) -> bool {
    if ptr.is_null() {
        return false;
    }
    let cstr = unsafe { CStr::from_ptr(glob_text) };
	let path = match cstr.to_str() {
		Ok(path) => path,
		Err(_) => {
			return false;
		}
	};
	let ig: Box<Gitignore> = unsafe { ::std::mem::transmute(ptr) };
    let m = ig.matched(path, is_dir).is_ignore();
	::std::mem::forget(ig);
	return m;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroyIgnoreGlob(ptr: *mut Gitignore) {
	if ptr.is_null() {
		return;
	}

	// Now, we know the pointer is non-null, we can continue.
	let obj: Box<Gitignore> = unsafe { ::std::mem::transmute(ptr) };

	// We don't *have* to do anything else; once obj goes out of scope, it will
	// be dropped.  I'm going to drop it explicitly, however, for clarity.
	::std::mem::drop(obj);
}
