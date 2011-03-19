#ifndef OPCODE_H
#define OPCODE_H

#include "win32def.h" //for stricmp

#include <vector>
#include <algorithm>

template<class FunctionType>
class OpcodeRegistry {
public:
	struct Description {
		const char* Name;
		FunctionType Function;
		int Flags;
		int Opcode;
	};
public:
	void Register(int count, const Description* opcodes);
	Description* Find(const char* Name);
	Description* Find(FunctionType Name);
private:
	static bool Compare(Description const& lhs, Description const& rhs) {
		return stricmp(lhs.Name, rhs.Name) < 0;
	}
	static bool Compare2(Description const& lhs, char const* rhs) {
		return stricmp(lhs.Name, rhs) < 0;
	}
	static bool CompareFunc(Description const& lhs, Description const& rhs) {
		return lhs.Func == rhs.Func;
	}
	std::vector<Description> Names;
};

template<class FT>
void OpcodeRegistry<FT>::Register(int count, const Description* opcodes)
{
	size_t size = Names.size();
	Names.resize(size + count);

	std::copy(opcodes, opcodes + count, Names.begin() + size);
	std::sort(Names.begin(), Names.end(), Compare);
}

template<class FT>
typename OpcodeRegistry<FT>::Description* OpcodeRegistry<FT>::Find(const char* Name)
{
	if (!Name)
		return NULL;
	typename std::vector<Description>::iterator it = std::lower_bound(Names.begin(), Names.end(), Name, Compare2);
	if (stricmp(it->Name, Name) != 0)
		return NULL;
	return &*it;
}

template<class FT>
typename OpcodeRegistry<FT>::Description* OpcodeRegistry<FT>::Find(FT Func)
{
	for (size_t i = 0; i < Names.size(); ++i)
		if (Names[i].Function == Func)
			return &Names[i];
	return NULL;
}

#endif
