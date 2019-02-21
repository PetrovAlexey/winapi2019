#pragma once
class ITEM {
public:
	int size;
	LPVOID addres;
	bool state;
	ITEM(int size, LPVOID addres, bool state = false) : size(size), addres(addres), state(state) {};
	bool operator<(const ITEM& r) const
	{
		return (size < r.size);
	}
};