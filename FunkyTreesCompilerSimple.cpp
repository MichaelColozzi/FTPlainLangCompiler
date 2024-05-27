#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <memory>
#include <unordered_map>

// Define enumeration for node types
enum NodeType { VAR_ASSIGN };

// Define structure for AST nodes
struct ASTNode {
    NodeType type;
    std::string variable;
    std::string expression;
    std::string activator;

    ASTNode(NodeType t, const std::string& var, const std::string& expr, const std::string& act = "")
        : type(t), variable(var), expression(expr), activator(act) {}
};

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

// Function to parse a single line of code into an AST node
std::shared_ptr<ASTNode> parseLine(const std::string& line) {
    std::regex varAssignRegex(R"(([a-zA-Z0-9_]+)\s*:=\s*(.*?)(\s*\$\s*(.*))?)");
    std::smatch match;

    if (std::regex_match(line, match, varAssignRegex)) {
        std::string variable = match[1].str();
        std::string expression = match[2].str();
        std::string activator = match[4].str();
        if (activator.empty()) {
            activator = "1";
        }
        return std::make_shared<ASTNode>(VAR_ASSIGN, variable, expression, activator);
    }

    return nullptr;
}

// Function to expand matrix operations into individual variable assignments
std::vector<std::string> preprocess(const std::string& code, std::unordered_map<std::string, VariableInfo>& variableInfo) {
    std::vector<std::string> expandedLines;
    std::istringstream stream(code);
    std::string line;
    std::regex zerosPattern(R"((\w+):=zeros\((\d+),(\d+)\))");
    std::regex onesPattern(R"((\w+):=ones\((\d+),(\d+)\))");
    std::regex eyePattern(R"((\w+):=eye\((\d+)\))");
    std::regex scalarMultPattern(R"((\w+):=([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");
    std::regex elemMultPattern(R"((\w+):=([a-zA-Z0-9_]+)\.\*\s*([a-zA-Z0-9_]+))");
    std::regex addPattern(R"((\w+):=([a-zA-Z0-9_]+)\s*\+\s*([a-zA-Z0-9_]+))");

    while (std::getline(stream, line)) {
        line = trim(removeComments(line));
        std::smatch match;

        if (std::regex_match(line, match, zerosPattern)) {
            std::string var = match[1];
            int rows = std::stoi(match[2]);
            int cols = std::stoi(match[3]);
            variableInfo[var] = VariableInfo(true, rows, cols);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=0");
                }
            }
        } else if (std::regex_match(line, match, onesPattern)) {
            std::string var = match[1];
            int rows = std::stoi(match[2]);
            int cols = std::stoi(match[3]);
            variableInfo[var] = VariableInfo(true, rows, cols);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=1");
                }
            }
        } else if (std::regex_match(line, match, eyePattern)) {
            std::string var = match[1];
            int n = std::stoi(match[2]);
            variableInfo[var] = VariableInfo(true, n, n);
            for (int i = 1; i <= n; ++i) {
                for (int j = 1; j <= n; ++j) {
                    if (i == j) {
                        expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=1");
                    } else {
                        expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=0");
                    }
                }
            }
        } else if (std::regex_match(line, match, scalarMultPattern)) {
            std::string resultVar = match[1];
            std::string varA = match[2];
            std::string varB = match[3];

            bool varAIsMatrix = variableInfo[varA].isMatrix;
            bool varBIsMatrix = variableInfo[varB].isMatrix;

            if (varAIsMatrix && varBIsMatrix) {
                // Matrix multiplication
                int rowsA = variableInfo[varA].rows;
                int colsA = variableInfo[varA].cols;
                int rowsB = variableInfo[varB].rows;
                int colsB = variableInfo[varB].cols;

                variableInfo[resultVar] = VariableInfo(true, rowsA, colsB);
                for (int i = 1; i <= rowsA; ++i) {
                    for (int j = 1; j <= colsB; ++j) {
                        std::string expression = "";
                        for (int k = 1; k <= colsA; ++k) {
                            if (k > 1) expression += "+";
                            expression += varA + std::to_string(i) + std::to_string(k) + "*" + varB + std::to_string(k) + std::to_string(j);
                        }
                        expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + expression);
                    }
                }
            } else if (varAIsMatrix) {
                // Scalar multiplication (scalar * matrix)
                int rows = variableInfo[varA].rows;
                int cols = variableInfo[varA].cols;
                variableInfo[resultVar] = VariableInfo(true, rows, cols);
                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + varA + std::to_string(i) + std::to_string(j) + "*" + varB);
                    }
                }
            } else if (varBIsMatrix) {
                // Scalar multiplication (scalar * matrix)
                int rows = variableInfo[varB].rows;
                int cols = variableInfo[varB].cols;
                variableInfo[resultVar] = VariableInfo(true, rows, cols);
                for (int i = 1; i <= rows; ++i) {
                    for (int j = 1; j <= cols; ++j) {
                        expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + varA + "*" + varB + std::to_string(i) + std::to_string(j));
                    }
                }
            }
        } else if (std::regex_match(line, match, elemMultPattern)) {
            std::string resultVar = match[1];
            std::string matrixVarA = match[2];
            std::string matrixVarB = match[3];

            int rows = variableInfo[matrixVarA].rows;
            int cols = variableInfo[matrixVarA].cols;
            variableInfo[resultVar] = VariableInfo(true, rows, cols);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + matrixVarA + std::to_string(i) + std::to_string(j) + ".*" + matrixVarB + std::to_string(i) + std::to_string(j));
                }
            }
        } else if (std::regex_match(line, match, addPattern)) {
            std::string resultVar = match[1];
            std::string matrixVarA = match[2];
            std::string matrixVarB = match[3];

            int rows = variableInfo[matrixVarA].rows;
            int cols = variableInfo[matrixVarA].cols;
            variableInfo[resultVar] = VariableInfo(true, rows, cols);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + matrixVarA + std::to_string(i) + std::to_string(j) + "+" + matrixVarB + std::to_string(i) + std::to_string(j));
                }
            }
        } else {
            expandedLines.push_back(line); // No preprocessing needed for this line
        }
    }

    return expandedLines;
}

std::vector<std::shared_ptr<ASTNode>> parseCode(const std::string& code, std::unordered_map<std::string, VariableInfo>& variableInfo) {
    std::vector<std::shared_ptr<ASTNode>> nodes;
    auto lines = preprocess(code, variableInfo);

    for (const auto& line : lines) {
        auto node = parseLine(line);
        if (node) {
            nodes.push_back(node);
        }
    }

    return nodes;
}

void generateXML(const std::vector<std::shared_ptr<ASTNode>>& nodes, std::ostream& out) {
    out << "<Variables>\n";
    for (const auto& node : nodes) {
        if (node->type == VAR_ASSIGN) {
            out << "    <Setter variable=\"" << node->variable << "\" function=\"" << node->expression << "\" activator=\"" << node->activator << "\" priority=\"0\" />\n";
        }
    }
    out << "</Variables>\n";
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

    std::unordered_map<std::string, VariableInfo> variableInfo;
    auto nodes = parseCode(code, variableInfo);
    generateXML(nodes, outfile);

    return 0;
}