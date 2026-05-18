#ifndef NOTEPAD_EXCEPTION_H
#define NOTEPAD_EXCEPTION_H

#include <exception>
#include <string>

class notepad_exception : public std::exception {
private:
    std::string message;

public:
    notepad_exception(const std::string &msg) : message(msg) {
    }

    const char *what() const noexcept override {
        return message.c_str();
    }
};

class file_not_found_exception : public notepad_exception {
public:
    file_not_found_exception(const std::string &filename)
        : notepad_exception("File not found: " + filename) {
    }
};

class file_read_exception : public notepad_exception {
public:
    file_read_exception(const std::string &filename)
        : notepad_exception("Cannot read file: " + filename) {
    }
};

class file_write_exception : public notepad_exception {
public:
    file_write_exception(const std::string &filename)
        : notepad_exception("Cannot write to file: " + filename) {
    }
};

#endif // NOTEPAD_EXCEPTION_H
