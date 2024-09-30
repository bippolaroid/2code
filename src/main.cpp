#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Global variable to handle shutdown
volatile std::sig_atomic_t shutdown = false;

// Array of folders and files to ignore
const std::vector<std::string> ignoredFolders = {
    ".cache",
    "build",
    "CMakeFiles",
    "contents",
    ".git"
};

const std::vector<std::string> ignoredFiles = {
    "CMakeCXXCompilerId.cpp",
};

// Array of allowed file extensions to show (e.g., ".cpp", ".h", etc.)
const std::vector<std::string> allowedExtensions = {
    ".cpp",
    ".h",
};

// Function to check if a folder should be ignored
bool shouldIgnoreFolder(const fs::path &folder) {
  for (const auto &ignored : ignoredFolders) {
    if (folder.filename() == ignored) {
      return true;
    }
  }
  return false;
}

// Function to check if a file should be ignored
bool shouldIgnoreFile(const fs::path &file) {
  for (const auto &ignored : ignoredFiles) {
    if (file.filename() == ignored) {
      return true;
    }
  }
  return false;
}

// Function to check if a file has an allowed extension
bool hasAllowedExtension(const fs::path &file) {
  for (const auto &ext : allowedExtensions) {
    if (file.extension() == ext) {
      return true;
    }
  }
  return false;
}

// Function to print the directory tree to the console, skipping ignored folders
// and showing only allowed file types
void printDirTree(const fs::path &path, int indent = 0) {
  for (const auto &entry : fs::directory_iterator(path)) {
    // Skip ignored folders
    if (entry.is_directory() && shouldIgnoreFolder(entry.path())) {
      continue;
    }

    // Skip ignored files and files without allowed extensions
    if (entry.is_regular_file() && (shouldIgnoreFile(entry.path()) ||
                                    !hasAllowedExtension(entry.path()))) {
      continue;
    }

    // Print directory structure with indentation
    for (int i = 0; i < indent; i++)
      std::cout << "-";

    if (entry.is_directory()) {
      std::cout << " + " << entry.path().filename() << std::endl;
      printDirTree(entry.path(), indent + 2);
    } else if (entry.is_regular_file()) {
      std::cout << " | " << entry.path().filename() << std::endl;
    }
  }
}

// Function to get formatted date and time as a string
std::string getCurrentDateTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_time = *std::localtime(&now_time);

  std::ostringstream oss;
  oss << std::put_time(&local_time, "contents__%Y-%m-%d__%H-%M-%S.txt");
  return oss.str();
}

// Function to log the contents of all files with allowed extensions in a
// directory tree, skipping ignored folders and files
void logFileContents(const fs::path &path, const fs::path &outputFolder) {
  // Create contents folder if it doesn't exist
  if (!fs::exists(outputFolder)) {
    fs::create_directory(outputFolder);
  }

  // Build the full path for the output file
  fs::path outputFile = outputFolder / getCurrentDateTime();
  std::ofstream output(outputFile, std::ios::out | std::ios::trunc);

  if (!output.is_open()) {
    std::cerr << "Error: Unable to open " << outputFile << std::endl;
    return;
  }

  for (const auto &entry : fs::recursive_directory_iterator(path)) {
    // Skip ignored folders
    if (entry.is_directory() && shouldIgnoreFolder(entry.path())) {
      continue;
    }

    // Skip ignored files and files without allowed extensions
    if (entry.is_regular_file() && (shouldIgnoreFile(entry.path()) ||
                                    !hasAllowedExtension(entry.path()))) {
      continue;
    }

    // Log contents of files with allowed extensions
    if (entry.is_regular_file() && hasAllowedExtension(entry.path())) {
      std::ifstream file(entry.path());
      if (file.is_open()) {
        output << "File: " << entry.path().filename() << "\n";
        output << std::string((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>())
               << "\n\n";
      }
    }
  }

  output.close();
  std::cout << "Log created: " << outputFile.string() << std::endl;
}

// Signal handler to perform graceful shutdown
void signalHandler(int signum) {
  std::cout << "\nReceived interrupt signal (" << signum
            << "). Shutting down gracefully..." << std::endl;
  shutdown = true;
}

// Function to display ASCII art styled menu
void displayMenu() {
  std::cout << "\n"
            << "=================================\n"
            << "              2code              \n"
            << "          (C) 2024 0xB           \n"
            << "=================================\n\n"
            << "  Options:                       \n"
            << "    1 - Show Directory Tree      \n"
            << "    2 - Log File Contents        \n"
            << "    0 - Quit                     \n\n"
            << "=================================\n\n"
            << "Please choose an option: ";
}

// Main function
int main() {
  // Register signal handler for graceful shutdown
  std::signal(SIGINT, signalHandler);

  // Use the current directory as the root path
  fs::path rootPath = fs::current_path(); // Get current directory
  fs::path outputFolder =
      rootPath / "contents"; // Create contents folder in the current directory

  while (!shutdown) {
    displayMenu();

    int command;
    std::cin >> command;

    if (shutdown)
      break; // Exit loop if shutdown signal is received

    switch (command) {
    case 0:
      shutdown = true;
      break;
    case 1:
      std::cout << "\nDirectory Tree for: " << rootPath << "\n";
      printDirTree(rootPath);
      break;
    case 2:
      std::cout << "\nLogging file contents for allowed file types...\n";
      logFileContents(rootPath, outputFolder);
      break;
    default:
      std::cout << "Invalid option. Please choose again." << std::endl;
    }
  }

  std::cout << "Program exited successfully." << std::endl;
  return 0;
}
