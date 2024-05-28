#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>

// Define structure to hold function metadata
struct FunctionInfo {
    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
    std::vector<std::string> body;
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

// Function to parse function definitions and return the remaining code
std::vector<std::string> parseFunctions(const std::vector<std::string>& lines, std::unordered_map<std::string, FunctionInfo>& functions) {
    std::vector<std::string> remainingLines;
    std::regex functionPattern(R"(function\s+\[(.*)\]\s*=\s*(\w+)\((.*)\))");
    std::regex endPattern(R"(end\s*)");

    bool inFunction = false;
    std::string currentFunction;
    FunctionInfo currentFunctionInfo;

    for (const std::string& line : lines) {
        std::string trimmedLine = trim(removeComments(line));
        std::smatch match;

        if (std::regex_match(trimmedLine, match, functionPattern)) {
            inFunction = true;
            currentFunction = match[2];
            currentFunctionInfo.outputs = splitString(match[1], ",");
            currentFunctionInfo.inputs = splitString(match[3], ",");
        } else if (inFunction && std::regex_match(trimmedLine, match, endPattern)) {
            inFunction = false;
            functions[currentFunction] = currentFunctionInfo;
            currentFunctionInfo = FunctionInfo();
        } else if (inFunction) {
            currentFunctionInfo.body.push_back(trimmedLine);
        } else {
            remainingLines.push_back(line);
        }
    }

    return remainingLines;
}

// Function to replace function calls with function bodies
std::vector<std::string> replaceFunctionCalls(const std::vector<std::string>& lines, const std::unordered_map<std::string, FunctionInfo>& functions) {
    std::vector<std::string> replacedLines;
    std::regex functionCallPattern(R"(\[(.*)\]\s*:=\s*(\w+)\((.*)\))");

    for (const std::string& line : lines) {
        std::smatch match;
        if (std::regex_match(line, match, functionCallPattern)) {
            std::string outputVars = match[1];
            std::string functionName = match[2];
            std::string inputVars = match[3];

            auto funcIt = functions.find(functionName);
            if (funcIt != functions.end()) {
                const FunctionInfo& funcInfo = funcIt->second;

                std::vector<std::string> outputs = splitString(outputVars, ",");
                std::vector<std::string> inputs = splitString(inputVars, ",");

                for (const std::string& funcLine : funcInfo.body) {
                    std::string replacedLine = funcLine;
                    for (size_t i = 0; i < funcInfo.outputs.size(); ++i) {
                        replacedLine = std::regex_replace(replacedLine, std::regex(R"(\b)" + funcInfo.outputs[i] + R"(\b)"), trim(outputs[i]));
                    }
                    for (size_t i = 0; i < funcInfo.inputs.size(); ++i) {
                        replacedLine = std::regex_replace(replacedLine, std::regex(R"(\b)" + funcInfo.inputs[i] + R"(\b)"), trim(inputs[i]));
                    }
                    replacedLines.push_back(replacedLine);
                }
            } else {
                replacedLines.push_back(line);
            }
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

    // Parse function definitions
    std::unordered_map<std::string, FunctionInfo> functions;
    std::vector<std::string> remainingLines = parseFunctions(lines, functions);

    // Replace function calls with function bodies
    std::vector<std::string> processedLines = replaceFunctionCalls(remainingLines, functions);

    // Output the processed lines
    for (const auto& line : processedLines) {
        outfile << line << "\n";
    }

    return 0;
}
