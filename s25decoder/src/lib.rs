#![allow(non_snake_case)]

use s25::{S25Archive, S25Image};
use std::ffi::CStr;

unsafe fn s25_archive_open(path: *const u8) -> Option<S25Archive> {
    let path = CStr::from_ptr(path as *const _);
    let path = path.to_str().ok()?;
    S25Archive::open(path).ok()
}

// archive

#[no_mangle]
pub unsafe extern "C" fn S25ArchiveOpen(path: *const u8) -> *mut S25Archive {
    s25_archive_open(path)
        .map(|s25| Box::leak(Box::new(s25)) as *mut _)
        .unwrap_or_else(|| std::ptr::null_mut::<S25Archive>())
}

#[no_mangle]
pub unsafe extern "C" fn S25ArchiveRelease(archive: *mut S25Archive) {
    if archive.is_null() {
        return;
    }

    drop(Box::from_raw(archive));
}

#[no_mangle]
pub unsafe extern "C" fn S25ArchiveLoadImage(
    archive: *mut S25Archive,
    entry: usize,
) -> *mut S25Image {
    let archive = &mut *archive;
    archive
        .load_image(entry)
        .map(|s25| Box::leak(Box::new(s25)) as *mut _)
        .unwrap_or_else(|_| std::ptr::null_mut::<S25Image>())
}

#[no_mangle]
pub unsafe extern "C" fn S25ArchiveGetTotalEntries(
    archive: *const S25Archive,
) -> usize {
    let archive = &*archive;
    archive.total_entries()
}

// image

#[no_mangle]
pub unsafe extern "C" fn S25ImageRelease(image: *mut S25Image) {
    if image.is_null() {
        return;
    }

    drop(Box::from_raw(image));
}

#[no_mangle]
pub unsafe extern "C" fn S25ImageGetSize(
    image: *const S25Image,
    width: *mut i32,
    height: *mut i32,
) {
    let image = &*image;
    *width = image.metadata.width;
    *height = image.metadata.height;
}

#[no_mangle]
pub unsafe extern "C" fn S25ImageGetOffset(
    image: *const S25Image,
    x: *mut i32,
    y: *mut i32,
) {
    let image = &*image;
    *x = image.metadata.offset_x;
    *y = image.metadata.offset_y;
}

#[no_mangle]
pub unsafe extern "C" fn S25ImageGetRGBABufferView(
    image: *const S25Image,
    size: *mut usize,
) -> *const u8 {
    let image = &*image;

    if !size.is_null() {
        *size = image.rgba_buffer.len();
    }
    
    image.rgba_buffer.as_ptr()
}
