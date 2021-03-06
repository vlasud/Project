﻿#include "Global.h"

std::string generate_session_key(void){
	std::srand(time(0));
	std::string key = EMPTY;

	do{
		key = EMPTY;

		for (register u_int i = 0; i < 20; i++)
			key += (1 + rand() % 9) + 48;

	} while (all_user_sessions.find(key) != all_user_sessions.end());

	return key;
}


std::string interpreter_start(const SOCKET client_socket, Page &page_object, Server *server, Session *_session)
// client_socket - сокет клиента
// file_id - смещение вектора файлов сайта
{
	bool isError						= false;
	bool isScriptFound					= false;
	bool isEndFound						= false;
	std::string redirect_page			= EMPTY;
	std::string session_key				= (server != nullptr) ? server->get_logosid() : EMPTY;
	unsigned int count_lines_of_file	= page_object.all_lines.size();
	

	for (register u_int i = 0; i < count_lines_of_file; i++)
	{
		for (register u_int i_begin = 0; i_begin + 1 < page_object.all_lines[i].size(); i_begin++) {
			if (page_object.all_lines[i][i_begin] == '<' && page_object.all_lines[i][i_begin + 1] == '#')
			{
				for (register u_int j = i + 1; j < count_lines_of_file; j++)
				{
					for (register u_int i_end = 0; i_end + 1 < page_object.all_lines[j].size(); i_end++) {
						if (page_object.all_lines[j][i_end] == '#' && page_object.all_lines[j][i_end + 1] == '>')
						{
							Session session(i + 1, client_socket);
							// Получение идентификатора сессии

							// Если клиент имеет куки с ключом сессии
							if (session_key != EMPTY)
							{
								// Если в памяти уже хранятся данные сессии с таким ключом
								// Происходит копирование всех данных
								if (all_user_sessions.find(session_key) != all_user_sessions.end())
								{
									session.all_data = all_user_sessions[session_key].all_data;
									session.all_data_buffer = all_user_sessions[session_key].all_data_buffer;
									session.mysql_connections = all_user_sessions[session_key].mysql_connections;
								}
								// Иначе выделяется память под данную сессию
								else all_user_sessions[session_key] = session;
							}
							// Если у клиента нет куки с id сессии
							else
							{
								// Если клиент зашел впервые - генерируется новый ключ сессии
								if (_session == nullptr || _session == nullptr && _session->session_key == EMPTY)
								{
									session_key = generate_session_key();
									all_user_sessions[session_key] = session;
								}
								// Если данная функция была вызвана путем команды include (т.к в параметре request будет пусто)
								// То скопировать все данные сессии из предыдущего вызова данной функции
								else if (_session != nullptr) {
									session_key = _session->session_key;
								}
							}
							session.session_key = session_key;
							// Запись названия текущего файла
							session.set_file_name(page_object.getName());

							// Удаление всех данных с именем _post и _get
							if (server != nullptr) {
								std::map<std::string, Instruction>::iterator iter = session.all_data.find("_post");
								if (iter != session.all_data.end()) session.all_data.erase(iter);

								iter = session.all_data.find("_get");
								if (iter != session.all_data.end()) session.all_data.erase(iter);

								// Если были переданы GLOBAL параметры, то записать их
								session.all_data[server->get_global_data().body] = server->get_global_data();
							}
							all_user_sessions[session_key].all_data = session.all_data;

							// Если существуют статические данные, то скопировать их в текущую сессию
							if (static_data.size() > 0)
								session.all_data.insert(static_data.begin(), static_data.end());

							if (_session != nullptr) {
								session.mysql_connections = _session->mysql_connections;
								session.all_data = _session->all_data;
								session.all_data_buffer = _session->all_data_buffer;
								session.definition_functions = _session->definition_functions;
							}

							// Выполнение скрипта
							read_script(session, page_object, i + 1, j - 1);

							// Если была ошибка - показать сообщение ошибки
							if (session.error != nullptr)
							{
								if (_session != nullptr) {
									_session->error = new ErrorCore(session.error->get_error_text(), _session);
								}
								page_object.all_lines.clear();
								page_object.all_lines.push_back(session.error->get_error_text());
								isError = true;
								break;
							}
							// Иначе заменить исполняемый скрипт на результат работы скрипта
							else
							{
								if (!session.isSessionDelete) {
									all_user_sessions[session_key] = session;
								}
								// Если была команда удаления текущей сессии
								else if (all_user_sessions.find(session_key) != all_user_sessions.end()) {
									all_user_sessions.erase(session_key);
								}

								redirect_page = session.redirect_page;

								page_object.all_lines.erase(page_object.all_lines.begin() + i, page_object.all_lines.begin() + j + 1);

								// Перевернуть вектор
								std::reverse(session.output.output_data.begin(), session.output.output_data.end());

								for (register u_int mn = 0; mn < session.output.output_data.size(); mn++) {
									page_object.all_lines.insert(page_object.all_lines.begin() + i, session.output.output_data[mn]);
								}

								// Копирование всех данных если была вызвана функция include
								if (_session != nullptr)
								{
									_session->mysql_connections = session.mysql_connections;
									_session->all_data = session.all_data;
									_session->all_data_buffer = session.all_data_buffer;
									_session->definition_functions = session.definition_functions;

									for (register u_int o_i = 0; o_i < page_object.all_lines.size(); o_i++)
										_session->output.output_data.push_back(page_object.all_lines[o_i]);
								}
							}

							count_lines_of_file = page_object.all_lines.size();
							isEndFound = true;
							break;
						}
						if (redirect_page != EMPTY)
							break;
					}
					if (isError || redirect_page != EMPTY) {
						break;
					}
					if (isEndFound) {
						isEndFound = false;
						break;
					}
				}

				break;
			}

			if (redirect_page != EMPTY || isError)
				break;
		}
		if (isError || redirect_page != EMPTY) {
			break;
		}
	}

	// Если было перенаправление - вернуть страницу на которую идет перенаправление
	if (redirect_page != EMPTY) {
		return redirect_page;
	}
	// Иначе вернуть строку 22 в виде байт
	else {
		if (server != nullptr) {
			server->set_logosid(session_key);
		}
		return "[\x32\x32]";
	}
}

std::string format_data(std::string data)
// data - данные
// ~ Переводит данные в правильный формат
{
	if (data[0] == '"')
	{
		if(data[data.length() - 1] != '"')
			data += "\"";
		return data;
	}
	
	// Перевод в нижний регистр
	std::transform(data.begin(), data.end(), data.begin(), ::tolower);

	int counter = 0;
	bool isDoted = false;
	while (data[counter] >= '0' && data[counter] <= '9' || data[counter] == '.')
	{
		if (data[counter] == '.')
		{
			if (isDoted) break;
			else isDoted = true;
		}
		counter++;
	}

	if (data.length() > 0 && counter == 0 && 
		std::find(KEY_WORDS.begin(), KEY_WORDS.end(), data) == KEY_WORDS.end()) data = "null";

	return data;
}

TYPE_OF_INSTRUCTION_FOR_PARSER get_type_of_instruction(char ch)
// ch - символ
// ~ Определяет тип инструкции
{
	if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' ||
		ch >= '0' && ch <= '9' || ch == '_' || ch == '"' || ch == '.')
		return TYPE_OF_INSTRUCTION_FOR_PARSER::_WORD;
	else if (ch == '\t' || ch == ' ')
		return TYPE_OF_INSTRUCTION_FOR_PARSER::SPACE;
	else return TYPE_OF_INSTRUCTION_FOR_PARSER::_OPERATOR;
}

TYPE_OF_DATA get_type_of_data(std::string data)
// data - данные
// ~ Определяет тип данных
{
	// Приведение данных к правильному виду
	data = format_data(data);

	if (data[0] == '"') return TYPE_OF_DATA::_STRING;
	else if (data == "true" || data == "false") return TYPE_OF_DATA::_BOOLEAN;
	else if (data == "null") return TYPE_OF_DATA::_NONE;
	else if (data.find('.') != std::string::npos) return TYPE_OF_DATA::_DOUBLE;
	else return TYPE_OF_DATA::_INT;
}

void read_script(Session &session, Page &page_object, const unsigned int start, const unsigned int end)
// client_socket - сокет клиента
// start - начало строки
// end - конец строки
{
	unsigned int all_lines_length = 0;
	std::string word = EMPTY;
	unsigned int line_counter = 0;
	TYPE_OF_INSTRUCTION_FOR_PARSER last_type_of_instruction = TYPE_OF_INSTRUCTION_FOR_PARSER::SPACE;

	// Определение команд
	for (unsigned register int i = start; i <= end; i++)
	{
		all_lines_length = page_object.all_lines[i].length();
		session.lines.push_back(LineInstructions { } );

		// Подсчет уровня локального пространства
		register int space_counter = -1;
		unsigned register int j = 0;

		u_int space_counter_ = 0;
		for (register u_int j = 0; j < page_object.all_lines[i].length(); j++)
			if (page_object.all_lines[i][j] == ' ')
				space_counter_++;
			else break;

		session.lines[line_counter].namespace_level += space_counter_ / 4;

		while (page_object.all_lines[i][++space_counter] == '\t')
		{
			session.lines[line_counter].namespace_level++;
			j++;
		}
		//

		for (; j < all_lines_length; j++)
		{
			if (last_type_of_instruction != get_type_of_instruction(page_object.all_lines[i][j]))
			{
				if (word != EMPTY)
				{
					if(last_type_of_instruction != TYPE_OF_INSTRUCTION_FOR_PARSER::SPACE)
						session.lines[line_counter].instructions.push_back(Instruction{ word });
					word = EMPTY;
				}

				last_type_of_instruction = get_type_of_instruction(page_object.all_lines[i][j]);
				word += page_object.all_lines[i][j];

				if (word == "\"")
					do { word += page_object.all_lines[i][++j]; } while (page_object.all_lines[i][j] != '"');
			}
			else
			{				
				word += page_object.all_lines[i][j];

				if (last_type_of_instruction == TYPE_OF_INSTRUCTION::OPERATOR)
				{
					if (std::find(OPERATORS.begin(), OPERATORS.end(), word) != OPERATORS.end())
					{
						session.lines[line_counter].instructions.push_back(Instruction{ word });
						word = EMPTY;
					}
					else
					{
						if (std::find(OPERATORS.begin(), OPERATORS.end(), std::string{ word[0] }) != OPERATORS.end())
							session.lines[line_counter].instructions.push_back(Instruction{ std::string { word[0]} });

						word = word[1];
					}
				}
			}
		}
		
		if (word != EMPTY && last_type_of_instruction != TYPE_OF_INSTRUCTION_FOR_PARSER::SPACE)
		{
			session.lines[line_counter].instructions.push_back(Instruction{ word });
			word = EMPTY;
		}
		last_type_of_instruction = TYPE_OF_INSTRUCTION_FOR_PARSER::SPACE;

		line_counter++;
	}

	// Определение начального уровня локального пространства
	for (register u_int i = 0; i < session.lines.size(); i++) {
		if (session.lines[i].instructions.size() > 0) {
			session.start_level = session.lines[i].namespace_level;
			break;
		}
	}
	// Начало выполнения скрипта
	do_script(session, 0, session.lines.size() - 1);
}


std::string check_correct_syntax(LineInstructions &line)
// Проверка корректности синтаксиса
{
	std::string result = EMPTY;
	// Скобочки
	int counter[3] = { 0,0,0 };
	std::string brackets[3][2] = {
		{"(", ")"},
		{"[", "]"},
		{"{", "}"}
	};

	for (register u_int i = 0; i < line.instructions.size(); i++) {
		if (line.instructions[i].body == "(") counter[0]++;
		else if (line.instructions[i].body == ")") counter[0]--;
		else if (line.instructions[i].body == "[") counter[1]++;
		else if (line.instructions[i].body == "]") counter[1]--;
		else if (line.instructions[i].body == "{") counter[2]++;
		else if (line.instructions[i].body == "}") counter[2]--;
	}
	for (register u_int i = 0; i < 3; i++) {
		if (counter[i] < 0) {
			// Ошибка.
			result = "missing bracket " + brackets[i][0];
			break;
		}
		else if (counter[i] > 0) {
			// Ошибка.
			result = "missing bracket " + brackets[i][1];
			break;
		}
	}
	// Последовательность инструкций
	for (register u_int i = 1; i < line.instructions.size(); i++) {
		if ((line.instructions[i].body == "(" || line.instructions[i].body == ")"
			|| line.instructions[i].body == "[" || line.instructions[i].body == "]"
			|| line.instructions[i].body == "{" || line.instructions[i].body == "}"
			|| line.instructions[i].body == "++" || line.instructions[i].body == "--")
			||
			(line.instructions[i - 1].body == "(" || line.instructions[i - 1].body == ")"
			|| line.instructions[i - 1].body == "[" || line.instructions[i - 1].body == "]"
			|| line.instructions[i - 1].body == "{" || line.instructions[i - 1].body == "}")
			|| line.instructions[i - 1].body == "++" || line.instructions[i - 1].body == "--"
			) {
			continue;
		}
		if (line.instructions[i].type_of_instruction == line.instructions[i - 1].type_of_instruction ) {
			
			// Если это оператор "-", то проверить является ли он оператором разности
			// Если оператор показывает знак числа, то ошибки нет и пропустить дальнейшую проверку
			if (i + 1 < line.instructions.size() && (line.instructions[i].body == "-" || line.instructions[i].body == "!")
				&& line.instructions[i + 1].type_of_instruction == TYPE_OF_INSTRUCTION::DATA) {
				continue;
			}

			// Ошибка.
			result = "wrong syntax between " + line.instructions[i - 1].body + " and " + line.instructions[i].body;
			break;
		}
		else if (line.instructions[i].type_of_instruction == TYPE_OF_INSTRUCTION::OPERATOR) {
			if (i == line.instructions.size() - 1) {
				// Ошибка.
				result = "wrong syntax between " + line.instructions[i].body + " and [end command line]";
				break;
			}
			else if (line.instructions[i + 1].type_of_instruction != TYPE_OF_INSTRUCTION::DATA
				&& line.instructions[i + 1].body != "("
				&& line.instructions[i + 1].body != "{"
				&& line.instructions[i + 1].body != "++"
				&& line.instructions[i + 1].body != "--") {

				// Если это оператор "-", то проверить является ли он оператором разности
				// Если оператор показывает знак числа, то ошибки нет и пропустить дальнейшую проверку
				if (i + 2 < line.instructions.size() && line.instructions[i + 1].body == "-"
					&& line.instructions[i + 2].type_of_instruction == TYPE_OF_INSTRUCTION::DATA) {
					continue;
				}

				// Ошибка.
				result = "wrong syntax between " + line.instructions[i].body + " and " + line.instructions[i + 1].body;
				break;
			}
		}
	}

	return result;
}
 
void action_of_function_return(Session &session) 
// Вызывается каждый раз после выполнения пользовательской функции
{
	// Обновить данные с модификатором global
	for (auto b = session.all_data.begin(); b != session.all_data.end(); b++)
		if (b->second.isUsedHasGlobal) {
			session.all_data_buffer[b->second.body] = b->second;
		}

	session.all_data = session.all_data_buffer;
	session.all_data_buffer.clear();
}

void do_script(Session &session, const unsigned int begin, unsigned int end, bool isOnlyData, FunctionDefinition *func, bool isCommand)
// Выполнение скрипта
{
	int start_line;
	// Если происходит выполнение функции, то переместить все данные в буффер
	// И дальше заносить данные как локальные переменные
	// После завершения функции вернуть данные из буфера
	if (func != nullptr)
	{
		start_line = func->line + 1;

		session.all_data_buffer = session.all_data;
		session.all_data.clear();
		
		session.current_function = func;

		// Внесение параметров в буффер локальных переменных
		for (register u_int i = 0; i < func->parametrs.size(); i++)
			session.all_data[func->parametrs[i].body] = func->parametrs[i];
	}
	else {
		start_line = session.get_start_line();
	}

	u_int start_level = session.lines[begin].namespace_level;

	for (register u_int i = begin; i <= end; i++)
	{

		// Обновление номера текущей строки в файле
		if (func == nullptr) {
			session.update_current_line(start_line + i);
		}
		else {
			session.update_current_line(start_line + (func->body.size() - (session.lines.size() - i)));
		}

		// Если была вызвана вызвана инструкция continue
		if (session.isContinue)
		{
			session.isContinue = false;
			return;
		}
		// Если была вызвана вызвана инструкция break
		if (session.isContinue) return;

		if (session.lines[i].namespace_level != start_level) continue;

		// Восстановление использованных инструкций
		if ((func != nullptr || isCommand) && session.lines[i].copy_instructions.size() != 0 
			&& session.lines[i].copy_instructions.size() != session.lines[i].instructions.size())
			session.lines[i].instructions = session.lines[i].copy_instructions;

		// Проверка корректности синтаксиса
		std::string result_of_check_on_syntax = check_correct_syntax(session.lines[i]);
		if (result_of_check_on_syntax != EMPTY) {
			// Ошибка. Синтаксис
			session.error = new ErrorCore(result_of_check_on_syntax, &session);
			return;
		}

		// Сохранение инструкций перед их выполнением
		if (session.lines[i].copy_instructions.size() == 0) session.lines[i].copy_instructions = session.lines[i].instructions;

		for (register int j = session.lines[i].instructions.size() - 1; j >= 0; j--)
		{
			// Если инструкция - данные
			if (session.lines[i].instructions[j].type_of_instruction == TYPE_OF_INSTRUCTION::DATA)
			{
				// Если это не константные данные
				if (!(session.lines[i].instructions[j].body[0] >= '0' && session.lines[i].instructions[j].body[0] <= '9' ||
					session.lines[i].instructions[j].body[0] == '"' || std::find(KEY_WORDS.begin(), KEY_WORDS.end(), session.lines[i].instructions[j].body) != KEY_WORDS.end()))
				{
					// Если это функция
					if (j < session.lines[i].instructions.size() - 1 && session.lines[i].instructions[j + 1].body == "(")
					{
						// Если это пользовательская функция
						if (session.definition_functions.find(session.lines[i].instructions[j].body) != session.definition_functions.end())
						{
							int param_counter = -1;
							FunctionDefinition *tmp = &session.definition_functions.find(session.lines[i].instructions[j].body)->second;

							// Определяет конец функции и начинает исполнение операторов внутри параметров функции
							int bracket_counter = 1;
							for (register u_int z = j + 2; z < session.lines[i].instructions.size(); z++) {
								if (session.lines[i].instructions[z].body == ")") bracket_counter--;
								else if (session.lines[i].instructions[z].body == "(") bracket_counter++;

								if (bracket_counter == 0) {
									do_line_script_operators(session, i, j + 1, z - 1);
									break;
								}
							}
							// Копирование параметров
							for (register unsigned int z = j + 1; z < session.lines[i].instructions.size() && session.lines[i].instructions[z].body != ")"; z++)
								if (session.lines[i].instructions[z].type_of_instruction == TYPE_OF_INSTRUCTION::DATA) {
									param_counter++;
									std::string body_name = tmp->parametrs[param_counter].body;
									bool isConst = tmp->parametrs[param_counter].isConst;
									tmp->parametrs[param_counter] = session.lines[i].instructions[z];
									tmp->parametrs[param_counter].body = body_name;
									if (!isConst) {
										tmp->parametrs[param_counter].isConst = false;
									}
								}

							// Размер вектора lines до добавления тела функции
							u_int last_index = session.lines.size();
							// Размер тела функции
							size_t function_body_size = tmp->body.size();
							// Вставка тела функции в вектор lines
							session.lines.insert(session.lines.end(), tmp->body.begin(), tmp->body.end());
							// Сохранение в буффер инструкций текущей функции
							// Это необходимо т.к при передаче данных в другую функцию после исполнения они удаляются
							std::map<std::string, Instruction> buffer_for_instructions_current_func = session.all_data;
							// Исполнение инструкций
							do_script(session, last_index, last_index + function_body_size - 1, false, tmp);
							
							
							// Запись инструкций из буффера
							for (auto b = session.all_data.begin(); b != session.all_data.end(); b++) {
								if (b->second.isUsedHasGlobal) {
									buffer_for_instructions_current_func[b->second.body] = session.all_data[b->second.body];
								}
							}
							session.all_data = buffer_for_instructions_current_func;

							// Удаление тела инструкций из вектора lines
							session.lines.erase(session.lines.begin() + last_index, session.lines.end());
							// Запись результата возврата функции
							session.lines[i].instructions[j] = tmp->result;

							// Удаление лишних инструкций после вызова функций
							if (j + 1 < session.lines[i].instructions.size()) {
								for (register u_int p = j + 1; p < session.lines[i].instructions.size()
									&& session.lines[i].instructions[p].body != ")"; p++){
									session.lines[i].instructions.erase(session.lines[i].instructions.begin() + p);
									p--;
								}
								session.lines[i].instructions.erase(session.lines[i].instructions.begin() + j + 1);
							}

							// Если была вызвана команда return - завершить работу функции
							if (session.current_function->isReturn) {
								action_of_function_return(session);
								return;
							}
							continue;
						}
						// Если возможно это системная функция
						else if (j > 0 && session.lines[i].instructions[j - 1].body != "fun" || j == 0){
							for (register u_int b = 0; b < system_functions.size(); b++){

								if (system_functions[b].get_name() == session.lines[i].instructions[j].body){
									SystemFunction tmp = system_functions[b];

									// Определяет конец функции и начинает исполнение операторов внутри параметров функции
									// Происходит подсчет баланса скобок. Если переменная bracket_counter будет равна 0, то значит был достигнут конец функции
									int bracket_counter = 1;
									for (register u_int z = j + 2; z < session.lines[i].instructions.size(); z++) {
										if (session.lines[i].instructions[z].body == ")") bracket_counter--;
										else if (session.lines[i].instructions[z].body == "(") bracket_counter++;

										if (bracket_counter == 0) {
											do_line_script_operators(session, i, j + 1, z - 1);
											break;
										}
									}

									// Перемещение параметров функции в объект обстракции функции
									// Сразу же происходит удаление просмотренных инструкций-операторов
									// Данные нельзя сразу удалять потому, что в функцию может передаться массив...
									// ... и если удалить из конвейера данные, то указатель будет указывать в пустоту
									while(session.lines[i].instructions.size() > j + 1) {
										if (session.lines[i].instructions[j + 1].type_of_instruction == TYPE_OF_INSTRUCTION::DATA) {
											tmp.set_params(session.lines[i].instructions[j + 1]);
										}
										std::string current_cmd = session.lines[i].instructions[j + 1].body;
										session.lines[i].instructions.erase(session.lines[i].instructions.begin() + j + 1);
										if (current_cmd == ")") {
											break;
										}
									}

									// Переход на исполнение системной функции
									tmp.start_function(&session);

									// Запись в команду, которая раньше представляла функцию - результат работы функции
									session.lines[i].instructions[j] = tmp.get_result();
									break;
								}
								// Иначе если функция с таким именем не найдена
								else if (b == system_functions.size() - 1){
									// Ошибка. Неизвестная функция
									session.error = new ErrorCore("unknow function {" + session.lines[i].instructions[j].body + "} ", &session);
									return;
								}
							}
						}
					}
					// Иначе если переменная
					else if (session.all_data.find(session.lines[i].instructions[j].body) == session.all_data.end())
					{
						// Если переменная не нашлась в локальной видимости, то ищем ее в глобальной
						if (j > 0 && session.lines[i].instructions[j - 1].body == "global" && session.all_data_buffer.find(session.lines[i].instructions[j].body) != session.all_data_buffer.end())
						{
							session.all_data[session.all_data_buffer.find(session.lines[i].instructions[j].body)->second.body] = session.all_data_buffer.find(session.lines[i].instructions[j].body)->second;
							session.all_data[session.all_data_buffer.find(session.lines[i].instructions[j].body)->second.body].isUsedHasGlobal = true;
						}
						// Иначе если переменной с таким именем вовсе не существует
						else
						{
							session.lines[i].instructions[j].isVariable = true;

							// Если перед переменной стоит модификатор константы
							if (j > 0 && session.lines[i].instructions[j - 1].body == "const" || j > 1 && session.lines[i].instructions[j - 2].body == "const")
								session.lines[i].instructions[j].isConst = true;
							// Если перед переменной стоит модификатор static
							if (j > 0 && session.lines[i].instructions[j - 1].body == "static")
							{
								if (func != nullptr) {
									// Ошибка. Неизвестная функция
									session.error = new ErrorCore("static data can be init only in global space", &session);
									return;
								}

								if (static_data.find(session.lines[i].instructions[j].body) == static_data.end())
								{
									session.lines[i].instructions[j].isStatic = true;
									static_data[session.lines[i].instructions[j].body] = session.lines[i].instructions[j];
								}
								else session.lines[i].instructions[j] = static_data[session.lines[i].instructions[j].body];
							}
							session.all_data[session.lines[i].instructions[j].body] = session.lines[i].instructions[j];
						}
					}
					else {
						session.lines[i].instructions[j] = session.all_data.find(session.lines[i].instructions[j].body)->second;
						if (session.lines[i].instructions[j].isStatic)
						{
							if (static_data.find(session.lines[i].instructions[j].body) != static_data.end())
								session.lines[i].instructions[j] = static_data.find(session.lines[i].instructions[j].body)->second;
						}
					}
				}
				else
				{
					session.lines[i].instructions[j].isVariable = false;
				}
			}
		}
		if (session.lines[i].instructions.size() > 0)
		{
			do_line_script_operators(session, i, 0, session.lines[i].instructions.size() - 1);
			// Если произошла синтаксическая ошибка - прекратить дальнейшее выполнение программы
			if (session.error != nullptr) return;

			if (!isOnlyData && do_line_script_commands(session, i, 0, session.lines[i].instructions.size() - 1));
			// Если произошла синтаксическая ошибка - прекратить дальнейшее выполнение программы
			if (session.error != nullptr) return;

			if (func != nullptr && func->isReturn)
			{
				func->isReturn = false;
				break;
			}
		}
	}
	if (func != nullptr){
		action_of_function_return(session);
	}
}


Instruction::Instruction(const std::string body)
{
	this->body = body;
	this->ptr = nullptr;
	this->selected_char = -1;

	if (std::find(OPERATORS.begin(), OPERATORS.end(), body) != OPERATORS.end())
		this->type_of_instruction = TYPE_OF_INSTRUCTION::OPERATOR;
	else if (std::find(COMMANDS.begin(), COMMANDS.end(), body) != COMMANDS.end())
		this->type_of_instruction = TYPE_OF_INSTRUCTION::COMMAND;
	else
		this->type_of_instruction = TYPE_OF_INSTRUCTION::DATA;

	if (type_of_instruction == TYPE_OF_INSTRUCTION::DATA)
	{
		if (!(body[0] >= '0' && body[0] <= '9' ||
			body[0] == '"' || std::find(KEY_WORDS.begin(), KEY_WORDS.end(), body) != KEY_WORDS.end()))
		{
			this->data = "null";
			this->type_of_data = TYPE_OF_DATA::_NONE;
		}
		else
		{
			this->data = body;
			type_of_data = get_type_of_data(this->data);

			if (this->type_of_data == TYPE_OF_DATA::_STRING)
			{
				data = EMPTY;
				for (register unsigned int i = 1; i < body.length() - 1; i++)
					this->data += body[i];
			}
		}
	}
}