#include "tokenizer.h"
#include "log_utils.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "log/logs.h"


using Json = nlohmann::json;

const std::string BASE_TEXT = "[Tokenizer] ";

Tokenizer::Tokenizer():m_max_token_length(0), m_eos_token_id(32007) {

}

// 加载 tokenizer.json
bool Tokenizer::load(const std::string& tokenizer_json_path) {
    std::ifstream fin(tokenizer_json_path);
    if (!fin.is_open()) {
        LOG_ERROR(BASE_TEXT + "无法打开 tokenizer 文件" + tokenizer_json_path);
    }

    Json root = Json::parse(fin);
    if (!root.contains("model") || !root["model"].contains("vocab")) {
        LOG_ERROR(BASE_TEXT + "缺少model.vocab字段");
        return false;
    }

    Json vocab = root["model"]["vocab"];


    for (auto& [token, id] : vocab.items()) {
        if (!id.is_number()) continue;
        int tid = id.get<int>();
        m_max_token_length = std::max(m_max_token_length, token.size());
        m_token2id[token] = tid;
        m_id2token[tid] = token;
    }
    LOG_INFO(BASE_TEXT + "Max token length: " + std::to_string(m_max_token_length));

    // auto eos_it = m_token2id.find("</s>");
    // if (eos_it != m_token2id.end()) {
    //     m_eos_token_id = eos_it->second;
    //  LOG_INFO(BASE_TEXT + "eos_token_id = " + std::to_string(m_eos_token_id));
    // } else {
    //     LOG_WARN(BASE_TEXT + "未找到 </s> token，推理可能无法正确终止！");
    // }
    return true;
}

// 调用 python 文件实现编码
std::vector<int64_t> Tokenizer::encodeFromPython(const std::string& text, const std::string& tokenizer_path) const {
    std::vector<int64_t> token_ids;

    // 构造命令行：python3 encode.py <tokenizer_path> "<text>"
    std::string command = "python3 encode.py \"" + tokenizer_path + "\" \"" + text + "\"";
    // std::string command = "python3 ../encode.py \"" + tokenizer_path + "\" \"" + text + "\"";

    // 打开 pipe 读取子进程 stdout
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR(BASE_TEXT + "Failed to run python script.");
        return token_ids;
    }

    // 读取脚本输出
    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

    pclose(pipe);

    // 解析 JSON
    try {
        auto json = Json::parse(result);
        for (auto& id : json) {
            token_ids.push_back(id.get<int64_t>());
        }
    } catch (...) {
        LOG_ERROR(BASE_TEXT + "Failed to parse output: " + result);
    }

    return token_ids;
}

// 调用 python 文件实现解码
std::string Tokenizer::decodeFromPython(const std::vector<int64_t>& token_ids, const std::string& tokenizer_path) const {
    std::ostringstream oss;
    for (size_t i = 0; i < token_ids.size(); ++i) {
        oss << token_ids[i];
        if (i != token_ids.size() - 1) oss << ",";
    }

    // LOG_INFO(BASE_TEXT + oss.str());

    std::string command = "python3 decode.py \"" + tokenizer_path + "\" \"" + oss.str() + "\"";
    // std::string command = "python3 ../decode.py \"" + tokenizer_path + "\" \"" + oss.str() + "\"";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "[Decode Failed]";

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);
    return result;
}

std::string Tokenizer::decodeSingleTokenFromPython(int64_t token, const std::string& tokenizer_path) {
    std::ostringstream oss;
    oss << token;

    // LOG_INFO(BASE_TEXT + oss.str());

    std::string command = "python3 decode.py \"" + tokenizer_path + "\" \"" + oss.str() + "\"";
    // std::string command = "python3 ../decode.py \"" + tokenizer_path + "\" \"" + oss.str() + "\"";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "[Decode Failed]";

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);
    return result;
}

// 编码：文本 -> token_id列表
std::vector<int64_t> Tokenizer::encode(const std::string& text) const {
    std::vector<int64_t> token_ids;

    size_t i = 0;
    while (i < text.size()) {
        bool matched = false;

        // 贪心匹配（从长到短尝试匹配）
        for (size_t len = std::min(m_max_token_length, text.size() - i); len > 0; --len) {
            std::string sub = text.substr(i, len);
            auto it = m_token2id.find(sub);
            if (it != m_token2id.end()) {
                token_ids.push_back(it->second);
                i += len;
                matched = true;
                break;
            }
        }

        if (!matched) {
            auto unk = m_token2id.find("<unk>");
            token_ids.push_back((unk != m_token2id.end()) ? unk->second : 0);
            ++i;
        }
    }

    return token_ids;
}

// 解码：token_id列表 -> 文本
std::string Tokenizer::decode(const std::vector<int64_t>& token_ids) const {
    std::string result;

    for (int64_t id : token_ids) {
        auto it = m_id2token.find(id);
        if (it == m_id2token.end()) continue;

        const std::string& token = it->second;

        // 跳过控制符
        if (token == "<unk>" || token == "<s>" || token == "</s>") continue;

        // 处理形如<0x..>的token
        if (token.size() >= 5 && token.substr(0, 3) == "<0x" && token.back() == '>') {
            try {
                int byte = std::stoi(token.substr(3, token.size() - 4), nullptr, 16);
                if (byte >= 0 && byte <= 255) {
                    result += static_cast<char>(byte);
                    continue;
                }
            } catch (...) {
                // 非法hex不处理，继续拼接
            }
        }
        // 正常拼接token
        result += token;
    }

    return result;
}