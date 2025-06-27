import sys
import json
from tokenizers import Tokenizer

# argv[1]: tokenizer.json 路径
# argv[2]: 逗号分隔的 token id 字符串

tokenizer = Tokenizer.from_file(sys.argv[1])
ids = list(map(int, sys.argv[2].split(",")))

# tokenizer = Tokenizer.from_file(sys.argv[1])
# token_id = int(sys.argv[2])

# decoded = tokenizer.decode([token_id])
# print(decoded, end="")

decoded = tokenizer.decode(ids)
print(decoded, end="")