/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2014 President and Fellows of Harvard College
 * Copyright (c) 2012-2014 Massachusetts Institute of Technology
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
#ifndef STRINGBAG_HH
#define STRINGBAG_HH 1
#include "compiler.hh"
#include "string_slice.hh"

/** @brief String collection used for Masstree key suffixes.

    A stringbag is a compact collection of up to W strings, where W is a
    parameter called the <em>bag width</em>. These strings are stored
    in contiguously allocated memory.

    Stringbag component strings support
    string_slice<uintptr_t>::equals_sloppy() without memory errors.

    The template parameter T is the offset type. This type determines the
    maximum supported capacity of a stringbag. Smaller types have lower
    overhead, but support smaller bags.

    Stringbags favor compactness over usability. The bag width W is an
    important parameter, but you can't recover W from the stringbag itself;
    you'll need to store that externally. Stringbags cannot be allocated
    conventionally. You must manage stringbag memory yourself:
    allocate an array of characters for the stringbag, then use placement
    new to construct the stringbag on that memory. Stringbag has a
    trivial destructor. */

#if defined(STRINGBAG_IMPL_ORIGINAL)

template <typename T>
class stringbag {
 public:
    /** @brief Type of offsets. */
    typedef T offset_type;
    typedef string_slice<uintptr_t> slice_type;

 private:
    struct info_type {
        offset_type pos;
        offset_type len;
        info_type()
            : pos(0), len(0) {
        }
        info_type(unsigned p, unsigned l)
            : pos(p), len(l) {
        }
    };

 public:
    /** @brief Return the maximum allowed capacity of a stringbag. */
    static constexpr unsigned max_size() {
        return ((unsigned) (offset_type) -1) + 1;
    }
    /** @brief Return the base size of a stringbag. */
    static constexpr size_t empty_size() {
        return sizeof(offset_type) * 2;
    }
    /** @brief Return the overhead for a stringbag of width @a width.

        This is the number of bytes allocated for overhead. */
    static constexpr size_t overhead(size_t width) {
        return empty_size() + width * sizeof(info_type);
    }
    /** @brief Return a capacity that can definitely contain a stringbag.
        @param width number of strings in bag
        @param len total number of bytes in bag's strings */
    static constexpr size_t safe_size(int width, unsigned len) {
        return overhead(width) + len + slice_type::size - 1;
    }

    /** @brief Construct an empty stringbag.
        @param width Number of strings in the bag
        @param capacity Number of bytes allocated for the bag
        @pre @a capacity > overhead(width)
        @pre @a capacity <= max_offset

        Stringbags should not be constructed on the stack or by a direct call
        to new. Allocate space for a stringbag, then call the constructor on
        that space using placement new. @a capacity must be no bigger than
        the allocated space. */
    stringbag(int width, size_t capacity) {
        size_t firstpos = overhead(width);
        assert(capacity >= firstpos && capacity <= max_size());
        size_ = firstpos;
        capacity_ = capacity - 1;
        for (int i = 0; i != width; ++i) {
            info_[i] = info_type();
        }
    }

    /** @brief Return the capacity used to construct this bag. */
    size_t capacity() const {
        return capacity_ + 1;
    }
    /** @brief Return the number of bytes used so far (including overhead). */
    size_t used_capacity() const {
        return size_;
    }

    /** @brief Return the string at position @a p.
        @pre @a p >= 0 && @a p < bag width */
    lcdf::Str operator[](int p) const {
        info_type info = info_[p];
        return lcdf::Str(s_ + info.pos, info.len);
    }
    /** @brief Return the string at position @a p.
        @pre @a p >= 0 && @a p < bag width */
    lcdf::Str get(int p) const {
        info_type info = info_[p];
        return lcdf::Str(s_ + info.pos, info.len);
    }

    /** @brief Assign the string at position @a p to @a s.
        @param p position
        @param s string
        @param len length of string
        @return true if the assignment succeeded, false if it failed
           (because the stringbag is out of capacity)
        @pre @a p >= 0 && @a p < bag width */
    bool assign(int p, const char *s, int len) {
        unsigned pos, mylen = info_[p].len;
        if (mylen >= (unsigned) len)
            pos = info_[p].pos;
        else if (size_ + (unsigned) std::max(len, slice_type::size)
                   <= capacity()) {
            pos = size_;
            size_ += len;
        } else
            return false;
        memcpy(s_ + pos, s, len);
        info_[p] = info_type(pos, len);
        return true;
    }
    /** @override */
    bool assign(int p, lcdf::Str s) {
        return assign(p, s.s, s.len);
    }

    /** @brief Print a representation of the stringbag to @a f. */
    void print(int width, FILE *f, const char *prefix, int indent) {
        fprintf(f, "%s%*s%p (%d:)%d:%d...\n", prefix, indent, "",
                this, (int) overhead(width), size_, capacity());
        for (int i = 0; i < width; ++i)
            if (info_[i].len)
                fprintf(f, "%s%*s  #%x %u:%u %.*s\n", prefix, indent, "",
                        i, info_[i].pos, info_[i].len, std::min(info_[i].len, 40U), s_ + info_[i].pos);
    }

  private:
    union {
        struct {
            offset_type size_;
            offset_type capacity_;
            info_type info_[1];
        };
        char s_[1];
    };
};

#elif defined(STRINGBAG_IMPL_ATOMIC)

template <typename T>
class stringbag {
 public:
    /** @brief Type of offsets. */
    typedef T offset_type;
    typedef string_slice<uintptr_t> slice_type;

 private:
    struct info_type {
        relaxed_atomic<offset_type> pos;
        relaxed_atomic<offset_type> len;
        info_type()
            : pos(0), len(0) {
        }
        // info_type(unsigned p, unsigned l)
        //     : pos(p), len(l) {
        // }

        void assign(unsigned p, unsigned l) {
            pos = p;
            len = l;
        }
        void assign() {
            assign(0, 0);
        }
    };

 public:
    /** @brief Return the maximum allowed capacity of a stringbag. */
    static constexpr unsigned max_size() {
        return ((unsigned) (offset_type) -1) + 1;
    }
    /** @brief Return the base size of a stringbag. */
    static constexpr size_t empty_size() {
        return sizeof(offset_type) * 2;
    }
    /** @brief Return the overhead for a stringbag of width @a width.

        This is the number of bytes allocated for overhead. */
    static constexpr size_t overhead(size_t width) {
        return empty_size() + width * sizeof(info_type);
    }
    /** @brief Return a capacity that can definitely contain a stringbag.
        @param width number of strings in bag
        @param len total number of bytes in bag's strings */
    static constexpr size_t safe_size(int width, unsigned len) {
        return overhead(width) + len + slice_type::size - 1;
    }

    /** @brief Construct an empty stringbag.
        @param width Number of strings in the bag
        @param capacity Number of bytes allocated for the bag
        @pre @a capacity > overhead(width)
        @pre @a capacity <= max_offset

        Stringbags should not be constructed on the stack or by a direct call
        to new. Allocate space for a stringbag, then call the constructor on
        that space using placement new. @a capacity must be no bigger than
        the allocated space. */
    stringbag(int width, size_t capacity) {
        // debug_fprintf(stderr, "stringbag()\n");
        size_t firstpos = overhead(width);
        assert(capacity >= firstpos && capacity <= max_size());
        size_ = firstpos;
        capacity_ = capacity - 1;

        // for (int i = 0; i != width; ++i) {
        //     info_[i].assign();
        // }

        //  set up atomics
        static_assert(sizeof(relaxed_atomic<char>) == 1);
        static_assert(alignof(relaxed_atomic<char>) == 1);
        (void) new(s_ + empty_size()) info_type[width];
        (void) new(s_ + size_) relaxed_atomic<char>[(capacity - size_) / sizeof(relaxed_atomic<char>)];
    }

    /** @brief Return the capacity used to construct this bag. */
    size_t capacity() const {
        return capacity_ + 1;
    }
    /** @brief Return the number of bytes used so far (including overhead). */
    size_t used_capacity() const {
        return size_;
    }

    /** @brief Return the string at position @a p.
        @pre @a p >= 0 && @a p < bag width */
    // lcdf::atomic_Str operator[](int p) const {
    //     info_type info = info_[p];
    //     return lcdf::Str(s_ + info.pos, info.len);
    // }
    /** @brief Return the string at position @a p.
        @pre @a p >= 0 && @a p < bag width */
    lcdf::atomic_Str get(int p) const {
        offset_type pos = info_[p].pos;
        offset_type len = info_[p].len;
        return lcdf::atomic_Str(reinterpret_cast<const relaxed_atomic<char>*>(s_ + pos), len);
    }

    /** @brief Assign the string at position @a p to @a s.
        @param p position
        @param s string
        @param len length of string
        @return true if the assignment succeeded, false if it failed
           (because the stringbag is out of capacity)
        @pre @a p >= 0 && @a p < bag width */
    template<typename C>
    bool assign(int p, const C *s, int len) {
        // debug_fprintf(stderr, "stringbag::assign() called\n");
        unsigned pos, mylen = info_[p].len;
        if (mylen >= (unsigned) len)
            //  string currently at p is already at least as long as new string
            //  we can put the new string in the same place
            pos = info_[p].pos;
        else if (size_ + (unsigned) std::max(len, slice_type::size)
                   <= capacity()) {
            //  size_ marks beginning of open space
            //  need a space that's at least big enough for a string_slice with slice_type uintptr_t
            //  and at least big enough for the length of the new string
            pos = size_;
            size_ = size_ + len;
        } else
            return false;
        // debug_fprintf(stderr, "stringbag::assign() hit memcpy\n");
        atomic_memcpy(reinterpret_cast<relaxed_atomic<char>*>(s_ + pos), s, len);
        info_[p].assign(pos, len);

        // debug_fprintf(stderr, "START_STRINGBAG_PRINT\n");
        // print(15, stderr, "[stringbag]", 4);

        return true;
    }
    /** @override */
    bool assign(int p, lcdf::Str s) {
        return assign(p, s.s, s.len);
    }
    bool assign(int p, lcdf::atomic_Str s) {
        return assign(p, s.s, s.len);
    }

    /** @brief Print a representation of the stringbag to @a f. */
    void print(int width, FILE *f, const char *prefix, int indent) {
        fprintf(f, "%s%*s%p (%d:)%d:%lu...\n", prefix, indent, "",
                this, (int) overhead(width), size_, capacity());
        for (int i = 0; i < width; ++i)
            if (info_[i].len)
                fprintf(f, "%s%*s  #%x %u:%u %.*s\n", prefix, indent, "",
                        i, info_[i].pos, info_[i].len, std::min(info_[i].len, static_cast<offset_type>(40)), s_ + info_[i].pos);
    }

  private:
    union {
        struct {
            relaxed_atomic<offset_type> size_;
            relaxed_atomic<offset_type> capacity_;
            info_type info_[1];
        };
        char s_[1];
    };
};

#elif defined(STRINBAG_IMPL_NOREASSIGN)

//  TODO

#endif

#endif