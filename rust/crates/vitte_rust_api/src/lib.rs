#![allow(non_camel_case_types)]
use std::panic::{self, AssertUnwindSafe};

use vitte_common::err::VitteErr;

pub const VR_ABI_VERSION: u32 = 1;

#[repr(C)]
pub struct vr_buf {
  pub ptr: *mut u8,
  pub len: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum vr_err {
  VR_OK = 0,
  VR_EINVAL = 1,
  VR_EPARSE = 2,
  VR_EOOM = 3,
  VR_EINTERNAL = 100,
  VR_EABI_MISMATCH = 101,
}

fn map_err(e: VitteErr) -> vr_err {
  match e {
    VitteErr::InvalidArg => vr_err::VR_EINVAL,
    VitteErr::Parse => vr_err::VR_EPARSE,
    VitteErr::Oom => vr_err::VR_EOOM,
    VitteErr::Internal => vr_err::VR_EINTERNAL,
    VitteErr::AbiMismatch => vr_err::VR_EABI_MISMATCH,
  }
}

fn leak_vec(mut v: Vec<u8>) -> vr_buf {
  if v.is_empty() {
    return vr_buf { ptr: std::ptr::null_mut(), len: 0 };
  }
  let b = v.into_boxed_slice();
  let len = b.len();
  let ptr = Box::into_raw(b) as *mut u8;
  vr_buf { ptr, len }
}

#[no_mangle]
pub extern "C" fn vr_abi_version() -> u32 {
  VR_ABI_VERSION
}

#[no_mangle]
pub extern "C" fn vr_buf_free(b: vr_buf) {
  if b.ptr.is_null() || b.len == 0 { return; }
  unsafe {
    let slice = std::slice::from_raw_parts_mut(b.ptr, b.len);
    let _ = Box::from_raw(slice);
  }
}

#[no_mangle]
pub extern "C" fn vr_muf_to_json(data: *const u8, len: usize, out_json: *mut vr_buf) -> vr_err {
  if out_json.is_null() { return vr_err::VR_EINVAL; }
  if len != 0 && data.is_null() { return vr_err::VR_EINVAL; }

  let res = panic::catch_unwind(AssertUnwindSafe(|| {
    let bytes = unsafe { std::slice::from_raw_parts(data, len) };
    vitte_muf::muf_to_json_bytes(bytes).map_err(map_err).map(leak_vec)
  }));

  match res {
    Ok(Ok(buf)) => {
      unsafe { *out_json = buf; }
      vr_err::VR_OK
    }
    Ok(Err(e)) => e,
    Err(_) => vr_err::VR_EINTERNAL,
  }
}

#[no_mangle]
pub extern "C" fn vr_utf8_validate(data: *const u8, len: usize) -> i32 {
  if len != 0 && data.is_null() { return 0; }
  let res = panic::catch_unwind(AssertUnwindSafe(|| {
    let bytes = unsafe { std::slice::from_raw_parts(data, len) };
    vitte_unicode::utf8_validate(bytes)
  }));
  match res {
    Ok(ok) => if ok { 1 } else { 0 },
    Err(_) => 0,
  }
}

#[no_mangle]
pub extern "C" fn vr_regex_is_match(hay: *const u8, hay_len: usize,
                                    pat: *const u8, pat_len: usize) -> i32 {
  if hay_len != 0 && hay.is_null() { return 0; }
  if pat_len != 0 && pat.is_null() { return 0; }
  let res = panic::catch_unwind(AssertUnwindSafe(|| {
    let h = unsafe { std::slice::from_raw_parts(hay, hay_len) };
    let p = unsafe { std::slice::from_raw_parts(pat, pat_len) };
    vitte_regex::is_match(h, p)
  }));
  match res {
    Ok(ok) => if ok { 1 } else { 0 },
    Err(_) => 0,
  }
}
