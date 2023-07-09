#pragma once
#include <string>


class TaskObj {
public:
	TaskObj(const std::string& fileName, std::uintmax_t fileSize);
	std::string getName() const;
	std::uintmax_t getSize() const;
private:
	std::string fileName;
	std::uintmax_t fileSize;
};