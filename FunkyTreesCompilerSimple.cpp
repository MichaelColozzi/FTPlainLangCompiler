#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <memory>
#include <unordered_map>

enum NodeType { VAR_ASSIGN };

struct ASTNode {
    NodeType type;
    std::string variable;
    std::string expression;
    std::string activator;

    ASTNode(NodeType t, const std::string& var, const std::string& expr, const std::string& act = "")
        : type(t), variable(var), expression(expr), activator(act) {}
};

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

std::string removeComments(const std::string& line) {
    size_t pos = line.find('%');
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

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

std::vector<std::string> preprocess(const std::string& code) {
    std::vector<std::string> expandedLines;
    std::istringstream stream(code);
    std::string line;
    std::regex zerosPattern(R"((\w+):=zeros\((\d+),(\d+)\))");
    std::regex onesPattern(R"((\w+):=ones\((\d+),(\d+)\))");
    std::regex eyePattern(R"((\w+):=eye\((\d+)\))");
    std::regex scalarMultPattern(R"((\w+):=([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");
    std::regex elemMultPattern(R"((\w+):=([a-zA-Z0-9_]+)\.\*\s*([a-zA-Z0-9_]+))");
    std::regex addPattern(R"((\w+):=([a-zA-Z0-9_]+)\s*\+\s*([a-zA-Z0-9_]+))");
    std::regex matMultPattern(R"((\w+):=([a-zA-Z0-9_]+)\s*\*\s*([a-zA-Z0-9_]+))");

    while (std::getline(stream, line)) {
        line = trim(removeComments(line));
        std::smatch match;

        if (std::regex_match(line, match, zerosPattern)) {
            std::string var = match[1];
            int rows = std::stoi(match[2]);
            int cols = std::stoi(match[3]);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j
<= cols; ++j) {
                    expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=0");
                }
            }
        } else if (std::regex_match(line, match, onesPattern)) {
            std::string var = match[1];
            int rows = std::stoi(match[2]);
            int cols = std::stoi(match[3]);
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(var + std::to_string(i) + std::to_string(j) + ":=1");
                }
            }
        } else if (std::regex_match(line, match, eyePattern)) {
            std::string var = match[1];
            int n = std::stoi(match[2]);
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
            std::string scalarVar = match[2];
            std::string matrixVar = match[3];
            int rows = 2; // Define rows based on your context or infer from inputs
            int cols = 2; // Define cols based on your context or infer from inputs
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + scalarVar + "*" + matrixVar + std::to_string(i) + std::to_string(j));
                }
            }
        } else if (std::regex_match(line, match, elemMultPattern)) {
            std::string resultVar = match[1];
            std::string matrixVarA = match[2];
            std::string matrixVarB = match[3];
            int rows = 2; // Define rows based on your context or infer from inputs
            int cols = 2; // Define cols based on your context or infer from inputs
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + matrixVarA + std::to_string(i) + std::to_string(j) + ".*" + matrixVarB + std::to_string(i) + std::to_string(j));
                }
            }
        } else if (std::regex_match(line, match, addPattern)) {
            std::string resultVar = match[1];
            std::string matrixVarA = match[2];
            std::string matrixVarB = match[3];
            int rows = 2; // Define rows based on your context or infer from inputs
            int cols = 2; // Define cols based on your context or infer from inputs
            for (int i = 1; i <= rows; ++i) {
                for (int j = 1; j <= cols; ++j) {
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + matrixVarA + std::to_string(i) + std::to_string(j) + "+" + matrixVarB + std::to_string(i) + std::to_string(j));
                }
            }
        } else if (std::regex_match(line, match, matMultPattern)) {
            std::string resultVar = match[1];
            std::string matrixVarA = match[2];
            std::string matrixVarB = match[3];
            int rowsA = 2; // Define rows of A based on your context or infer from inputs
            int colsA = 2; // Define cols of A (and rows of B) based on your context or infer from inputs
            int colsB = 2; // Define cols of B based on your context or infer from inputs
            for (int i = 1; i <= rowsA; ++i) {
                for (int j = 1; j <= colsB; ++j) {
                    std::string expression = "";
                    for (int k = 1; k <= colsA; ++k) {
                        if (k > 1) expression += "+";
                        expression += matrixVarA + std::to_string(i) + std::to_string(k) + "*" + matrixVarB + std::to_string(k) + std::to_string(j);
                    }
                    expandedLines.push_back(resultVar + std::to_string(i) + std::to_string(j) + ":=" + expression);
                }
            }
        } else {
            expandedLines.push_back(line); // No preprocessing needed for this line
        }
    }

    return expandedLines;
}

std::vector<std::shared_ptr<ASTNode>> parseCode(const std::string& code) {
    std::vector<std::shared_ptr<ASTNode>> nodes;
    auto lines = preprocess(code);

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

    auto nodes = parseCode(code);
    generateXML(nodes, outfile);

    return 0;
}



