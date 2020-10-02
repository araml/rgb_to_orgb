CC=g++

FLAGS=-std=c++2a -ggdb # -Wall -Wunused-function -Wextra -Wundef

INCLUDE_DIRS =
INCLUDE= $(foreach p, $(INCLUDE_DIRS), -Isrc/$p)
INCLUDE =

SRC = scopic_task.cpp orgb_matrix.cpp

OBJS = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(SRC))))
FOLDERS = $(sort $(addprefix $(BUILD)/, $(dir $(SRC))))

BUILD = build

.PHONY: task clean tests
all: task
tests: compile_and_run_tests

$(BUILD)/%.o: %.cpp
	$(CC) -c $^ -o $@ $(INCLUDE) $(FLAGS)

$(FOLDERS):
	mkdir -p build

task: $(FOLDERS) $(OBJS)
	$(CC) $(OBJS) $(LIBS) $(FLAGS) $(INCLUDE) -o build/task

compile_and_run_tests:
	$(CC) test_matrix.cpp orgb_matrix.cpp -lcmocka $(INCLUDE) $(FLAGS) -o build/tests

clean:
	rm -rf build

vpath %.cpp src

