TARGET = cfeeny

SRC_DIR = ./src
SRC_SUBDIR += . utils compiler vm
OBJ_DIR = .

CC = gcc
C_FLAGS = -g -std=gnu11 -O3 -Wno-int-to-void-pointer-cast

SRCS += ${foreach subdir, $(SRC_SUBDIR), ${wildcard $(SRC_DIR)/$(subdir)/*.[cs]}}

$(TARGET): $(SRCS)
	$(CC) $(C_FLAGS) $^ -o $@

.PHONY : clean
clean :
	rm $(TARGET)

