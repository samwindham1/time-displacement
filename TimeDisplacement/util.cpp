#include "util.h"

namespace util {
	/* Wait for enter to be pressed before continuing. */
	void PressEnterToContinue() {
		std::cout << "\n\nPress ENTER to continue... " << std::flush;
		std::cin.ignore(std::numeric_limits <std::streamsize> ::max(), '\n');
	}

#include <Windows.h>

	/* Create a new temporary funciton at the given location. Returns true if successful. */
	bool CreateTempDirectory(const std::string location) {
		std::cout << "Creating temp file..." << std::endl;
		if (!CreateDirectoryA(location.c_str(), NULL)) {
			if (GetLastError() != ERROR_ALREADY_EXISTS)
				std::cout << "ERROR: " << GetLastError() << std::endl;
			std::cout << "Directory already exists." << std::endl;
			return false;
		}
		else {
			std::cout << "done." << std::endl;
			return true;
		}
	}
	/* Removes the directory and all files in it given a location. Returns true if successful. */
	bool RemoveTempDirectory(const std::string location) {
		std::cout << "Removing temp file..." << std::endl;

		// ------------ Remove all files in directory

		// file variables to iterate through files
		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		const std::string szDir = location + "*";

		hFind = FindFirstFile(szDir.c_str(), &ffd);
		// Handle find error
		if (hFind == INVALID_HANDLE_VALUE) {
			std::cout << "Error: FindFirstFile" << std::endl;
			return false;
		}

		do { // loop through all files/ directories
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// suppress
			}
			else {
				// get file name
				const std::string file_loc = location + ffd.cFileName;

				// delete file
				if (!DeleteFile(file_loc.c_str())) {
					// handle delete error
					if (GetLastError() == ERROR_FILE_NOT_FOUND) {
						std::cout << "error: FILE_NOT_FOUND at " << file_loc << std::endl;
					}
					else if (GetLastError() == ERROR_ACCESS_DENIED) {
						std::cout << "error: ACCESS_DENIED at " << file_loc << std::endl;
					}
					else {
						std::cout << " ...error: " << GetLastError() << " at " << file_loc << std::endl;
					}
				}
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		// handle find error
		if (GetLastError() != ERROR_NO_MORE_FILES) {
			std::cout << "Error: FindNextFile" << std::endl;
			FindClose(hFind);
			return false;
		}

		FindClose(hFind);
		// ----------- End File Removal

		// Remove empty directory
		if (!RemoveDirectoryA(location.c_str())) {
			if (GetLastError() == ERROR_DIR_NOT_EMPTY) {
				std::cout << "File removal error: ERROR_DIR_NOT_EMPTY" << std::endl;
				return false;
			}
			else {
				std::cout << "File removal error: " << GetLastError() << std::endl;
				return false;
			}
		}

		// completed successfully
		std::cout << "done." << std::endl;
		return true;
	}


}

