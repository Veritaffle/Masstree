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
#ifndef MASSTREE_NODEVERSION_HH
#define MASSTREE_NODEVERSION_HH
#include "compiler.hh"

// #define NODEVERSION_IMPL_HANDROLLED 1
// #define NODEVERSION_IMPL_FULLATOMIC 2
// #define NODEVERSION_IMPL_ATOMICFLAG 3

// #define NODEVERSION_IMPL NODEVERSION_IMPL_HANDROLLED

#if defined(NODEVERSION_IMPL_HANDROLLED)
//  original implementation
template <typename P>
class nodeversion {
  public:
    typedef P traits_type;
    typedef typename P::value_type value_type;

    nodeversion() {
    }
    explicit nodeversion(bool isleaf) {
        v_ = isleaf ? (value_type) P::isleaf_bit : 0;
    }

    bool isleaf() const {
        return v_ & P::isleaf_bit;
    }

    nodeversion<P> stable() const {
        return stable(relax_fence_function());
    }
    template <typename SF>
    nodeversion<P> stable(SF spin_function) const {
        value_type x = v_;
        while (x & P::dirty_mask) {
            spin_function();
            x = v_;
        }
        acquire_fence();
        return x;
    }
    template <typename SF>
    nodeversion<P> stable_annotated(SF spin_function) const {
        value_type x = v_;
        while (x & P::dirty_mask) {
            spin_function(nodeversion<P>(x));
            x = v_;
        }
        acquire_fence();
        return x;
    }

    bool locked() const {
        return v_ & P::lock_bit;
    }
    bool inserting() const {
        return v_ & P::inserting_bit;
    }
    bool splitting() const {
        return v_ & P::splitting_bit;
    }
    bool deleted() const {
        return v_ & P::deleted_bit;
    }
    bool has_changed(nodeversion<P> x) const {
        fence();
        return (x.v_ ^ v_) > P::lock_bit;
    }
    bool is_root() const {
        return v_ & P::root_bit;
    }
    bool has_split(nodeversion<P> x) const {
        fence();
        return (x.v_ ^ v_) >= P::vsplit_lowbit;
    }
    bool simple_has_split(nodeversion<P> x) const {
        return (x.v_ ^ v_) >= P::vsplit_lowbit;
    }

    nodeversion<P> lock() {
        return lock(*this);
    }
    nodeversion<P> lock(nodeversion<P> expected) {
        return lock(expected, relax_fence_function());
    }
    template <typename SF>
    nodeversion<P> lock(nodeversion<P> expected, SF spin_function) {
        while (true) {
            if (!(expected.v_ & P::lock_bit)
                && bool_cmpxchg(&v_, expected.v_,
                                expected.v_ | P::lock_bit)) {
                break;
            }
            spin_function();
            expected.v_ = v_;
        }
        masstree_invariant(!(expected.v_ & P::dirty_mask));
        expected.v_ |= P::lock_bit;
        acquire_fence();
        masstree_invariant(expected.v_ == v_);
        return expected;
    }

    bool try_lock() {
        return try_lock(relax_fence_function());
    }
    template <typename SF>
    bool try_lock(SF spin_function) {
        value_type expected = v_;
        if (!(expected & P::lock_bit)
            && bool_cmpxchg(&v_, expected, expected | P::lock_bit)) {
            masstree_invariant(!(expected & P::dirty_mask));
            acquire_fence();
            masstree_invariant((expected | P::lock_bit) == v_);
            return true;
        } else {
            spin_function();
            return false;
        }
    }

    void unlock() {
        unlock(*this);
    }
    void unlock(nodeversion<P> x) {
        masstree_invariant((fence(), x.v_ == v_));
        masstree_invariant(x.v_ & P::lock_bit);
        if (x.v_ & P::splitting_bit)
            x.v_ = (x.v_ + P::vsplit_lowbit) & P::split_unlock_mask;
        else
            x.v_ = (x.v_ + ((x.v_ & P::inserting_bit) << 2)) & P::unlock_mask;
        release_fence();
        v_ = x.v_;
    }

    void mark_insert() {
        masstree_invariant(locked());
        v_ |= P::inserting_bit;
        acquire_fence();
    }
    nodeversion<P> mark_insert(nodeversion<P> current_version) {
        masstree_invariant((fence(), v_ == current_version.v_));
        masstree_invariant(current_version.v_ & P::lock_bit);
        v_ = (current_version.v_ |= P::inserting_bit);
        acquire_fence();
        return current_version;
    }
    void mark_split() {
        masstree_invariant(locked());
        v_ |= P::splitting_bit;
        acquire_fence();
    }
    void mark_change(bool is_split) {
        masstree_invariant(locked());
        v_ |= (is_split + 1) << P::inserting_shift;
        acquire_fence();
    }
    nodeversion<P> mark_deleted() {
        masstree_invariant(locked());
        v_ |= P::deleted_bit | P::splitting_bit;
        acquire_fence();
        return *this;
    }
    void mark_deleted_tree() {
        masstree_invariant(locked() && is_root());
        v_ |= P::deleted_bit;
        acquire_fence();
    }
    void mark_root() {
        v_ |= P::root_bit;
        acquire_fence();
    }
    void mark_nonroot() {
        masstree_invariant(locked());
        v_ &= ~P::root_bit;
        acquire_fence();
    }

    void assign_version(nodeversion<P> x) {
        v_ = x.v_;
    }

    value_type version_value() const {
        return v_;
    }
    value_type unlocked_version_value() const {
        return v_ & P::unlock_mask;
    }

  private:
    value_type v_;

    nodeversion(value_type v)
        : v_(v) {
    }
};

#elif defined(NODEVERSION_IMPL_FULLATOMIC)
//  implementation with underlying datatype as relaxed_atomic
template <typename P>
class nodeversion {
  public:
    typedef P traits_type;
    typedef typename P::value_type value_type;

    nodeversion() {
    }

    
    nodeversion(const nodeversion &n)
    {
        // value_type v = n.v_.load();
        // atomic_release_fence();

        // v_.store(n.v_.load(), MO_RELEASE);
        // v_.store(n.v_.load(), MO_SEQ_CST);
        v_.store(n.v_.load());
    }
    
    explicit nodeversion(bool isleaf)
    {
        // value_type v = isleaf ? (value_type) P::isleaf_bit : 0;
        // atomic_release_fence();
        
        // v_.store(isleaf ? (value_type) P::isleaf_bit : 0, MO_RELEASE);
        // v_.store(isleaf ? (value_type) P::isleaf_bit : 0, MO_SEQ_CST);
        v_.store(isleaf ? (value_type) P::isleaf_bit : 0);
    }
    
    
    /*
    nodeversion(const nodeversion &n)
        : v_(n.v_.load()) {}

    explicit nodeversion(bool isleaf)
        : v_(isleaf ? (value_type) P::isleaf_bit : 0) {}
    */

    nodeversion<P>& operator=(const nodeversion<P>& other) {
        v_.store(other.v_.load(), MO_ACQUIRE);
        return *this;
    }

    bool isleaf() const {
        return v_.load(MO_ACQUIRE) & P::isleaf_bit;
    }

    nodeversion<P> stable() const {
        return stable(atomic_relax_fence_function());
    }
    template <typename SF>
    nodeversion<P> stable(SF spin_function) const {
        value_type x = v_.load();
        while (x & P::dirty_mask) {
            spin_function();
            x = v_.load();
        }
        atomic_acquire_fence();
        return x;
    }
    template <typename SF>
    nodeversion<P> stable_annotated(SF spin_function) const {
        value_type x = v_.load();
        while (x & P::dirty_mask) {
            spin_function(nodeversion<P>(x));
            x = v_.load();
        }
        atomic_acquire_fence();
        return x;
    }

    bool locked() const {
        return v_.load() & P::lock_bit;
    }
    bool inserting() const {
        return v_.load() & P::inserting_bit;
    }
    bool splitting() const {
        return v_.load() & P::splitting_bit;
    }
    bool deleted() const {
        return v_.load() & P::deleted_bit;
    }

    bool has_changed(nodeversion<P> x) const {
        atomic_fence();
        return (x.v_.load() ^ v_.load()) > P::lock_bit;
    }
    bool is_root() const {
        return v_.load() & P::root_bit;
    }
    bool has_split(nodeversion<P> x) const {
        atomic_fence();
        return (x.v_.load() ^ v_.load()) >= P::vsplit_lowbit;
    }
    bool simple_has_split(nodeversion<P> x) const {
        return (x.v_.load() ^ v_.load()) >= P::vsplit_lowbit;
    }

    nodeversion<P> lock() {
        //  TODO: is all this initialization of atomics going to hurt?
        return lock(*this);
    }
    nodeversion<P> lock(nodeversion<P> expected) {
        return lock(expected, atomic_relax_fence_function());
    }
    template <typename SF>
    nodeversion<P> lock(nodeversion<P> expected, SF spin_function) {
        //  Spin until we acquire the lock.
        while (true) {
            //  If expected isn't locked
            //  and we successfullly CAS the lock bit in this:
            value_type expected_val = expected.v_.load();
            if (!(expected_val & P::lock_bit)
                && v_.compare_exchange_weak(expected_val,
                                            expected_val | P::lock_bit)) {
                //  Stop spinning.
                break;
            }
            //  Else spin.
            spin_function();
            //  Update expected.
            //  spin_function() (usually?) contains a fence so that expected isn't changed too early.
            expected.v_.store(v_.load());
        }

        //  We (should) have successfully acquired the lock.
        //  Verify that expected isn't dirty.
        masstree_invariant(!(expected.v_.load() & P::dirty_mask));
        //  Set lock bit in expected.
        // expected.v_.fetch_and_or(P::lock_bit);
        expected.v_.store(expected.v_.load() | P::lock_bit);
        //  Acquire fence, since we have acquired the lock.
        atomic_acquire_fence();
        //  Verify that expected and this match.
        masstree_invariant(expected.v_.load() == v_.load());
        return expected;
    }
    
    bool try_lock() {
        return try_lock(atomic_relax_fence_function());
    }
    template <typename SF>
    bool try_lock(SF spin_function) {
        value_type expected = v_.load();
        if (!(expected & P::lock_bit)
            && v_.compare_exchange_weak(expected,
                                        expected | P::lock_bit)) {
            masstree_invariant(!(expected & P::dirty_mask));
            atomic_acquire_fence();
            masstree_invariant((expected | P::lock_bit) == v_.load());
            return true;
        } else {
            spin_function();
            return false;
        }
    }

    void unlock() {
        unlock(*this);
    }
    void unlock(nodeversion<P> x) {
        masstree_invariant((atomic_fence(), x.v_.load() == v_.load()));
        masstree_invariant(x.v_.load() & P::lock_bit);
        if (x.v_.load() & P::splitting_bit) {
            //  TODO: these two are jank
            // x.v_ = (x.v_ + P::vsplit_lowbit) & P::split_unlock_mask;

            // int expected = x.v_.load(std::memory_order_relaxed);
            // int desired;

            // do {
            //     desired = (expected + P::vsplit_lowbit) & P::split_unlock_mask;
            // } while (!x.v_.compare_exchange_weak(expected, desired, std::memory_order_acquire, std::memory_order_relaxed));

            x.v_.store((x.v_.load() + P::vsplit_lowbit) & P::split_unlock_mask);
        }
        else {
            value_type prev = x.v_.load();
            // x.v_ = (x.v_ + ((x.v_ & P::inserting_bit) << 2)) & P::unlock_mask;
            x.v_.store((prev + ((prev & P::inserting_bit) << 2)) & P::unlock_mask);
        }
        
        //  TODO: this could be a release operation instead of a release fence?
        atomic_release_fence();
        v_.store(x.v_.load());
    }

    void mark_insert() {
        masstree_invariant(locked());
        v_.fetch_and_or(P::inserting_bit);
        atomic_acquire_fence();
    }
    nodeversion<P> mark_insert(nodeversion<P> current_version) {
        masstree_invariant((atomic_fence(), v_.load() == current_version.v_.load()));
        masstree_invariant(current_version.v_.load() & P::lock_bit);
        // v_ = (current_version.v_ |= P::inserting_bit);
        v_.store(current_version.v_.fetch_and_or(P::inserting_bit) | P::inserting_bit);
        atomic_acquire_fence();
        return current_version;
    }
    void mark_split() {
        masstree_invariant(locked());
        v_.fetch_and_or(P::splitting_bit);
        atomic_acquire_fence();
    }
    void mark_change(bool is_split) {
        masstree_invariant(locked());
        v_.fetch_and_or((is_split + 1) << P::inserting_shift);
        atomic_acquire_fence();
    }
    nodeversion<P> mark_deleted() {
        masstree_invariant(locked());
        v_.fetch_and_or(P::deleted_bit | P::splitting_bit);
        atomic_acquire_fence();
        return *this;
    }
    void mark_deleted_tree() {
        masstree_invariant(locked() && is_root());
        v_.fetch_and_or(P::deleted_bit);
        atomic_acquire_fence();
    }
    void mark_root() {
        v_.fetch_and_or(P::root_bit);
        atomic_acquire_fence();
    }
    void mark_nonroot() {
        masstree_invariant(locked());
        v_.fetch_and_and(~P::root_bit);
        atomic_acquire_fence();
    }

    void assign_version(nodeversion<P> x) {
        //  TODO: release?
        v_.store(x.v_.load());
    }

    value_type version_value() const {
        return v_.load();
    }
    value_type unlocked_version_value() const {
        return v_.load() & P::unlock_mask;
    }

  private:
    relaxed_atomic<value_type> v_;

    nodeversion(value_type v)
        : v_(v) {
    }
};
#elif defined(NODEVERSION_IMPL_ATOMICFLAG)
//  lock bit separated as atomic flag
//  TODO: can I even do this?
#endif

template <typename P>
class singlethreaded_nodeversion {
  public:
    typedef P traits_type;
    typedef typename P::value_type value_type;

    singlethreaded_nodeversion() {
    }
    explicit singlethreaded_nodeversion(bool isleaf) {
        v_ = isleaf ? (value_type) P::isleaf_bit : 0;
    }

    bool isleaf() const {
        return v_ & P::isleaf_bit;
    }

    singlethreaded_nodeversion<P> stable() const {
        return *this;
    }
    template <typename SF>
    singlethreaded_nodeversion<P> stable(SF) const {
        return *this;
    }
    template <typename SF>
    singlethreaded_nodeversion<P> stable_annotated(SF) const {
        return *this;
    }

    bool locked() const {
        return false;
    }
    bool inserting() const {
        return false;
    }
    bool splitting() const {
        return false;
    }
    bool deleted() const {
        return false;
    }
    bool has_changed(singlethreaded_nodeversion<P>) const {
        return false;
    }
    bool is_root() const {
        return v_ & P::root_bit;
    }
    bool has_split(singlethreaded_nodeversion<P>) const {
        return false;
    }
    bool simple_has_split(singlethreaded_nodeversion<P>) const {
        return false;
    }

    singlethreaded_nodeversion<P> lock() {
        return *this;
    }
    singlethreaded_nodeversion<P> lock(singlethreaded_nodeversion<P>) {
        return *this;
    }
    template <typename SF>
    singlethreaded_nodeversion<P> lock(singlethreaded_nodeversion<P>, SF) {
        return *this;
    }

    bool try_lock() {
        return true;
    }
    template <typename SF>
    bool try_lock(SF) {
        return true;
    }

    void unlock() {
    }
    void unlock(singlethreaded_nodeversion<P>) {
    }

    void mark_insert() {
    }
    singlethreaded_nodeversion<P> mark_insert(singlethreaded_nodeversion<P>) {
        return *this;
    }
    void mark_split() {
        v_ &= ~P::root_bit;
    }
    void mark_change(bool is_split) {
        if (is_split)
            mark_split();
    }
    singlethreaded_nodeversion<P> mark_deleted() {
        return *this;
    }
    void mark_deleted_tree() {
        v_ |= P::deleted_bit;
    }
    void mark_root() {
        v_ |= P::root_bit;
    }
    void mark_nonroot() {
        v_ &= ~P::root_bit;
    }

    void assign_version(singlethreaded_nodeversion<P> x) {
        v_ = x.v_;
    }

    value_type version_value() const {
        return v_;
    }
    value_type unlocked_version_value() const {
        return v_;
    }

  private:
    value_type v_;
};


template <typename V> struct nodeversion_parameters {};

template <> struct nodeversion_parameters<uint32_t> {
    enum {
        lock_bit = (1U << 0),
        inserting_shift = 1,
        inserting_bit = (1U << 1),
        splitting_bit = (1U << 2),
        dirty_mask = inserting_bit | splitting_bit,
        vinsert_lowbit = (1U << 3), // == inserting_bit << 2
        vsplit_lowbit = (1U << 9),
        unused1_bit = (1U << 28),
        deleted_bit = (1U << 29),
        root_bit = (1U << 30),
        isleaf_bit = (1U << 31),
        split_unlock_mask = ~(root_bit | unused1_bit | (vsplit_lowbit - 1)),
        unlock_mask = ~(unused1_bit | (vinsert_lowbit - 1)),
        top_stable_bits = 4
    };

    typedef uint32_t value_type;
};

template <> struct nodeversion_parameters<uint64_t> {
    enum {
        lock_bit = (1ULL << 8),
        inserting_shift = 9,
        inserting_bit = (1ULL << 9),
        splitting_bit = (1ULL << 10),
        dirty_mask = inserting_bit | splitting_bit,
        vinsert_lowbit = (1ULL << 11), // == inserting_bit << 2
        vsplit_lowbit = (1ULL << 27),
        unused1_bit = (1ULL << 60),
        deleted_bit = (1ULL << 61),
        root_bit = (1ULL << 62),
        isleaf_bit = (1ULL << 63),
        split_unlock_mask = ~(root_bit | unused1_bit | (vsplit_lowbit - 1)),
        unlock_mask = ~(unused1_bit | (vinsert_lowbit - 1)),
        top_stable_bits = 4
    };

    typedef uint64_t value_type;
};

typedef nodeversion<nodeversion_parameters<uint32_t> > nodeversion32;

#endif
