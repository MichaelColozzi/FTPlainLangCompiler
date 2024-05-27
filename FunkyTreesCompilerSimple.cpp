#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <memory>

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

std::vector<std::shared_ptr<ASTNode>> parseCode(const std::string& code) {
    std::vector<std::shared_ptr<ASTNode>> nodes;
    std::istringstream stream(code);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim(removeComments(line));
        if (line.empty()) continue;

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


