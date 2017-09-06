#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <iostream>

namespace util {
	/* Wait for enter to be pressed before continuing. */
	void PressEnterToContinue();

	/* Create a new temporary funciton at the given location. Returns true if successful. */
	bool CreateTempDirectory(std::string location);
	/* Removes the directory and all files in it given a location. Returns true if successful. */
	bool RemoveTempDirectory(std::string location);
}

#endif