//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// main.cpp 
// entry point and core loop (obviously)

#include <iostream>

#include "Core/typedef.h"

using namespace std;

int main()
{
	cout << "Size of u8  = " << sizeof(u8) << endl;
	cout << "Size of s8  = " << sizeof(s8) << endl;
	cout << "Size of u16 = " << sizeof(u16) << endl;
	cout << "Size of s16 = " << sizeof(s16) << endl;
	cout << "Size of u32 = " << sizeof(u32) << endl;
	cout << "Size of s32 = " << sizeof(s32) << endl;
	cout << "Size of u64 = " << sizeof(u64) << endl;
	cout << "Size of s64 = " << sizeof(s64) << endl;
	cin.ignore();
	return 0;
}