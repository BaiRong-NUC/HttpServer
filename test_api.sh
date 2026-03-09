# 指定构建路径并编译
cmake -B build -S .
cmake --build build
cd build

# # 自动递归执行 ./build/api_test/ 下所有可执行程序
# API_DIR="./api_test/"
# find "$API_DIR" -type f -perm -111 | while read exe; do
#     echo "运行: $exe"
#     "$exe"
#     echo "----------------------"
# done

