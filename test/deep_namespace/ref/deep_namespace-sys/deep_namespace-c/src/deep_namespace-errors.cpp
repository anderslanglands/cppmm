#include "deep_namespace-errors.h"
#include "deep_namespace-errors-private.h"

thread_local std::string TLG_EXCEPTION_STRING;

DEEP_NAMESPACE_CPPMM_API const char* deep_namespace_get_exception_string() {
    return TLG_EXCEPTION_STRING.c_str();
}

