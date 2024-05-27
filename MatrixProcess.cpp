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

// Function to process matrix operations and format them
std::vector<std::string> formatMatrixOperations(const std::string& code, std::unordered_map<std::string, VariableInfo>& variableInfo) {
    std::vector<std::string> formattedLines;
    std::istringstream stream(code);
    std::string line;
    std::regex varAssignPattern(R"((\w+):=(.*))");

    while (std::getline(stream, line)) {
        line = trim(removeComments(line));
        std::smatch match;

        if (std::regex_match(line, match, varAssignPattern)) {
            std::string resultVar = match[1];
            std::string expression = match[2];

            std::regex scalarMultPattern(R"(([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");
            std::regex elemMultPattern(R"(([a-zA-Z0-9_]+)\s*\.\*\s*([a-zA-Z0-9_]+))");
            std::regex addPattern(R"(([a-zA-Z0-9_]+)\s*\+\s*([a-zA-Z0-9_]+))");

            // Match for scalar and matrix operations
            std::smatch subMatch;
            if (std::regex_search(expression, subMatch, scalarMultPattern)) {
                std::string op1 = subMatch[1];
                std::string op2 = subMatch[2];

                bool op1IsMatrix = variableInfo[op1].isMatrix;
                bool op2IsMatrix = variableInfo[op2].isMatrix;

                if (op1IsMatrix && op2IsMatrix) {
                    // Matrix multiplication
                    int rows = variableInfo[op1].rows;
                    int cols = variableInfo[op2].cols;
                    int innerDim = variableInfo[op1].cols;
                    variableInfo[resultVar] = VariableInfo(true, rows, cols);

                    for (int i = 1; i <= rows; ++i) {
                        for (int j = 1; j <= cols; ++j) {
                            std::string expr = "";
                            for (int k = 1; k <= innerDim; ++k) {
                                if (k > 1) expr += "+";
                                expr += op1 + std::to_string(i) + std::to_string(k) + "*" + op2 + std::to_string(k) + std::to_string(j);
                            }
                            formattedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + expr);
                        }
                    }
                } else if (op1IsMatrix || op2IsMatrix) {
                    // Scalar multiplication
                    std::string matrixVar = op1IsMatrix ? op1 : op2;
                    std::string scalarVar = op1IsMatrix ? op2 : op1;
                    int rows = variableInfo[matrixVar].rows;
                    int cols = variableInfo[matrixVar].cols;
                    variableInfo[resultVar] = VariableInfo(true, rows, cols);

                    for (int i = 1; i <= rows; ++i) {
                        for (int j = 1; j <= cols; ++j) {
                            formattedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + scalarVar + "*" + matrixVar + std::to_string(i) + std::to_string(j));
                        }
                    }
                } else {
                    // Scalar multiplication
                    formattedLines.push_back(resultVar + ":=" + op1 + "*" + op2);
                }
            } else if (std::regex_search(expression, subMatch, elemMultPattern)) {
                std::string op1 = subMatch[1];
                std::string op2 = subMatch[2];

                int rows = variableInfo[op1].rows;
                int cols = variableInfo[op1].cols;
                variableInfo[resultVar] = VariableInfo(true, rows, cols);

                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        formattedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + op1 + std::to_string(i) + std::to_string(j) + ".*" + op2 + std::to_string(i) + std::to_string(j));
                    }
                }
            } else if (std::regex_search(expression, subMatch, addPattern)) {
                std::string op1 = subMatch[1];
                std::string op2 = subMatch[2];

                int rows = variableInfo[op1].rows;
                int cols = variableInfo[op1].cols;
                variableInfo[resultVar] = VariableInfo(true, rows, cols);

                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        formattedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + op1 + std::to_string(i) + std::to_string(j) + "+" + op2 + std::to_string(i) + std::to_string(j));
                    }
                }
            } else {
                formattedLines.push_back(line);
            }
        } else {
            formattedLines.push_back(line);
        }
    }

    return formattedLines;
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

    // Define variable information based on expected matrix dimensions
    std::unordered_map<std::string, VariableInfo> variableInfo;
    // Example: populate with known matrix dimensions
    variableInfo["A"] = VariableInfo(true, 2, 3);
    variableInfo["B"] = VariableInfo(true, 2, 3);
    variableInfo["C"] = VariableInfo(true, 2, 3);
    variableInfo["D"] = VariableInfo(true, 2, 3);
    variableInfo["F"] = VariableInfo(true, 2, 2);

    auto formattedLines = formatMatrixOperations(code, variableInfo);

    // Write formatted lines to the output file
    for (const auto& line : formattedLines) {
        outfile << line << "\n";
    }

    return 0;
}
