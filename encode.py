import sys
import json
from tokenizers import Tokenizer

# argv[1]: tokenizer.json 路径
# argv[2]: 待编码文本

if len(sys.argv) != 3:
    print("[]")
    sys.exit(1)

tokenizer_path = sys.argv[1]
text = sys.argv[2]

# 加载tokenizer.json
tokenizer = Tokenizer.from_file(tokenizer_path)
# tokenizer = Tokenizer.from_file("model/phi3/tokenizer.json")

# 读取文本
# text = "hi"

# 执行编码
encoded = tokenizer.encode(text)

# 输出 JSON 格式的 token ids
print(json.dumps(encoded.ids))