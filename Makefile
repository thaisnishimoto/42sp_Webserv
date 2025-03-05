NAME = webserv

#flags
CC = c++
CPPFLAGS = -std=c++98 -Wall -Wextra -Werror
DEPFLAGS = -MMD -MP

#directories
SRCS_DIR = src/
INC = -I$(CURDIR)/include/
OBJS_DIR = objs/

#source files
SRC_FILES = main \
			WebServer \
			Logger \
			Request \
			Response \
			Connection \
			WebServerCgiHandlers \
			Cgi \
			Config \
			VirtualServer \
			Location \
			utils


SRCS = $(addprefix $(SRCS_DIR), $(addsuffix .cpp, $(SRC_FILES)))
OBJS = $(addprefix $(OBJS_DIR), $(addsuffix .o, $(basename $(SRC_FILES).cpp)))
DEP = $(OBJS:.o=.d)

#rules
all: $(NAME)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp | $(OBJS_DIR)
	$(CC) $(CPPFLAGS) $(DEPFLAGS) $(INC) -c $< -o $@

-include $(DEP)

$(NAME): $(OBJS)
	@mkdir -p content/upload
	$(CC) $(CPPFLAGS) $(INC) -o $(NAME) $(OBJS)

fd: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --track-fds=yes ./$(NAME) $(ARG)

run:
	@make
	./$(NAME) "config/basic.conf"

clean:
	rm -rf $(OBJS_DIR)

tclean: clean
	rm -f $(TEST_NAME)

fclean: clean tclean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean tclean run
