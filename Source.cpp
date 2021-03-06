#include "Global.h"

#define BOOST_ASIO_SEPARATE_COMPILATION

#define MAX_BUFFER_SIZE 2048
#define DEFAULT_SCRIPT_DIRECTORY "\\source"

#pragma comment(lib, "Ws2_32.lib")

std::vector<Page> all_pages;
std::map<std::string, Instruction> static_data;
std::map<std::string, Session> all_user_sessions;

const std::string ip = "127.0.0.1";
const char* port = "2020";

u_int get_content_length(const std::vector<char> &request) {
	const std::string header = "Content-Length: ";
	std::string length = EMPTY;

	for (register u_int i = 0; i < request.size(); i++) {
		if (length != EMPTY) {
			break;
		}
		for (register u_int j = 0; j < header.length(); j++) {
			if (i + j < request.size() && header[j] != request[i + j]) {
				break;
			}
			else if (j == header.length() - 1) {
				j++;
				while (request[i + j] != '\n') {
					length += request[i + j];
					j++;
				}
				if (length[length.size() - 1] == '\r') {
					length[length.size() - 1] = '\x0';
				}
				break;
			}
		}
	}
	return atoi(length.c_str());
}

void client_handler(const SOCKET client_socket)
// Функция обработки клиентского запроса
{
	char buffer[MAX_BUFFER_SIZE];
	std::vector<char> temp;
	int content_length = 0;

	// Чтение клиентского запроса из потока
	int res = 0;
	do { 

		res = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
		for (register u_int i = 0; i < res; i++) {
			temp.push_back(buffer[i]);
		}
		if (content_length == 0) {
			content_length = get_content_length(temp);
			if (content_length < res) {
				break;
			}
		}
		else if (content_length > 0) {
			content_length -= res;
		}
	} while (content_length != 0);

	if (res != SOCKET_ERROR)
	{	
		// Файл, который запросил клиент
		std::string file_name = "/";
		bool isFound = false;

		for (register u_short i = 0; i < res && temp[i] != '\n'; i++)
		{
			if (temp[i] == '/')
			{
				for (register u_short j = i + 1; j < res && temp[j] != ' ' && temp[j] != '?'; j++)
					file_name += temp[j];
				if (file_name != "")
					isFound = true;
			}
			if (isFound)
				break;
		}
	

		std::string body = EMPTY;

		int all_pages_size = all_pages.size();
		register unsigned int i = 0; // -> ID файла в векторе

		for (; i < all_pages_size; i++)
		{
			if (all_pages[i].getName() == file_name)
			{
				// Вызов абстракции сервера на момент обработки клиенсткого запроса
				Server server(all_pages[i], temp, client_socket);
				break;
			}
			else if (i == all_pages_size - 1)
			{
				std::string response = "HTTP/1.1 404 Not Found";

				send(client_socket, response.c_str(), response.length(), 0);
				return;
			}
		}

		//std::cout << tmp e<< std::endl;
	}
	closesocket(client_socket);
}

size_t getSize(const char *text)
// Определение размера
{
	unsigned int counter = 0;
	while (text[counter] != '\0') counter++;
	return counter;
}

void loadPages(const std::string dir = "")
// Функция загружает все файлы в ОЗУ
{
	WIN32_FIND_DATA winFileData;
	HANDLE hFile;
	char szPath[MAX_PATH];
	if (GetCurrentDirectory(sizeof(szPath), szPath))
	{
		lstrcat(szPath, (DEFAULT_SCRIPT_DIRECTORY + dir).c_str());
		lstrcat(szPath, "\\*.*");

		hFile = FindFirstFile(szPath, &winFileData);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				size_t size = getSize(winFileData.cFileName);
				for (register unsigned int i = 0; i < size; i++)
				{
					if (winFileData.cFileName[0] == '.') break;
					if (winFileData.cFileName[i] == '.')
					{
						all_pages.push_back(Page{ dir + "/" + winFileData.cFileName });
						std::string s = dir + "/" + winFileData.cFileName;
						break;
					}
					else if (i == size - 1)
						loadPages(dir + "/" + winFileData.cFileName);
				}

			} while (FindNextFile(hFile, &winFileData) != 0);
			FindClose(hFile);
		}
	}
}

int main(int argc, char **argv[])
{
	setlocale(LC_NUMERIC, "C");
 

	std::cout << " __          # ______      # _______     # ______      # ______      #" << std::endl;
	std::cout << "/_/\\         #/_____/\\     #/______/\\    #/_____/\\     #/_____/\\     #" << std::endl;
	std::cout << "\\:\\ \\        #\\:::_ \\ \\    #\\::::__\\/__  #\\:::_ \\ \\    #\\::::_\\/_    #" << std::endl;
	std::cout << " \\:\\ \\       # \\:\\ \\ \\ \\   # \\:\\ /____/\\ # \\:\\ \\ \\ \\   # \\:\\/___/\\   #" << std::endl;
	std::cout << "  \\:\\ \\____  #  \\:\\ \\ \\ \\  #  \\:\\\\_  _\\/ #  \\:\\ \\ \\ \\  #  \\_::._\\:\\  #" << std::endl;
	std::cout << "   \\:\\/___/\\ #   \\:\\_\\ \\ \\ #   \\:\\_\\ \\ \\ #   \\:\\_\\ \\ \\ #    /____\\:\\ #" << std::endl;
	std::cout << "    \\_____\\/ #    \\_____\\/ #    \\_____\\/ #    \\_____\\/ #    \\_____\\/ #" << std::endl;
	std::cout << "\t\t\t\t\t\t\t[V 2.2]" << std::endl;
	std::cout << "\n\n" << std::endl;

	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

	std::cout << "IP: \t" << ip << std::endl;
	std::cout << "PORT: \t" << port << std::endl;

	std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
	std::cout << "Server initialization \t... ";

	//// Если адрес указан явно - изменить
	//if (argc > 0)
	//	ip = argv[0][0];

	// Инициализация библиотеки
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
	{
		std::cout << "\t failed to initialize library" << std::endl;
		WSACleanup();
		system("pause >> VOID");
		return 1;
	}

	// Инициализация основных структур
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	struct addrinfo* addr;
	struct addrinfo hints;

	ZeroMemory(&addr, sizeof(addr));
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Если инициализация не удалась - вывести сообщение об ошибке
	if (getaddrinfo(ip.c_str(), port, &hints, &addr) == SOCKET_ERROR)
	{
		std::cout << "\t не удалось инициализировать основные структуры" << std::endl;
		WSACleanup();
		system("pause >> VOID");
		return 1;
	}

	// Инициализация основного сокета
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SOCKET server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (server_socket == SOCKET_ERROR)
	{
		std::cout << "\t не удалось инициализировать основной сокет" << std::endl;
		WSACleanup();
		freeaddrinfo(addr);
		system("pause >> VOID");
		return 1;
	}

	// Привязка основного сокета
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (bind(server_socket, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
	{
		std::cout << "\t не удалось привязать основной сокет" << std::endl;
		WSACleanup();
		freeaddrinfo(addr);
		system("pause >> VOID");
		return 1;
	}

	std::cout << "\tOK" << std::endl;
	std::cout << "File upload \t\t... ";

	// Загрузить файлы
	loadPages();
	std::cout << "\tOK (" << all_pages.size() << ")" << std::endl;

	// Начать прослушку входящих TCP соединений
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::cout << "Server startup \t\t... ";
	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout <<  "\t не удалось начать прослушку входящих TCP соединений" << std::endl;
		WSACleanup();
		freeaddrinfo(addr);
		return 1;
	}

	std::cout << "\tServer started successfully!" << std::endl;
	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

	// Начать работу сервера
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	while (true)
	{
		struct sockaddr clientaddress;
		int size = sizeof(clientaddress);

		SOCKET client_socket = accept(server_socket, &clientaddress, &size);
		if (client_socket != INVALID_SOCKET)
		{
			// Запуск потока обработки клиентского запроса
			std::thread listen_client_thread(client_handler, client_socket);
			listen_client_thread.detach();
		}
	}

	system("pause >> VOID");
	return 0;
}