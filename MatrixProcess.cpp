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

// Function to convert dummy variable assignments to direct assignments
std::vector<std::string> convertDummyAssignments(const std::vector<std::string>& lines) {
    std::vector<std::string> convertedLines;
    std::unordered_map<std::string, std::string> dummyAssignments;
    std::regex assignPattern(R"((\w+):=(.*))");

    for (const std::string& line : lines) {
        std::smatch match;
        if (std::regex_match(line, match, assignPattern)) {
            std::string var = match[1];
            std::string expr = match[2];
            if (dummyAssignments.find(expr) != dummyAssignments.end()) {
                // Replace dummy variable with actual expression
                convertedLines.push_back(var + ":=" + dummyAssignments[expr]);
            } else {
                convertedLines.push_back(line);
                dummyAssignments[var] = expr;
            }
        } else {
            convertedLines.push_back(line);
        }
    }

    return convertedLines;
}

// Function to format matrix operations including slicing, indexing, and initialization
std::vector<std::string> formatMatrixOperations(const std::vector<std::string>& lines, std::unordered_map<std::string, VariableInfo>& variableInfo) {
    std::vector<std::string> formattedLines;
    std::regex varAssignPattern(R"((\w+):=(.*))");

    for (const std::string& line : lines) {
        std::string trimmedLine = trim(removeComments(line));
        std::smatch match;

        if (std::regex_match(trimmedLine, match, varAssignPattern)) {
            std::string resultVar = match[1];
            std::string expression = match[2];

            std::regex zerosPattern(R"((\w+):=zeros\((\d+),(\d+)\))");
            std::regex onesPattern(R"((\w+):=ones\((\d+),(\d+)\))");
            std::regex eyePattern(R"((\w+):=eye\((\d+)\))");
            std::regex scalarMultPattern(R"(([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");
            std::regex elemMultPattern(R"(([a-zA-Z0-9_]+)\s*\.\*\s*([a-zA-Z0-9_]+))");
            std::regex addPattern(R"(([a-zA-Z0-9_]+)\s*\+\s*([a-zA-Z0-9_]+))");
            std::regex indexPattern(R"((\w+)\((\d+),(\d+)\))");
            std::regex rowSlicePattern(R"((\w+)\((\d+),:\))");
            std::regex colSlicePattern(R"((\w+)\(:,(\d+)\))");
            std::regex submatrixSlicePattern(R"((\w+)\((\d+):(\d+),(\d+):(\d+)\))");
            std::regex matrixAssignPattern(R"((\w+):=(\w+))");

            // Match for matrix initialization
            std::smatch subMatch;
            if (std::regex_search(trimmedLine, match, zerosPattern)) {
                std::string var = match[1];
                int rows = std::stoi(match[2]);
                int cols = std::stoi(match[3]);
                variableInfo[var] = VariableInfo(true, rows, cols);
                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        formattedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=0");
                    }
                }
            } else if (std::regex_search(trimmedLine, match, onesPattern)) {
                std::string var = match[1];
                int rows = std::stoi(match[2]);
                int cols = std::stoi(match[3]);
                variableInfo[var] = VariableInfo(true, rows, cols);
                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        formattedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=1");
                    }
                }
            } else if (std::regex_search(trimmedLine, match, eyePattern)) {
                std::string var = match[1];
                int n = std::stoi(match[2]);
                variableInfo[var] = VariableInfo(true, n, n);
                for (int i = 1; i <= n; ++i) {
                    for (int j = 1; j <= n; ++j) {
                        if (i == j) {
                            formattedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=1");
                        } else {
                            formattedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=0");
                        }
                    }
                }
            } else if (std::regex_search(expression, subMatch, indexPattern)) {
                std::string matrixVar = subMatch[1];
                int row = std::stoi(subMatch[2]);
                int col = std::stoi(subMatch[3]);

                formattedLines.push_back(resultVar + ":=" + matrixVar + std::to_string(row) + std::to_string(col));
            } else if (std::regex_search(expression, subMatch, rowSlicePattern)) {
                std::string matrixVar = subMatch[1];
                int row = std::stoi(subMatch[2]);
                int cols = variableInfo[matrixVar].cols;

                for (int j = 1; j <= cols; ++j) {
                    formattedLines.push_back(resultVar + std::to_string(1) + std::to_string(j) + ":=" + matrixVar + std::to_string(row) + std::to_string(j));
                }
            } else if (std::regex_search(expression, subMatch, colSlicePattern)) {
                std::string matrixVar = subMatch[1];
                int col = std::stoi(subMatch[2]);
                int rows = variableInfo[matrixVar].rows;

                for (int i = 1; i <= rows; ++i) {
                    formattedLines.push_back(resultVar + std::to_string(i) + std::to_string(1) + ":=" + matrixVar + std::to_string(i) + std::to_string(col));
                }
            } else if (std::regex_search(expression, subMatch, submatrixSlicePattern)) {
                std::string matrixVar = subMatch[1];
                int rowStart = std::stoi(subMatch[2]);
                int rowEnd = std::stoi(subMatch[3]);
                int colStart = std::stoi(subMatch[4]);
                int colEnd = std::stoi(subMatch[5]);

                int rowCounter = 1;
                for (int i = rowStart; i <= rowEnd; ++i) {
                    int colCounter = 1;
                    for (int j = colStart; j <= colEnd; ++j) {
                        formattedLines.push_back(resultVar + std::to_string(rowCounter) + std::to_string(colCounter) + ":=" + matrixVar + std::to_string(i) + std::to_string(j));
                        colCounter++;
                    }
                    rowCounter++;
                }
            } else if (std::regex_search(expression, subMatch, scalarMultPattern)) {
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

// Function to perform the final pass for expanding matrix assignments
std::vector<std::string> expandMatrixAssignments(const std::vector<std::string>& formattedLines, std::unordered_map<std::string, VariableInfo>& variableInfo) {
    std::vector<std::string> expandedLines;
    std::regex matrixAssignPattern(R"((\w+):=(\w+))");

    for (const std::string& line : formattedLines) {
        std::smatch match;
        if (std::regex_match(line, match, matrixAssignPattern)) {
            std::string destVar = match[1];
            std::string sourceVar = match[2];

            auto destInfoIt = variableInfo.find(destVar);
            auto sourceInfoIt = variableInfo.find(sourceVar);

            if (destInfoIt != variableInfo.end() && sourceInfoIt != variableInfo.end() && destInfoIt->second.isMatrix && sourceInfoIt->second.isMatrix) {
                int rows = sourceInfoIt->second.rows;
                int cols = sourceInfoIt->second.cols;

                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        expandedLines.push_back(destVar + std::to_string(i) + std::to_string(j) + ":=" + sourceVar + std::to_string(i) + std::to_string(j));
                    }
                }
            } else {
                expandedLines.push_back(line);
            }
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

    // Split the code into lines
    std::istringstream codeStream(code);
    std::vector<std::string> lines;
    std::string tempLine;
    while (std::getline(codeStream, tempLine)) {
        lines.push_back(tempLine);
    }

    // Define variable information based on expected matrix dimensions
    std::unordered_map<std::string, VariableInfo> variableInfo;

    // First, convert dummy assignments to direct assignments
    std::vector<std::string> convertedLines = convertDummyAssignments(lines);
    auto formattedLines = formatMatrixOperations(convertedLines, variableInfo);
    auto expandedLines = expandMatrixAssignments(formattedLines, variableInfo);

    // Write expanded lines to the output file
    for (const auto& line : expandedLines) {
        outfile << line << "\n";
    }

    return 0;
}





