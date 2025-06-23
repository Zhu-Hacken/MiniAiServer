#pragma once
#include "sys_utils.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Tokenizer {
public:
    Tokenizer();
    // 加载 tokenizer.json
    bool load(const std::string& tokenizer_json_path);
    // 调用 python 文件实现编码
    std::vector<int64_t> encodeFromPython(const std::string& text, const std::string& tokenizer_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json") const;
    // 调用 python 文件实现解码
    std::string decodeFromPython(const std::vector<int64_t>& token_ids, const std::string& tokenizer_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json") const;
    // 调用 python 解码单个token
    std::string decodeSingleTokenFromPython(int64_t token, const std::string& tokenizer_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json");
    // 编码：文本 -> token_id列表
    std::vector<int64_t> encode(const std::string& text) const;
    // 解码：token_id列表 -> 文本
    std::string decode(const std::vector<int64_t>& token_ids) const;
    // 获取eos的id
    int getEosTokenId() {return m_eos_token_id;}
private:
    std::unordered_map<std::string, int> m_token2id;
    std::unordered_map<int, std::string> m_id2token;

    size_t m_max_token_length;
    int m_eos_token_id;
};