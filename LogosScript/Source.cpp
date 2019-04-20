#include "Global.h"

#define BOOST_ASIO_SEPARATE_COMPILATION

#define MAX_BUFFER_SIZE 1024
#define DEFAULT_SCRIPT_DIRECTORY "\\source"

#pragma comment(lib, "Ws2_32.lib")

std::vector<Page> all_pages;
std::map<std::string, Instruction> static_data;
std::map<std::string, Session> all_user_sessions;

const std::string ip = "127.0.0.1";
const char* port = "2020";

void client_handler(const SOCKET client_socket)
// Функция обработки клиентского запроса
{
	char buffer[MAX_BUFFER_SIZE];

	// Чтение клиентского запроса из потока
	int res = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);

	if (res != SOCKET_ERROR)
	{
		std::string temp(buffer);
		temp[res] = '\0';
		
		// Файл, который запросил клиент
		std::string file_name = "/";
		bool isFound = false;

		for (register unsigned short i = 0; i < res && temp[i] != '\n'; i++)
		{
			if (temp[i] == '/')
			{
				for (register unsigned short j = i + 1; j < res && temp[j] != ' ' && temp[j] != '?'; j++)
					file_name += temp[j];
				if (file_name != "")
					isFound = true;
			}
			if (isFound)
				break;
		}
	

		std::string body = "";

		int all_pages_size = all_pages.size();
		register unsigned int i = 0; // -> ID файла в векторе

		for (; i < all_pages_size; i++)
		{
			if (all_pages[i].getName() == file_name)
			{
				int page_all_lines_size = all_pages[i].all_lines.size();
				for (register unsigned int j = 0; j < page_all_lines_size; j++)
					body += all_pages[i].all_lines[j];
				break;
			}
			else if (i == all_pages_size - 1)
			{
				std::string response = "HTTP/1.1 404 Not Found";

				send(client_socket, response.c_str(), response.length(), 0);
				closesocket(client_socket);
				return;
			}
		}

		// Если файл имеет расширение html
		if (all_pages[i].getType() == "html")
		{
			std::string header;
			header += "HTTP/1.1 200 OK\r\n";
			header += "Server: LogosServer / 1.0 (Win64)\r\n";
			header += "X-Powered-By: Logos / 1.0\r\n";
			header += "Content-Length: " + std::to_string(body.length()) + "\r\n";
			header += "Keep-Alive: timeout=5, max=86\r\n";
			header += "Cache-Control: no-cache; no-store; must-revalidate\r\n";
			header += "Connection: Keep-Alive\r\n";
			header += "Content-Type: text/html; charset=UTF-8\r\n";
			header += "Content-Language: ru-RU\r\n";
			header += "\r\n\r\n";
			std::string response = header + body;

			send(client_socket, response.c_str(), response.length(), 0);
			closesocket(client_socket);
		}
		if (all_pages[i].getType() == "png" || all_pages[i].getType() == "jpg" || all_pages[i].getType() == "jpeg")
		{
			std::vector<char> bytes;
			std::ifstream image("Source" + all_pages[i].getName(), std::ios_base::binary);
			if (image)
			{
				body = EMPTY;
				char ch;
				while (image.get(ch))
					bytes.push_back(ch);
			}
			image.close();

			for (int z = 0; z < bytes.size(); z++)
				body += bytes[z];

			std::string header;
			header += "HTTP/1.1 200 OK\r\n";
			header += "Server: LogosServer / 1.0 (Win64)\r\n";
			header += "X-Powered-By: Logos / 1.0\r\n";
			header += "Accept-Ranges: bytes\r\n";
			header += "Content-Length: " + std::to_string(body.length()) + "\r\n";
			header += "Connection: Keep-Alive\r\n";
			if (all_pages[i].getType() == "png")
				header += "Content-Type: image/png";
			if (all_pages[i].getType() == "jpeg" || all_pages[i].getType() == "jpg")
				header += "Content-Type: image/jpg";
			header += "\r\n\r\n";


			std::string response = header + body;

			send(client_socket, response.c_str(), response.length(), 0);
			closesocket(client_socket);
		}
		else if (all_pages[i].getType() == "css")
		{
			std::string header;
			header += "HTTP/1.1 200 OK\r\n";
			header += "Server: LogosServer / 1.0 (Win64)\r\n";
			header += "X-Powered-By: Logos / 1.0\r\n";
			header += "Content-Length: " + std::to_string(body.length()) + "\r\n";
			header += "Keep-Alive: timeout=5, max=86\r\n";
			header += "Connection: Keep-Alive\r\n";
			header += "Content-Type: text/css; charset=UTF-8\r\n";
			header += "\r\n\r\n";
			std::string response = header + body;

			send(client_socket, response.c_str(), response.length(), 0);
			closesocket(client_socket);
		}
		else if (all_pages[i].getType() == "js")
		{
			std::vector<char> bytes;
			std::ifstream image("Source" + all_pages[i].getName(), std::ios_base::binary);
			if (image)
			{
				body = EMPTY;
				char ch;
				while (image.get(ch))
					bytes.push_back(ch);
			}
			image.close();

			for (int z = 0; z < bytes.size(); z++)
				body += bytes[z];

			std::string header;
			header += "HTTP/1.1 200 OK\r\n";
			header += "Server: LogosServer / 1.0 (Win64)\r\n";
			header += "X-Powered-By: Logos / 1.0\r\n";
			header += "Content-Length: " + std::to_string(body.length()) + "\r\n";
			header += "Connection: Keep-Alive\r\n";
			header += "Content-Type: application/javascript";
			header += "\r\n\r\n";
			std::string response = header + body;

			send(client_socket, response.c_str(), response.length(), 0);
			closesocket(client_socket);
		}
		else if (all_pages[i].getType() == "ttf")
		{
			std::vector<char> bytes;
			std::ifstream image("Source" + all_pages[i].getName(), std::ios_base::binary);
			if (image)
			{
				body = EMPTY;
				char ch;
				while (image.get(ch))
					bytes.push_back(ch);
			}
			image.close();

			for (int z = 0; z < bytes.size(); z++)
				body += bytes[z];

			std::string header;
			header += "HTTP/1.1 200 OK\r\n";
			header += "Server: LogosServer / 1.0 (Win64)\r\n";
			header += "X-Powered-By: Logos / 1.0\r\n";
			header += "Content-Length: " + std::to_string(body.length()) + "\r\n";
			header += "Connection: Keep-Alive\r\n";
			header += "Content-Type: text/ttf";
			header += "\r\n\r\n";
			std::string response = header + body;

			send(client_socket, response.c_str(), response.length(), 0);
			closesocket(client_socket);
		}
		// Если нужно выполнить скрипт...
		else
		{
			std::cout << temp << std::endl;
			// Передача управления интерпретатору
			interpreter_start(client_socket, i, temp);
		}
	}
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

	std::cout << "IP: \t" << ip << std::endl;
	std::cout << "PORT: \t" << port << std::endl;

	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
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