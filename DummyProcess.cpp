#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>

// Define structure to hold variable metadata
struct VariableInfo {
    bool isMatrix;
    int rows;
    int cols;

    VariableInfo(bool isMat = false, int r = 0, int c = 0) : isMatrix(isMat), rows(r), cols(c) {}
};

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

// Function to escape regex special characters
std::string escapeRegex(const std::string& str) {
    static const std::regex specialChars(R"([-[\]{}()*+?.,\^$|#\s])");
    return std::regex_replace(str, specialChars, R"(\$&)");
}

// Function to preprocess high-level code into low-level code with dummy variables
std::vector<std::string> preprocess(const std::string& code) {
    std::vector<std::string> expandedLines;
    std::istringstream stream(code);
    std::string line;
    std::regex scalarMultPattern(R"(([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");
    std::regex elemMultPattern(R"(([a-zA-Z0-9_]+)\s*\.\*\s*([a-zA-Z0-9_]+))");
    std::regex addPattern(R"(([a-zA-Z0-9_]+)\s*\+\s*([a-zA-Z0-9_]+))");

    int dummyCounter = 0;

    while (std::getline(stream, line)) {
        line = trim(removeComments(line));
        std::smatch match;

        // Preprocess composed operations
        std::regex composedOpPattern(R"((\w+):=\s*(.+))");
        if (std::regex_match(line, match, composedOpPattern)) {
            std::string resultVar = match[1];
            std::string expression = match[2];

            // Replace all composed operations with dummy variables
            std::smatch subMatch;
            while (std::regex_search(expression, subMatch, scalarMultPattern) ||
                   std::regex_search(expression, subMatch, elemMultPattern) ||
                   std::regex_search(expression, subMatch, addPattern)) {
                std::string op1 = subMatch[1];
                std::string op2 = subMatch[2];
                std::string op = subMatch[0];

                std::string dummyVar = "dummy" + std::to_string(dummyCounter++);
                std::string dummyAssign = dummyVar + ":=" + op;

                expandedLines.push_back(dummyAssign);

                expression = std::regex_replace(expression, std::regex(escapeRegex(op)), dummyVar);
            }

            expandedLines.push_back(resultVar + ":=" + expression);
        } else {
            expandedLines.push_back(line);
        }
    }

    return expandedLines;
}

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

    auto expandedLines = preprocess(code);

    // Write expanded lines to the output file
    for (const auto& line : expandedLines) {
        outfile << line << "\n";
    }

    return 0;
}

