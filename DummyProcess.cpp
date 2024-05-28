#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>

// Function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// Function to remove comments from a line of code
std::string removeComments(const std::string& line) {
    size_t pos = line.find('%');
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

// Function to split a string by a delimiter
std::vector<std::string> splitString(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

// Function to join a vector of strings into a single string with a delimiter
std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::ostringstream os;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i != 0) {
            os << delimiter;
        }
        os << strings[i];
    }
    return os.str();
}

// Function to replace nested function calls with dummy variables
std::vector<std::string> replaceNestedFunctionCalls(const std::vector<std::string>& lines, int& dummyCounter) {
    std::vector<std::string> replacedLines;
    std::regex functionCallPattern(R"(\[(.*)\]\s*:=\s*(\w+)\((.*)\))");
    std::regex nestedFunctionCallPattern(R"(\[(.*)\]\s*:=\s*(\w+)\((\w+\(.*\))\))");

    for (const std::string& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, nestedFunctionCallPattern)) {
            std::string outputVars = match[1];
            std::string outerFunction = match[2];
            std::string innerFunctionCall = match[3];

            // Extract inner function details
            auto innerMatch = std::smatch();
            std::regex_match(innerFunctionCall, innerMatch, functionCallPattern);
            std::string innerOutputVars = innerMatch[1];
            std::string innerFunction = innerMatch[2];
            std::string innerInputVars = innerMatch[3];

            // Generate dummy variables for the inner function outputs
            std::vector<std::string> innerOutputs = splitString(innerOutputVars, ",");
            std::vector<std::string> dummyVars(innerOutputs.size());
            for (size_t i = 0; i < dummyVars.size(); ++i) {
                dummyVars[i] = "dummy" + std::to_string(dummyCounter++);
            }

            // Create lines for the inner and outer function calls
            std::string dummyLine = "[" + joinStrings(dummyVars, ", ") + "] := " + innerFunction + "(" + innerInputVars + ")";
            std::string outerLine = "[" + outputVars + "] := " + outerFunction + "(" + joinStrings(dummyVars, ", ") + ")";

            // Insert the new lines in place of the original nested call
            replacedLines.push_back(dummyLine);
            replacedLines.push_back(outerLine);
        } else {
            replacedLines.push_back(line);
        }
    }

    return replacedLines;
}

// Main function for processing
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file: " << argv[1] << "\n";
        return 1;
    }

    std::ofstream outfile(argv[2]);
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << argv[2] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string code = buffer.str();

    // Split the code into lines
    std::istringstream codeStream(code);
    std::vector<std::string> lines;
    std::string tempLine;
    while (std::getline(codeStream, tempLine)) {
        lines.push_back(tempLine);
    }

    // Replace nested function calls with dummy variables
    int dummyCounter = 0;
    std::vector<std::string> replacedLines = replaceNestedFunctionCalls(lines, dummyCounter);

    // Output the processed lines
    for (const auto& line : replacedLines) {
        outfile << line << "\n";
    }

    return 0;
}




