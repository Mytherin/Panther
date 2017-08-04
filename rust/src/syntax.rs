
use syntect::parsing::ParseState;
use syntect::parsing::SyntaxSet;
use syntect::parsing::SyntaxDefinition;
use syntect::parsing::ScopeStack;
use syntect::parsing::ScopeRepository;

use std::path::Path;
use std::ptr;
use std::os::raw::c_char;
use std::ffi::CStr;

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGLoadSyntaxSet(directory: *const c_char) -> *mut SyntaxSet{
	let cstr = unsafe { CStr::from_ptr(directory) };
	let mut path = match cstr.to_str() {
		Ok(path) => Path::new(path),
		Err(_) => {
			return ptr::null_mut();
		}
	};

	let mut syntax = SyntaxSet::new();
	match syntax.load_syntaxes(path, false) {
		Ok(_) => (),
		Err(_) => {
			return ptr::null_mut();
		}
	};
	syntax.link_syntaxes();

	let mut ig = Box::new(syntax);
	let ptr: *mut _ = &mut *ig;
	// forget the object we have created so rust does not clean it up
	::std::mem::forget(ig);
	return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroySyntaxSet(ptr: *mut SyntaxSet) {
	if ptr.is_null() {
		return;
	}

	let obj: Box<SyntaxSet> = unsafe { ::std::mem::transmute(ptr) };
	::std::mem::drop(obj);
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGCreateParser(cset: *mut SyntaxSet, cname: *const c_char) -> *mut SyntaxDefinition {
	if cset.is_null() || cname.is_null() {
		return ptr::null_mut();
	}

	let cstr = unsafe { CStr::from_ptr(cname) };
	let name = match cstr.to_str() {
		Ok(n) => n,
		Err(_) => {
			return ptr::null_mut();
		}
	};

	let syntax: Box<SyntaxSet> = unsafe { ::std::mem::transmute(cset) };
	let definition = match syntax.find_syntax_by_name(name) {
		Some(def) => def.clone(),
		None => {
			return ptr::null_mut();
		}
	};
	::std::mem::forget(syntax);


	let mut def = Box::new(definition);
	let ptr: *mut _ = &mut *def;
	::std::mem::forget(def);
	return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroyParser(ptr: *mut SyntaxDefinition) {
	if ptr.is_null() {
		return;
	}

	let obj: Box<SyntaxDefinition> = unsafe { ::std::mem::transmute(ptr) };
	::std::mem::drop(obj);
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGGetDefaultState(cdef: *mut SyntaxDefinition) -> *mut ParseState {
	if cdef.is_null() {
		return ptr::null_mut();
	}

	let definition: Box<SyntaxDefinition> = unsafe { ::std::mem::transmute(cdef) };
	let state = ParseState::new(&definition);
	::std::mem::forget(definition);

	let mut ig = Box::new(state);
	let ptr: *mut _ = &mut *ig;
	// forget the object we have created so rust does not clean it up
	::std::mem::forget(ig);
	return ptr;
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGDestroyParserState(ptr: *mut ParseState) {
	if ptr.is_null() {
		return;
	}

	let obj: Box<ParseState> = unsafe { ::std::mem::transmute(ptr) };
	::std::mem::drop(obj);
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn PGCopyParserState(cstate: *mut ParseState) -> *mut ParseState {
	if cstate.is_null() {
		return ptr::null_mut();
	}

	let state: Box<ParseState> = unsafe { ::std::mem::transmute(cstate) };
	let new_state = state.clone();
	::std::mem::forget(state);

	let mut ig = Box::new(new_state);
	let ptr: *mut _ = &mut *ig;
	// forget the object we have created so rust does not clean it up
	::std::mem::forget(ig);
	return ptr;
}
