# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# 目录设置
BUILD_DIR = build
SRC_DIR = .

# 源文件和目标文件
CLIENT_SRC = client.cpp
SERVER_SRC = server.cpp
CLIENT_TARGET = $(BUILD_DIR)/client
SERVER_TARGET = $(BUILD_DIR)/server

# 默认目标
all: $(BUILD_DIR) $(CLIENT_TARGET) $(SERVER_TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译客户端
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

# 编译服务器
$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

# 运行目标（先运行服务器，再运行客户端）
.PHONY: run
run: all
	@echo "Starting server..."
	@$(SERVER_TARGET) &
	@sleep 1
	@echo "Starting client..."
	@$(CLIENT_TARGET)

# 清理目标
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
