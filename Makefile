NAME = web_server

#flags
CC = c++
CPPFLAGS = -std=c++98 -Wall -Wextra -Werror -g3
DEPFLAGS = -MMD -MP
TESTFLAGS = -std=c++11 -Wall -Wextra -Werror -I/usr/include/gtest
TEST_NAME = test_bin

#directories
SRCS_DIR = src/
INC = -I$(CURDIR)/include/
TEST_DIR = test/
OBJS_DIR = objs/

#source files without the main file
SRC_FILES = main.cpp \
			WebServer \
			VirtualServer \
			Request

#test files
TEST_FILES = test_main test_config test_utils

#files with prefixs and suffixs
	#main files
SRC_WITH_MAIN = $(SRC_FILES)
SRCS = $(addprefix $(SRCS_DIR), $(addsuffix .cpp, $(SRC_WITH_MAIN)))
	#test files
SRC_WITHOUT_MAIN = $(addprefix $(SRCS_DIR), $(addsuffix .cpp, $(SRC_FILES)))
TEST_SRCS = $(addprefix $(TEST_DIR),  $(addsuffix .cpp, $(TEST_FILES)))

#objs and deps files
OBJS = $(addprefix $(OBJS_DIR), $(addsuffix .o, $(basename $(SRC_WITH_MAIN).cpp)))
DEP = $(OBJS:.o=.d)

#rules
all: $(NAME)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp | $(OBJS_DIR)
	$(CC) $(CPPFLAGS) $(DEPFLAGS) $(INC) -c $< -o $@

-include $(DEP)

$(NAME): $(OBJS)
	$(CC) $(CPPFLAGS) $(INC) -o $(NAME) $(OBJS)

fd: $(NAME)
	valgrind --track-fds=yes ./$(NAME)

test:
	docker build -t cpp-gtest .
	docker run -it -v $(CURDIR):/app cpp-gtest make -C /app test_inside

test_inside:
	$(CC) $(TESTFLAGS) $(INC) $(SRC_WITHOUT_MAIN) $(TEST_SRCS) -o $(TEST_NAME) -lgtest -lgtest_main -pthread
	./$(TEST_NAME)

clean:
	rm -rf $(OBJS_DIR)

tclean: clean
	rm -f $(TEST_NAME)

fclean: clean tclean
	rm -f $(NAME)

re: fclean all

.PHONY: all test test_inside clean fclean tclean
