NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g3 -std=c++98 -g3

SRC = config/config.cpp serv_cgi/cgi_hdr_Tout.cpp serv_cgi/client_io.cpp serv_cgi/http_handler.cpp\
 serv_cgi/main.cpp serv_cgi/server_utils.cpp serv_cgi/server.cpp\
 methods/getMethod.cpp methods/postMethod.cpp \
 methods/deleteMethod.cpp parsingRequest/parsing_request.cpp\
 Response/response.cpp serv_cgi/client_class.cpp

OBJ = ${SRC:.cpp=.o}

all: ${NAME}

%.o:%.cpp
	${CXX} ${CXXFLAGS} -c $< -o $@

${NAME}: ${OBJ}
	${CXX} ${CXXFLAGS} ${OBJ} -o ${NAME}

clean: 
	rm -f ${OBJ}

fclean: clean
	rm -f ${NAME}

re: fclean all
