#![allow(non_snake_case)]
#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
#![allow(unused_imports)]
use crate::*;
pub use crate::cppmmabi::*;
use std::os::raw::*;

#[repr(C)]
pub struct DN__v2_2__Class_t {
    _unused: [u8; 0],
}
#[repr(C, align(4))]
#[derive(Clone)]
pub struct DN__v2_2__Class__Struct_t {
    pub a: c_int,
    pub b: c_float,
}


#[repr(transparent)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct DN__v2_2__Class__Enum_e(pub u32);
pub const DN_Class_Enum_One: DN__v2_2__Class__Enum_e = DN__v2_2__Class__Enum_e(0);
pub const DN_Class_Enum_Two: DN__v2_2__Class__Enum_e = DN__v2_2__Class__Enum_e(1);
pub const DN_Class_Enum_Three: DN__v2_2__Class__Enum_e = DN__v2_2__Class__Enum_e(2);


extern "C" {

pub fn DN__v2_2__Class__Struct_structMethod(this_: *mut DN_Class_Struct_t) -> Exception;

pub fn DN_v2_2_someFunction() -> Exception;


} // extern "C"
