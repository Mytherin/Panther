
use globset::{Glob,GlobSet,GlobSetBuilder};

use std::os::raw::c_char;
use std::ffi::CStr;
// FIXME: use catch_unwind to unwind panics on FFI boundaries
//use std::panic::catch_unwind;

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGCreateGlobBuilder() -> *mut GlobSetBuilder {
	let mut globset = Box::new(GlobSetBuilder::new());

    let ptr: *mut _ = &mut *globset;

    // forget the object we have created so rust does not clean it up
    ::std::mem::forget(globset);

    return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGGlobBuilderAddGlob(ptr: *mut GlobSetBuilder, glob_text: *mut c_char) -> i32 {
    let cstr = unsafe { CStr::from_ptr(glob_text) };
    let glob = match cstr.to_str() {
    	Ok(glob) => glob,
    	Err(_) => {
    		return -1;
    	}
    };
    let mut glob_builder: Box<GlobSetBuilder> = unsafe { ::std::mem::transmute(ptr) };

	let build_glob = match Glob::new(&glob) {
		Ok(globset) => globset,
		Err(_) => {
			return -1;
		}
	};
	glob_builder.add(build_glob);
	::std::mem::forget(glob_builder);
    return 0;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroyGlobBuilder(ptr: *mut GlobSetBuilder) {
    if ptr.is_null() {
        return;
    }

    // Now, we know the pointer is non-null, we can continue.
    let obj: Box<GlobSetBuilder> = unsafe { ::std::mem::transmute(ptr) };

    // We don't *have* to do anything else; once obj goes out of scope, it will
    // be dropped.  I'm going to drop it explicitly, however, for clarity.
    ::std::mem::drop(obj);
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGCompileGlobBuilder(ptr: *mut GlobSetBuilder) -> *mut GlobSet {
    let glob_builder: Box<GlobSetBuilder> = unsafe { ::std::mem::transmute(ptr) };

	let globset = match glob_builder.build() {
		Ok(globset) => globset,
		Err(_) => {
			return ::std::ptr::null_mut();
		}
	};

	let mut globset = Box::new(globset);

    let ptr: *mut _ = &mut *globset;

    // forget discards its argument (passed by-move), 
    // without triggering its destructor
	::std::mem::forget(glob_builder);
    ::std::mem::forget(globset);
    return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGGlobSetMatches(ptr: *mut GlobSet, glob_text: *mut c_char) -> i32 {
    let cstr = unsafe { CStr::from_ptr(glob_text) };
    let path = match cstr.to_str() {
    	Ok(path) => path,
    	Err(_) => {
    		return -1;
    	}
    };
    let globset: Box<GlobSet> = unsafe { ::std::mem::transmute(ptr) };
    let matches = globset.is_match(path);
    ::std::mem::forget(globset);
    return if matches { 1 } else { 0 };
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroyGlobSet(ptr: *mut GlobSet) {
    if ptr.is_null() {
        return;
    }

    // Now, we know the pointer is non-null, we can continue.
    let obj: Box<GlobSet> = unsafe { ::std::mem::transmute(ptr) };

    // We don't *have* to do anything else; once obj goes out of scope, it will
    // be dropped.  I'm going to drop it explicitly, however, for clarity.
    ::std::mem::drop(obj);
}
