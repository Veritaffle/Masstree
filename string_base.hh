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
#ifndef STRING_BASE_HH
#define STRING_BASE_HH
#include "compiler.hh"
#include "hashcode.hh"
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <iostream>

//  TODO: not sure these should go here but fine for now
template<typename T, typename U>
int atomic_memcmp(const T* ptr1, const U* ptr2, size_t num) {
    static_assert(sizeof(T) == 1, "T not one byte");
    static_assert(sizeof(U) == 1, "U not one byte");
    static_assert((std::is_same_v<T, relaxed_atomic<char>> ||
                   std::is_same_v<U, relaxed_atomic<char>>));
    
    // debug_fprintf(stdout, "atomic_memcmp()\n");
    for (unsigned i = 0; i < num; ++i) {
        unsigned char ca = static_cast<unsigned char>(ptr1[i]);
        unsigned char cb = static_cast<unsigned char>(ptr2[i]);
        if (ptr1[i] != ptr2[i])
            return ca - cb;
    }
    return 0;
}

template<typename T, typename U>
int maybe_atomic_memcmp(const T* ptr1, const U* ptr2, size_t num) {
    static_assert(sizeof(T) == 1, "T not one byte");
    static_assert(sizeof(U) == 1, "U not one byte");

    if constexpr ((std::is_same_v<T, relaxed_atomic<char>> ||
                   std::is_same_v<U, relaxed_atomic<char>>)) {
        return atomic_memcmp(ptr1, ptr2, num);
    }
    else
        return memcmp(ptr1, ptr2, num);
}

template<typename T, typename U>
void *atomic_memcpy(T *dest, const U* src, size_t count) {
    static_assert((std::is_same_v<T, relaxed_atomic<char>> ||
                   std::is_same_v<U, relaxed_atomic<char>>));
    // debug_fprintf(stdout, "atomic_memcpy()\n");
    for (unsigned i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
    return dest;
}

template<typename T, typename U>
void *maybe_atomic_memcpy(T *dest, const U* src, size_t count) {
    static_assert(sizeof(T) == 1, "T not one byte");
    static_assert(sizeof(U) == 1, "U not one byte");
    
    if constexpr ((std::is_same_v<T, relaxed_atomic<char>> ||
                   std::is_same_v<U, relaxed_atomic<char>>)) {
        return atomic_memcpy(dest, src, count);
    }
    else
        return memcpy(dest, src, count);
}





namespace lcdf {

template <typename T>
struct make_unsigned_like {
    using type = std::make_unsigned_t<T>;
};

// Specialization for std::atomic<T>
template <typename T>
struct make_unsigned_like<relaxed_atomic<T>> {
    using type = relaxed_atomic<std::make_unsigned_t<T>>;
};

template <typename T>
using make_unsigned_like_t = typename make_unsigned_like<T>::type;




class StringAccum;
#define LCDF_CONSTANT_CSTR(cstr) ((cstr) && __builtin_constant_p(strlen((cstr))))
class String_generic_templated_common {
  public:
    static const char empty_data[1];
    static const char bool_data[11]; // "false\0true\0"
    static const char out_of_memory_data[15];
    static const char base64_encoding_table[65];
    static const unsigned char base64_decoding_map[256];
    enum { out_of_memory_length = 14 };
};

template <typename T = char, typename U = char>
class String_generic_templated : public String_generic_templated_common {
  public:
    static bool out_of_memory(const T* s) {
        return unlikely(s >= out_of_memory_data
                        && s <= out_of_memory_data + out_of_memory_length);
    }
    static bool equals(const T* a, int a_len, const U* b, int b_len) {
        return a_len == b_len && maybe_atomic_memcmp(a, b, a_len) == 0;
    }
    static int compare(const T* a, int a_len, const U* b, int b_len);
    static inline int compare(const make_unsigned_like_t<T>* a, int a_len,
                              const make_unsigned_like_t<U>* b, int b_len) {
        return compare(reinterpret_cast<const T*>(a), a_len,
                       reinterpret_cast<const U*>(b), b_len);
    }
    static int natural_compare(const T* a, int a_len, const U* b, int b_len);
    static int natural_compare(const make_unsigned_like_t<T>* a, int a_len,
                               const make_unsigned_like_t<U>* b, int b_len) {
        return natural_compare(reinterpret_cast<const T*>(a), a_len,
                               reinterpret_cast<const U*>(b), b_len);
    }
    static bool starts_with(const T *a, int a_len, const U *b, int b_len) {
        return a_len >= b_len && maybe_atomic_memcmp(a, b, b_len) == 0;
    }
    static int find_left(const T *s, int len, int start, U x);
    static int find_left(const T *s, int len, int start, const U *x, int x_len);
    static int find_right(const T *s, int len, int start, U x);
    static int find_right(const T *s, int len, int start, const U *x, int x_len);
    static bool glob_match(const T* s, int slen, const U* pattern, int plen);
    template <typename V> static inline typename V::substring_type ltrim(const V &str);
    template <typename V> static inline typename V::substring_type rtrim(const V &str);
    template <typename V> static inline typename V::substring_type trim(const V &str);
    static hashcode_t hashcode(const T *s, int len);
    static hashcode_t hashcode(const T *first, const T *last) {
        return hashcode(first, last - first);
    }
    static long to_i(const T* first, const T* last);
    //  TODO: not templated yet
    static char upper_hex_nibble(int n) {
        return n + (n > 9 ? 'A' - 10 : '0');
    }
};

using String_generic = String_generic_templated<>;

template <typename T, typename C = char>
class String_base_templated {
  public:
    typedef T type;
    typedef const C* const_iterator;
    typedef const_iterator iterator;
    typedef const make_unsigned_like_t<C>* const_unsigned_iterator;
    typedef const_unsigned_iterator unsigned_iterator;
    typedef int (String_base_templated<T,C>::*unspecified_bool_type)() const;

    const C* data() const {
        return static_cast<const T*>(this)->data();
    }
    int length() const {
        return static_cast<const T*>(this)->length();
    }
    int size() const {
        return static_cast<const T*>(this)->length();
    }

    /** @brief Return a pointer to the string's data as unsigned chars.

        Only the first length() characters are valid, and the string data
        might not be null-terminated. @sa data() */
    const unsigned char* udata() const {
        return reinterpret_cast<const unsigned char*>(data());
    }
    /** @brief Return an iterator for the beginning of the string.

        String iterators are simply pointers into string data, so they are
        quite efficient. @sa String::data */
    const_iterator begin() const {
        return data();
    }
    /** @brief Return an iterator for the end of the string.

        The result points one character beyond the last character in the
        string. */
    const_iterator end() const {
        return data() + length();
    }
    /** @brief Return an unsigned iterator for the beginning of the string.

        This is equivalent to reinterpret_cast<const unsigned char
        *>(begin()). */
    const_unsigned_iterator ubegin() const {
        return reinterpret_cast<const_unsigned_iterator>(data());
    }
    /** @brief Return an unsigned iterator for the end of the string.

        This is equivalent to reinterpret_cast<const unsigned char
        *>(end()). */
    const_unsigned_iterator uend() const {
        return reinterpret_cast<const_unsigned_iterator>(data() + length());
    }
    /** @brief Test if the string is nonempty. */
    operator unspecified_bool_type() const {
        return length() ? &String_base_templated<T,C>::length : 0;
    }
    /** @brief Test if the string is empty. */
    bool operator!() const {
        return length() == 0;
    }
    /** @brief Test if the string is empty. */
    bool empty() const {
        return length() == 0;
    }
    /** @brief Test if the string is an out-of-memory string. */
    bool out_of_memory() const {
        return String_generic_templated<C>::out_of_memory(data());
    }
    /** @brief Return the @a i th character in the string.

        Does not check bounds. @sa at() */
    const char& operator[](int i) const {
        return data()[i];
    }
    /** @brief Return the @a i th character in the string.

        Checks bounds: an assertion will fail if @a i is less than 0 or not
        less than length(). @sa operator[] */
    const char& at(int i) const {
        assert(unsigned(i) < unsigned(length()));
        return data()[i];
    }
    /** @brief Return the first character in the string.

        Does not check bounds. Same as (*this)[0]. */
    const char& front() const {
        return data()[0];
    }
    /** @brief Return the last character in the string.

        Does not check bounds. Same as (*this)[length() - 1]. */
    const char& back() const {
        return data()[length() - 1];
    }


    //  TODO: remove duplicates here

    /** @brief Test if this string is equal to the C string @a cstr. */
    bool equals(const char *cstr) const {
        return String_generic_templated<C,char>::equals(data(), length(), cstr, strlen(cstr));
    }
    /** @brief Test if this string is equal to the first @a len characters
        of @a s. */
    bool equals(const char *s, int len) const {
        return String_generic_templated<C,char>::equals(data(), length(), s, len);
    }
    /** @brief Test if this string is equal to @a x. */
    template <typename TT, typename CC>
    bool equals(const String_base_templated<TT,CC>& x) const {
        return String_generic_templated<C,CC>::equals(data(), length(), x.data(), x.length());
    }
    /** @brief Compare this string with the C string @a cstr.

        Returns 0 if this string equals @a cstr, negative if this string is
        less than @a cstr in lexicographic order, and positive if this
        string is greater than @a cstr. Lexicographic order treats
        characters as unsigned. */
    int compare(const char* cstr) const {
        return String_generic_templated<C,char>::compare(data(), length(), cstr, strlen(cstr));
    }
    /** @brief Compare this string with the first @a len characters of @a
        s. */
    int compare(const char* s, int len) const {
        return String_generic_templated<C,char>::compare(data(), length(), s, len);
    }
    /** @brief Compare this string with @a x. */
    template <typename TT, typename CC>
    int compare(const String_base_templated<TT,CC>& x) const {
        return  String_generic_templated<C,CC>::compare(data(), length(), x.data(), x.length());
    }
    /** @brief Compare strings @a a and @a b. */
    template <typename TT, typename UU, typename CC, typename DD>
    static int compare(const String_base_templated<TT,CC>& a, const String_base_templated<UU,DD>& b) {
        return  String_generic_templated<CC,DD>::compare(a.data(), a.length(), b.data(), b.length());
    }
    /** @brief Compare strings @a a and @a b. */
    template <typename UU, typename DD>
    static int compare(const char* a, const String_base_templated<UU,DD> &b) {
        return  String_generic_templated<char,DD>::compare(a, strlen(a), b.data(), b.length());
    }
    /** @brief Compare strings @a a and @a b. */
    template <typename TT, typename CC>
    static int compare(const String_base_templated<TT,CC>& a, const char* b) {
        return  String_generic_templated<CC,char>::compare(a.data(), a.length(), b, strlen(b));
    }
    /** @brief Compare strings @a a and @a b. */
    static int compare(const char* a, const char* b) {
        return String_generic_templated<char,char>::compare(a, strlen(a), b, strlen(b));
    }
    /** @brief Compare this string with the C string @a cstr using natural order.

        Natural string comparison attempts to order embedded decimal number
        reprepresentations according to their numeric order; thus, the
        string "100" compares greater than the string "2". Returns 0 if this
        string equals @a cstr (only possible if the strings are
        byte-for-byte identical), negative if this string is less than @a
        cstr in natural order, and positive if this string is greater
        than @a cstr in natural order. */
    int natural_compare(const char *cstr) const {
        return String_generic_templated<C,char>::natural_compare(data(), length(), cstr, strlen(cstr));
    }
    /** @brief Compare this string with the first @a len characters of @a
        s using natural order. */
    int natural_compare(const char *s, int len) const {
        return String_generic_templated<C,char>::natural_compare(data(), length(), s, len);
    }
    /** @brief Compare this string with @a x using natural order. */
    template <typename TT, typename CC>
    int natural_compare(const String_base_templated<TT,CC> &x) const {
        return String_generic_templated<C,CC>::natural_compare(data(), length(), x.data(), x.length());
    }
    /** @brief Compare strings @a a and @a b using natural order. */
    template <typename TT, typename UU, typename CC, typename DD>
    static int natural_compare(const String_base_templated<TT,CC> &a, const String_base_templated<UU,DD> &b) {
        return String_generic_templated<CC,DD>::natural_compare(a.data(), a.length(), b.data(), b.length());
    }
    /** @brief Compare strings @a a and @a b using natural order. */
    template <typename UU, typename DD>
    static int natural_compare(const char* a, const String_base_templated<UU,DD> &b) {
        return String_generic_templated<char,DD>::natural_compare(a, strlen(a), b.data(), b.length());
    }
    /** @brief Compare strings @a a and @a b using natural order. */
    template <typename TT, typename CC>
    static int natural_compare(const String_base_templated<TT,CC>& a, const char* b) {
        return String_generic_templated<CC,char>::natural_compare(a.data(), a.length(), b, strlen(b));
    }
    /** @brief Compare strings @a a and @a b using natural order. */
    static int natural_compare(const char* a, const char* b) {
        return String_generic_templated<char,char>::natural_compare(a, strlen(a), b, strlen(b));
    }
    /** @brief Compare strings @a a and @a b using natural order. */
    static int natural_compare(const std::string& a, const std::string& b) {
        return String_generic_templated<char,char>::natural_compare(a.data(), a.length(), b.data(), b.length());
    }
    /** @brief Comparator function object for natural string comparison. */
    class natural_comparator {
      public:
        template <typename TT, typename UU>
        bool operator()(const TT& a, const UU& b) const {
            return String_base_templated<T,C>::natural_compare(a, b) < 0;
        }
    };
    /** @brief Test if this string begins with the C string @a cstr. */
    bool starts_with(const char *cstr) const {
        return String_generic_templated<C,char>::starts_with(data(), length(), cstr, strlen(cstr));
    }
    /** @brief Test if this string begins with the first @a len characters
        of @a s. */
    bool starts_with(const char *s, int len) const {
        return String_generic_templated<C,char>::starts_with(data(), length(), s, len);
    }
    /** @brief Test if this string begins with @a x. */
    template <typename TT, typename CC>
    bool starts_with(const String_base_templated<TT,CC> &x) const {
        return String_generic_templated<C,CC>::starts_with(data(), length(), x.data(), x.length());
    }
    /** @brief Search for a character in this string.

        Return the index of the leftmost occurrence of @a c, starting at
        index @a start and working up to the end of the string. Return -1 if
        the character is not found. */
    int find_left(char x, int start = 0) const {
        return String_generic_templated<C,char>::find_left(data(), length(), start, x);
    }
    /** @brief Search for the C string @a cstr as a substring in this string.

        Return the index of the leftmost occurrence of @a cstr, starting at
        index @a start. Return -1 if the substring is not found. */
    int find_left(const char *cstr, int start = 0) const {
        return String_generic_templated<C,char>::find_left(data(), length(), start, cstr, strlen(cstr));
    }
    /** @brief Search for @a x as a substring in this string.

        Return the index of the leftmost occurrence of @a x, starting at
        index @a start. Return -1 if the substring is not found. */
    template <typename TT, typename CC>
    int find_left(const String_base_templated<TT,CC> &x, int start = 0) const {
        return String_generic_templated<C,CC>::find_left(data(), length(), start, x.data(), x.length());
    }
    /** @brief Search backwards for a character in this string.

        Return the index of the rightmost occurrence of @a c, starting at
        index @a start and working up to the end of the string. Return -1 if
        the character is not found. */
    int find_right(char c, int start = INT_MAX) const {
        return String_generic_templated<C,char>::find_right(data(), length(), start, c);
    }
    /** @brief Search backwards for the C string @a cstr as a substring in
        this string.

        Return the index of the rightmost occurrence of @a cstr, starting
        at index @a start. Return -1 if the substring is not found. */
    int find_right(const char *cstr, int start = INT_MAX) const {
        return String_generic_templated<C,char>::find_right(data(), length(), start, cstr, strlen(cstr));
    }
    /** @brief Search backwards for @a x as a substring in this string.

        Return the index of the rightmost occurrence of @a x, starting at
        index @a start. Return -1 if the substring is not found. */
    template <typename TT, typename CC>
    int find_right(const String_base_templated<TT,CC> &x, int start = INT_MAX) const {
        return String_generic_templated<C,CC>::find_right(data(), length(), start, x.data(), x.length());
    }
    /** @brief Test if this string matches the glob @a pattern.

        Glob pattern syntax allows * (any number of characters), ? (one
        arbitrary character), [] (character classes, possibly negated), and
        \\ (escaping). */
    bool glob_match(const char* pattern) const {
        return String_generic_templated<C,char>::glob_match(data(), length(), pattern, strlen(pattern));
    }
    /** @overload */
    template <typename TT, typename CC>
    bool glob_match(const String_base_templated<TT,CC>& pattern) const {
        return String_generic_templated<C,CC>::glob_match(data(), length(), pattern.data(), pattern.length());
    }
    /** @brief Return a 32-bit hash function of the characters in [@a first, @a last).

        Uses Paul Hsieh's "SuperFastHash" algorithm, described at
        http://www.azillionmonkeys.com/qed/hash.html
        This hash function uses all characters in the string.

        @invariant If last1 - first1 == last2 - first2 and memcmp(first1,
        first2, last1 - first1) == 0, then hashcode(first1, last1) ==
        hashcode(first2, last2). */
    static hashcode_t hashcode(const C *first, const C *last) {
        return String_generic_templated<C,C>::hashcode(first, last);
    }
    /** @brief Return a 32-bit hash function of the characters in this string. */
    hashcode_t hashcode() const {
        return String_generic_templated<C,C>::hashcode(data(), length());
    }

    /** @brief Return the integer value of this string. */
    long to_i() const {
        return String_generic_templated<C,C>::to_i(begin(), end());
    }

    template <typename E>
    const_iterator encode_json_partial(E& e) const;
    template <typename E>
    inline void encode_json(E& e) const;
    template <typename E>
    void encode_base64(E& e, bool pad = false) const;
    template <typename E>
    bool decode_base64(E& e) const;
    template <typename E>
    void encode_uri_component(E& e) const;

    /** @brief Return this string as a std::string. */
    inline operator std::string() const {
        return std::string(begin(), end());
    }

  protected:
    String_base_templated() = default;
};

template <typename T>
using String_base = String_base_templated<T, char>;

template <typename T, typename U, typename C, typename D>
inline bool operator==(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return a.equals(b);
}

template <typename T, typename C>
inline bool operator==(const String_base_templated<T,C> &a, const std::string &b) {
    return a.equals(b.data(), b.length());
}

template <typename T, typename C>
inline bool operator==(const std::string &a, const String_base_templated<T,C> &b) {
    return b.equals(a.data(), a.length());
}

template <typename T, typename C>
inline bool operator==(const String_base_templated<T,C> &a, const char *b) {
    return a.equals(b, strlen(b));
}

template <typename T, typename C>
inline bool operator==(const char *a, const String_base_templated<T,C> &b) {
    return b.equals(a, strlen(a));
}

template <typename T, typename U, typename C, typename D>
inline bool operator!=(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return !(a == b);
}

template <typename T, typename C>
inline bool operator!=(const String_base_templated<T,C> &a, const std::string &b) {
    return !(a == b);
}

template <typename T, typename C>
inline bool operator!=(const std::string &a, const String_base_templated<T,C> &b) {
    return !(a == b);
}

template <typename T, typename C>
inline bool operator!=(const String_base_templated<T,C> &a, const char *b) {
    return !(a == b);
}

template <typename T, typename C>
inline bool operator!=(const char *a, const String_base_templated<T,C> &b) {
    return !(a == b);
}

template <typename T, typename U, typename C, typename D>
inline bool operator<(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return a.compare(b) < 0;
}

template <typename T, typename U, typename C, typename D>
inline bool operator<=(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return a.compare(b) <= 0;
}

template <typename T, typename U, typename C, typename D>
inline bool operator>=(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return a.compare(b) >= 0;
}

template <typename T, typename U, typename C, typename D>
inline bool operator>(const String_base_templated<T,C> &a, const String_base_templated<U,D> &b) {
    return a.compare(b) > 0;
}

template <typename T, typename C>
inline std::ostream &operator<<(std::ostream &f, const String_base_templated<T,C> &str) {
    return f.write(str.data(), str.length());
}

template <typename T, typename C>
inline hashcode_t hashcode(const String_base_templated<T,C>& x) {
    return String_generic::hashcode(x.data(), x.length());
}

// boost's spelling
template <typename T, typename C>
inline size_t hash_value(const String_base_templated<T,C>& x) {
    return String_generic::hashcode(x.data(), x.length());
}

template <typename T, typename U>
int String_generic_templated<T, U>::compare(const T* a, int a_len, const U* b, int b_len)
{
    if (reinterpret_cast<uintptr_t>(a) != reinterpret_cast<uintptr_t>(b)) {
        int len = a_len < b_len ? a_len : b_len;
        int cmp = maybe_atomic_memcmp(a, b, len);
        if (cmp != 0)
            return cmp;
    }
    return a_len - b_len;
}

template <typename T, typename U>
int String_generic_templated<T, U>::natural_compare(const T* a, int a_len,
                                    const U* b, int b_len) {
    const T* ae = a + a_len;
    const U* be = b + b_len;
    const T* aperiod = 0;
    bool aperiod_negative = false;
    int raw_compare = 0;

    while (a < ae && b < be) {
        if (isdigit((unsigned char) *a) && isdigit((unsigned char) *b)) {
            // compare the two numbers, but treat them as strings
            // (a decimal conversion might cause overflow)
            bool potential_decimal = (a == aperiod);

            // check if both are negative (note that if we get here, entire
            // string prefixes are identical)
            bool negative = false;
            if (a > ae - a_len && a[-1] == '-'
                && (a == ae - a_len + 1
                    || isspace((unsigned char) a[-2])))
                negative = true;

            // skip initial '0's, but remember any difference in length
            const T *ia = a;
            const U *ib = b;
            while (a < ae && *a == '0')
                ++a;
            while (b < be && *b == '0')
                ++b;
            int longer_zeros = (a - ia) - (b - ib);

            // walk over digits, remembering first nonidentical digit comparison
            int digit_compare = 0;
            bool a_good, b_good;
            while (1) {
                a_good = a < ae && isdigit((unsigned char) *a);
                b_good = b < be && isdigit((unsigned char) *b);
                if (!a_good || !b_good)
                    break;
                if (digit_compare == 0)
                    digit_compare = *a - *b;
                ++a;
                ++b;
            }

            // real number comparison: leading zeros are significant,
            // digit comparisons take precedence
            if (potential_decimal) {
                const T *ax = a;
                const U *bx = b;
                while (ax < ae && isdigit((unsigned char) *ax))
                    ++ax;
                while (bx < be && isdigit((unsigned char) *bx))
                    ++bx;
                // watch for IP addresses: don't treat "0.2." like a decimal
                if (!(ax + 1 < ae && *ax == '.' && !isspace((unsigned char) ax[1]))
                    && !(bx + 1 < be && *bx == '.' && !isspace((unsigned char) bx[1]))) {
                    negative = aperiod_negative;
                    if (longer_zeros)
                        return negative ? 1 : -1;
                    if (digit_compare)
                        a_good = b_good;
                }
            }
            // if one number is longer, it must also be larger
            if (a_good != b_good)
                return negative == a_good ? -1 : 1;
            // otherwise, digit comparisons take precedence
            if (digit_compare)
                return negative == (digit_compare > 0) ? -1 : 1;
            // as a last resort, the longer string of zeros is greater
            if (longer_zeros)
                return longer_zeros;
            // prepare for potential decimal comparison later
            if (!aperiod) {
                a_good = a + 1 < ae && *a == '.'
                    && isdigit((unsigned char) a[1]);
                b_good = b + 1 < be && *b == '.'
                    && isdigit((unsigned char) b[1]);
                if (a_good != b_good)
                    return negative == b_good ? 1 : -1;
                else if (a_good) {
                    aperiod = a + 1;
                    aperiod_negative = negative;
                }
            }

            // if we get here, the numeric portions were byte-for-byte
            // identical; move on
        } else if (isdigit((unsigned char) *a))
            return isalpha((unsigned char) *b) ? -1 : 1;
        else if (isdigit((unsigned char) *b))
            return isalpha((unsigned char) *a) ? 1 : -1;
        else {
            int alower = (unsigned char) tolower((unsigned char) *a);
            int blower = (unsigned char) tolower((unsigned char) *b);
            if (alower != blower)
                return alower - blower;
            if (raw_compare == 0)
                raw_compare = (unsigned char) *a - (unsigned char) *b;
            if (*a != '.')
                aperiod = 0;
            ++a;
            ++b;
        }
    }

    if ((ae - a) != (be - b))
        return (ae - a) - (be - b);
    else
        return raw_compare;
}

template <typename T, typename U>
hashcode_t String_generic_templated<T,U>::hashcode(const T *s, int len)
{
    //  TODO: only enabled for char, char
    if constexpr (!(std::is_same_v<T, char> && std::is_same_v<U, char>)) {
        assert(0);
    }

    if (len <= 0)
        return 0;

    uint32_t hash = len;
    int rem = hash & 3;
    const char *end = s + len - rem;
    uint32_t last16;

#if !HAVE_INDIFFERENT_ALIGNMENT
    if (!(reinterpret_cast<uintptr_t>(s) & 1)) {
#endif
#define get16(p) (*reinterpret_cast<const uint16_t *>((p)))
        for (; s != end; s += 4) {
            hash += get16(s);
            uint32_t tmp = (get16(s + 2) << 11) ^ hash;
            hash = (hash << 16) ^ tmp;
            hash += hash >> 11;
        }
        if (rem >= 2) {
            last16 = get16(s);
            goto rem2;
        }
#undef get16
#if !HAVE_INDIFFERENT_ALIGNMENT
    } else {
# if !__i386__ && !__x86_64__ && !__arch_um__
#  define get16(p) (((unsigned char) (p)[0] << 8) + (unsigned char) (p)[1])
# else
#  define get16(p) ((unsigned char) (p)[0] + ((unsigned char) (p)[1] << 8))
# endif
        // should be exactly the same as the code above
        for (; s != end; s += 4) {
            hash += get16(s);
            uint32_t tmp = (get16(s + 2) << 11) ^ hash;
            hash = (hash << 16) ^ tmp;
            hash += hash >> 11;
        }
        if (rem >= 2) {
            last16 = get16(s);
            goto rem2;
        }
# undef get16
    }
#endif

    /* Handle end cases */
    if (0) {                    // weird organization avoids uninitialized
      rem2:                     // variable warnings
        if (rem == 3) {
            hash += last16;
            hash ^= hash << 16;
            hash ^= ((unsigned char) s[2]) << 18;
            hash += hash >> 11;
        } else {
            hash += last16;
            hash ^= hash << 11;
            hash += hash >> 17;
        }
    } else if (rem == 1) {
        hash += (unsigned char) *s;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

template <typename T, typename U>
int String_generic_templated<T,U>::find_left(const T *s, int len, int start,
                          U x)
{
    if (start < 0)
        start = 0;
    for (int i = start; i < len; ++i)
        if (s[i] == x)
            return i;
    return -1;
}

template <typename T, typename U>
int String_generic_templated<T,U>::find_left(const T *s, int len, int start,
                          const U *x, int x_len)
{
    if (start < 0)
        start = 0;
    if (x_len == 0)
        return start <= len ? start : -1;
    int max_pos = len - x_len;
    for (int i = start; i <= max_pos; ++i)
        if (maybe_atomic_memcmp(s + i, x, x_len) == 0)
            return i;
    return -1;
}

template <typename T, typename U>
int String_generic_templated<T,U>::find_right(const T *s, int len, int start,
                           U x)
{
    if (start >= len)
        start = len - 1;
    for (int i = start; i >= 0; --i)
        if (s[i] == x)
            return i;
    return -1;
}

template <typename T, typename U>
int String_generic_templated<T,U>::find_right(const T *s, int len, int start,
                           const U *x, int x_len)
{
    if (start >= len)
        start = len - x_len;
    if (x_len == 0)
        return start >= 0 ? start : -1;
    for (int i = start; i >= 0; --i)
        if (maybe_atomic_memcmp(s + i, x, x_len) == 0)
            return i;
    return -1;
}

template <typename T, typename U>
long String_generic_templated<T,U>::to_i(const T* s, const T* ends) {
    bool neg;
    if (s != ends && (s[0] == '-' || s[0] == '+')) {
        neg = s[0] == '-';
        ++s;
    } else
        neg = false;
    if (s == ends || !isdigit((unsigned char) *s))
        return 0;
    unsigned long x = (unsigned char) *s - '0';
    for (++s; s != ends && isdigit((unsigned char) *s); ++s)
        x = x * 10 + *s - '0';  // XXX overflow
    return neg ? -x : x;
}

template <typename T, typename U>
bool String_generic_templated<T,U>::glob_match(const T* sbegin, int slen,
                                const U* pbegin, int plen) {
    const T* send = sbegin + slen;
    const U* pend = pbegin + plen;

    // quick common-case check for suffix matches
    while (pbegin < pend && sbegin < send
           && pend[-1] != '*' && pend[-1] != '?' && pend[-1] != ']'
           && (pbegin + 1 == pend || pend[-2] != '\\'))
        if (pend[-1] == send[-1])
            --pend, --send;
        else
            return false;

    std::vector<const U*> state, nextstate;
    state.push_back(pbegin);

    for (const T* s = sbegin; s != send && state.size(); ++s) {
        nextstate.clear();
        for (const U** pp = state.data(); pp != state.data() + state.size(); ++pp)
            if (*pp != pend) {
              reswitch:
                switch (**pp) {
                  case '?':
                    nextstate.push_back(*pp + 1);
                    break;
                  case '*':
                    if (*pp + 1 == pend)
                        return true;
                    if (nextstate.empty() || nextstate.back() != *pp)
                        nextstate.push_back(*pp);
                    ++*pp;
                    goto reswitch;
                  case '\\':
                    if (*pp + 1 != pend)
                        ++*pp;
                    goto normal_char;
                  case '[': {
                      const U *ec = *pp + 1;
                      bool negated;
                      if (ec != pend && *ec == '^') {
                          negated = true;
                          ++ec;
                      } else
                          negated = false;
                      if (ec == pend)
                          goto normal_char;

                      bool found = false;
                      do {
                          if (*++ec == *s)
                              found = true;
                      } while (ec != pend && *ec != ']');
                      if (ec == pend)
                          goto normal_char;

                      if (found == !negated)
                          nextstate.push_back(ec + 1);
                      break;
                  }
                  normal_char:
                  default:
                    if (**pp == *s)
                        nextstate.push_back(*pp + 1);
                    break;
                }
            }
        state.swap(nextstate);
    }

    for (const U** pp = state.data(); pp != state.data() + state.size(); ++pp) {
        while (*pp != pend && **pp == '*')
            ++*pp;
        if (*pp == pend)
            return true;
    }
    return false;
}

//  TODO: these still use char
template <typename T, typename U>
template <typename V>
inline typename V::substring_type String_generic_templated<T,U>::ltrim(const V &str) {
    const char *b = str.begin(), *e = str.end();
    while (b != e && isspace((unsigned char) b[0]))
        ++b;
    return str.fast_substring(b, e);
}

template <typename T, typename U>
template <typename V>
inline typename V::substring_type String_generic_templated<T,U>::rtrim(const V &str) {
    const char *b = str.begin(), *e = str.end();
    while (b != e && isspace((unsigned char) e[-1]))
        --e;
    return str.fast_substring(b, e);
}

template <typename T, typename U>
template <typename V>
inline typename V::substring_type String_generic_templated<T,U>::trim(const V &str) {
    const char *b = str.begin(), *e = str.end();
    while (b != e && isspace((unsigned char) e[-1]))
        --e;
    while (b != e && isspace((unsigned char) b[0]))
        ++b;
    return str.fast_substring(b, e);
}

#if HAVE_STD_HASH
# define LCDF_MAKE_STRING_HASH(type) \
    namespace std { template <> struct hash<type> {        \
        size_t operator()(const type& x) const noexcept {  \
            return x.hashcode();                           \
        } }; }
#else
# define LCDF_MAKE_STRING_HASH(type)
#endif

template <typename T, typename C>
template <typename E>
typename String_base_templated<T,C>::const_iterator String_base_templated<T,C>::encode_json_partial(E& enc) const {
    const char *last = this->begin(), *end = this->end();
    for (const char *s = last; s != end; ++s) {
        int c = (unsigned char) *s;

        // U+2028 and U+2029 can't appear in Javascript strings! (Though
        // they are legal in JSON strings, according to the JSON
        // definition.)
        if (unlikely(c == 0xE2)
            && s + 2 < end && (unsigned char) s[1] == 0x80
            && (unsigned char) (s[2] | 1) == 0xA9)
            c = 0x2028 + (s[2] & 1);
        else if (likely(c >= 32 && c != '\\' && c != '\"' && c != '/'))
            continue;

        enc.append(last, s);
        enc << '\\';
        switch (c) {
        case '\b':
            enc << 'b';
            break;
        case '\f':
            enc << 'f';
            break;
        case '\n':
            enc << 'n';
            break;
        case '\r':
            enc << 'r';
            break;
        case '\t':
            enc << 't';
            break;
        case '\\':
        case '\"':
        case '/':
            enc << (char) c;
            break;
        default: { // c is a control character, 0x2028, or 0x2029
            char* x = enc.extend(5);
            *x++ = 'u';
            *x++ = String_generic::upper_hex_nibble(c >> 12);
            *x++ = String_generic::upper_hex_nibble((c >> 8) & 0xF);
            *x++ = String_generic::upper_hex_nibble((c >> 4) & 0xF);
            *x++ = String_generic::upper_hex_nibble(c & 0xF);
            if (c > 255)        // skip rest of encoding of U+202[89]
                s += 2;
            break;
        }
        }
        last = s + 1;
    }
    return last;
}

template <typename T, typename C>
template <typename E>
inline void String_base_templated<T,C>::encode_json(E& enc) const {
    const char* last = encode_json_partial(enc);
    enc.append(last, end());
}

template <typename T, typename C>
template <typename E>
void String_base_templated<T,C>::encode_base64(E& enc, bool pad) const {
    char* out = enc.reserve(((length() + 2) * 4) / 3);
    const unsigned char* s = this->ubegin(), *end = this->uend();
    for (; end - s >= 3; s += 3) {
        unsigned x = (s[0] << 16) | (s[1] << 8) | s[2];
        *out++ = String_generic::base64_encoding_table[x >> 18];
        *out++ = String_generic::base64_encoding_table[(x >> 12) & 63];
        *out++ = String_generic::base64_encoding_table[(x >> 6) & 63];
        *out++ = String_generic::base64_encoding_table[x & 63];
    }
    if (end > s) {
        unsigned x = s[0] << 16;
        if (end > s + 1)
            x |= s[1] << 8;
        *out++ = String_generic::base64_encoding_table[x >> 18];
        *out++ = String_generic::base64_encoding_table[(x >> 12) & 63];
        if (end > s + 1)
            *out++ = String_generic::base64_encoding_table[(x >> 6) & 63];
        else if (pad)
            *out++ = '=';
        if (pad)
            *out++ = '=';
    }
    enc.set_end(out);
}

template <typename T, typename C>
template <typename E>
bool String_base_templated<T,C>::decode_base64(E& enc) const {
    char* out = enc.reserve((length() * 3) / 4 + 1);
    const unsigned char* s = this->ubegin(), *end = this->uend();
    while (end > s && end[-1] == '=')
        --end;
    for (; end - s >= 4; s += 4) {
        unsigned x = ((((unsigned) String_generic::base64_decoding_map[s[0]]) - 1) << 18)
            | ((((unsigned) String_generic::base64_decoding_map[s[1]]) - 1) << 12)
            | ((((unsigned) String_generic::base64_decoding_map[s[2]]) - 1) << 6)
            | (((unsigned) String_generic::base64_decoding_map[s[3]]) - 1);
        if ((int) x < 0)
            return false;
        *out++ = (unsigned char) (x >> 16);
        *out++ = (unsigned char) (x >> 8);
        *out++ = (unsigned char) x;
    }
    if (end - s >= 2) {
        unsigned x = ((((unsigned) String_generic::base64_decoding_map[s[0]]) - 1) << 18)
            | ((((unsigned) String_generic::base64_decoding_map[s[1]]) - 1) << 12)
            | (end - s == 3 ? (((unsigned) String_generic::base64_decoding_map[s[2]]) - 1) << 6 : 0);
        if ((int) x < 0)
            return false;
        *out++ = (unsigned char) (x >> 16);
        if (end - s == 3)
            *out++ = (unsigned char) (x >> 8);
    } else if (end - s)
        return false;
    enc.set_end(out);
    return true;
}

template <typename T, typename C>
template <typename E>
void String_base_templated<T,C>::encode_uri_component(E& enc) const {
    const char *last = this->begin(), *end = this->end();
    enc.reserve(end - last);
    for (const char *s = last; s != end; ++s) {
        int c = (unsigned char) *s;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            continue;
        enc.append(last, s);
        char* x = enc.extend(3);
        *x++ = '%';
        *x++ = String_generic::upper_hex_nibble(c >> 4);
        *x++ = String_generic::upper_hex_nibble(c & 0xF);
        last = s + 1;
    }
    enc.append(last, end);
}

} // namespace lcdf
#endif
