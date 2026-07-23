CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -g

BUILD_DIR := build

SHELLX := $(BUILD_DIR)/shellx
TEST_PARSER := $(BUILD_DIR)/test_parser
TEST_EXECUTOR := $(BUILD_DIR)/test_executor
TEST_BUILTINS := $(BUILD_DIR)/test_builtins
TEST_PIPELINE := $(BUILD_DIR)/test_pipeline
TEST_JOBS := $(BUILD_DIR)/test_jobs

SHELLX_OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/executor.o $(BUILD_DIR)/builtins.o $(BUILD_DIR)/pipeline.o $(BUILD_DIR)/jobs.o
TEST_PARSER_OBJS := $(BUILD_DIR)/test_parser.o $(BUILD_DIR)/parser.o
TEST_EXECUTOR_OBJS := $(BUILD_DIR)/test_executor.o $(BUILD_DIR)/executor.o $(BUILD_DIR)/jobs.o
TEST_BUILTINS_OBJS := $(BUILD_DIR)/test_builtins.o $(BUILD_DIR)/builtins.o
TEST_PIPELINE_OBJS := $(BUILD_DIR)/test_pipeline.o $(BUILD_DIR)/pipeline.o $(BUILD_DIR)/executor.o $(BUILD_DIR)/jobs.o
TEST_JOBS_OBJS := $(BUILD_DIR)/test_jobs.o $(BUILD_DIR)/jobs.o $(BUILD_DIR)/parser.o

.PHONY: all test run clean rebuild

all: $(SHELLX)

$(SHELLX): $(SHELLX_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ -lreadline

$(TEST_PARSER): $(TEST_PARSER_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(TEST_EXECUTOR): $(TEST_EXECUTOR_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(TEST_BUILTINS): $(TEST_BUILTINS_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(TEST_PIPELINE): $(TEST_PIPELINE_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(TEST_JOBS): $(TEST_JOBS_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

$(BUILD_DIR)/main.o: src/main.c include/parser.h include/executor.h include/builtins.h include/pipeline.h include/shell.h include/jobs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: src/parser.c include/parser.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/executor.o: src/executor.c include/executor.h include/pipeline.h include/jobs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/builtins.o: src/builtins.c include/builtins.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/pipeline.o: src/pipeline.c include/pipeline.h include/executor.h include/jobs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/jobs.o: src/jobs.c include/jobs.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: tests/test_parser.c include/parser.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/test_executor.o: tests/test_executor.c include/executor.h include/pipeline.h include/jobs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/test_builtins.o: tests/test_builtins.c include/builtins.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/test_pipeline.o: tests/test_pipeline.c include/pipeline.h include/jobs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/test_jobs.o: tests/test_jobs.c include/jobs.h include/pipeline.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_PARSER) $(TEST_EXECUTOR) $(TEST_BUILTINS) $(TEST_PIPELINE) $(TEST_JOBS)
	./$(TEST_PARSER)
	./$(TEST_EXECUTOR)
	./$(TEST_BUILTINS)
	./$(TEST_PIPELINE)
	./$(TEST_JOBS)

run: $(SHELLX)
	./$(SHELLX)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all
