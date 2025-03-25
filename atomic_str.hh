#if false

/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2013 President and Fellows of Harvard College
 * Copyright (c) 2012-2013 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */
#ifndef ATOMIC_STR_HH
#define ATOMIC_STR_HH
#include "string_base.hh"
#include <stdarg.h>
#include <stdio.h>
namespace lcdf {

struct atomic_Str : public String_base<atomic_Str, relaxed_atomic<char>> {
    // typedef atomic_Str substring_type;
    // typedef atomic_Str argument_type;

    const relaxed_atomic<char> *s;
    int len;

    atomic_Str()
        : s(0), len(0) {
    }
    template <typename T>
    // Str(const String_base<T>& x)
    //     : s(x.data()), len(x.length()) {
    // }
    // Str(const char* s_)
    //     : s(s_), len(strlen(s_)) {
    // }
    // Str(const char* s_, int len_)
    //     : s(s_), len(len_) {
    // }
    // Str(const unsigned char* s_, int len_)
    //     : s(reinterpret_cast<const char*>(s_)), len(len_) {
    // }
    // Str(const char *first, const char *last)
    //     : s(first), len(last - first) {
    //     precondition(first <= last);
    // }
    // Str(const unsigned char *first, const unsigned char *last)
    //     : s(reinterpret_cast<const char*>(first)), len(last - first) {
    //     precondition(first <= last);
    // }
    // Str(const std::string& str)
    //     : s(str.data()), len(str.length()) {
    // }
    // Str(const uninitialized_type &unused) {
    //     (void) unused;
    // }

    atomic_Str(const relaxed_atomic<char>* s_, int len_)
        : s(s_), len(len_) {
    }
    atomic_Str(const relaxed_atomic<unsigned char>* s_, int len_)
        : s(reinterpret_cast<const relaxed_atomic<char>*>(s_)), len(len_) {
    }
    atomic_Str(const relaxed_atomic<char> *first, const relaxed_atomic<char> *last)
        : s(first), len(last - first) {
        precondition(first <= last);
    }
    atomic_Str(const relaxed_atomic<unsigned char> *first, const relaxed_atomic<unsigned char> *last)
        : s(reinterpret_cast<const relaxed_atomic<char>*>(first)), len(last - first) {
        precondition(first <= last);
    }

    void assign() {
        s = 0;
        len = 0;
    }
    // template <typename T>
    // void assign(const String_base<T> &x) {
    //     s = x.data();
    //     len = x.length();
    // }
    // void assign(const char *s_) {
    //     s = s_;
    //     len = strlen(s_);
    // }
    // void assign(const char *s_, int len_) {
    //     s = s_;
    //     len = len_;
    // }
    void assign(relaxed_atomic<char>* s_, int len_) {
        s = s_;
        len = len_;
    }

    const relaxed_atomic<char> *data() const {
        return s;
    }
    int length() const {
        return len;
    }
    relaxed_atomic<char>* mutable_data() {
        return const_cast<relaxed_atomic<char>*>(s);
    }

    atomic_Str prefix(int lenx) const {
        return atomic_Str(s, lenx < len ? lenx : len);
    }
    atomic_Str substring(const relaxed_atomic<char> *first, const relaxed_atomic<char> *last) const {
        if (first <= last && first >= s && last <= s + len)
            return atomic_Str(first, last);
        else
            return atomic_Str();
    }
    atomic_Str substring(const relaxed_atomic<unsigned char> *first, const relaxed_atomic<unsigned char> *last) const {
        const relaxed_atomic<unsigned char> *u = reinterpret_cast<const relaxed_atomic<unsigned char>*>(s);
        if (first <= last && first >= u && last <= u + len)
            return atomic_Str(first, last);
        else
            return atomic_Str();
    }
    // atomic_Str fast_substring(const relaxed_atomic<char> *first, const relaxed_atomic<char> *last) const {
    //     assert(begin() <= first && first <= last && last <= end());
    //     return atomic_Str(first, last);
    // }
    // atomic_Str fast_substring(const unsigned char *first, const unsigned char *last) const {
    //     assert(ubegin() <= first && first <= last && last <= uend());
    //     return atomic_Str(first, last);
    // }
    // Str ltrim() const {
    //     return String_generic::ltrim(*this);
    // }
    // Str rtrim() const {
    //     return String_generic::rtrim(*this);
    // }
    // Str trim() const {
    //     return String_generic::trim(*this);
    // }

    long to_i() const {         // XXX does not handle negative
        long x = 0;
        int p;
        for (p = 0; p < len && s[p] >= '0' && s[p] <= '9'; ++p)
            x = (x * 10) + s[p] - '0';
        return p == len && p != 0 ? x : -1;
    }

    // static Str snprintf(char *buf, size_t size, const char *fmt, ...) {
    //     va_list val;
    //     va_start(val, fmt);
    //     int n = vsnprintf(buf, size, fmt, val);
    //     va_end(val);
    //     return Str(buf, n);
    // }
};

} // namespace lcdf

#endif

#endif