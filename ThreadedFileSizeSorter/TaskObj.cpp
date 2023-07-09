#include "TaskObj.hpp"

TaskObj::TaskObj(const std::string& fileName, std::uintmax_t fileSize) :
	fileName(fileName), fileSize(fileSize) 
{}

std::string TaskObj::getName() const {
	return fileName;
}

std::uintmax_t TaskObj::getSize() const {
	return fileSize;
}


