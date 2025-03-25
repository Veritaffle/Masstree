#include <cstdio>
#include "compiler.hh"
#include "str.hh"

constexpr unsigned EXAMPLE_LENGTH = 4;

template<typename T, typename U>
void StringTest(T a, U b) {
	assert(!a.equals(b));
	assert(a.compare(b) < 0);
	// fprintf(stdout, "%d\n", a.natural_compare(b));
	assert(a.natural_compare(b) < 0);
	assert(a.starts_with("01", 2));
	fprintf(stdout, "%d\n", b.starts_with("01", 2));
	assert(b.starts_with("01", 2));
	assert(a.find_left("1") == 1);
	assert(a.find_left("01") == 0);
	assert(a.find_right("1") == 1);
	assert(a.find_right("01") == 0);

	fprintf(stdout, "StringTest pass\n");
}

int main(void) {
	
	char CharString1[5] = "0123";
	char CharString2[5] = "0167";
	
	relaxed_atomic<char> AtomicString1[EXAMPLE_LENGTH];
	AtomicString1[0] = '0';
	AtomicString1[1] = '1';
	AtomicString1[2] = '4';
	AtomicString1[3] = '5';

	relaxed_atomic<char> AtomicString2[EXAMPLE_LENGTH];
	AtomicString2[0] = '0';
	AtomicString2[1] = '1';
	AtomicString2[2] = '8';
	AtomicString2[3] = '9';

	lcdf::Str s1(CharString1, 4);
	lcdf::Str s2(CharString2, 4);

	lcdf::atomic_Str as1(&AtomicString1[0], 4);
	lcdf::atomic_Str as2(&AtomicString2[0], 4);

	StringTest(s1, s2);
	StringTest(s1, as1);
	StringTest(as1, s2);
	StringTest(as1, as2);

	maybe_atomic_memcpy(CharString1, CharString2, 4);
	assert(s1.equals(s2));
	maybe_atomic_memcpy(CharString1, AtomicString1, 4);
	assert(s1.equals(as1));
	maybe_atomic_memcpy(AtomicString1, CharString2, 4);
	assert(as1.equals(s2));
	maybe_atomic_memcpy(AtomicString1, AtomicString2, 4);
	assert(as1.equals(as2));

	fprintf(stdout, "All tests passed!\n");

	return 0;
}