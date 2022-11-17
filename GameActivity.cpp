#include <Windows.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <regex>
#include <thread>

const bool AliveCell = 1;
const bool EmptyOrDead = 0;

const bool OfflineGameMode = true;
const bool OnlineGameMode = false;

const char ac = 254;

class Ceil {
public:
	int x, y;
	bool isAlive;

	Ceil(int _x, int _y, bool _isAlive=true) { x = _x, y = _y, isAlive = _isAlive; }
	~Ceil() = default;
};

class Messages
{
public:
	void showWarning(std::wstring message)
	{
		MessageBox(nullptr, message.c_str(), this->warningTitle.c_str(), MB_ICONWARNING);
	}

	void showReference()
	{
		MessageBox(nullptr, this->reference.c_str(), this->referenceTitle.c_str(), MB_ICONINFORMATION);
	}

	void errUnknownCommand()
	{
		MessageBox(nullptr, this->unknownCommand.c_str(), this->warningTitle.c_str(), MB_ICONWARNING);
	}

	void errNoInputFile()
	{
		MessageBox(nullptr, this->noFile.c_str(), this->noFileTitle.c_str(), MB_ICONERROR);
	}

	void done()
	{
		MessageBox(nullptr, this->doneStr.c_str(), this->doneTitle.c_str(), MB_ICONINFORMATION);
	}

private:
	std::wstring warningTitle = L"Warning!";
	std::wstring referenceTitle = L"Reference";
	std::wstring noFile = L"There's no such file!";
	std::wstring noFileTitle = L"Error";
	std::wstring unknownCommand = L"Unknown command!";
	std::wstring doneStr = L"Done!";
	std::wstring doneTitle = L"Success";
	std::wstring reference = {
L"Simple implementation of Conway's game of life. \n\
\n\
Universe input file format: \n\
1. #N Version \n\
2. #S wigth, heigth \n\
3. #R #Bx / Sy - birth and survival rool, x and y can be any number from{ 0..8 }\n\
4. Ceils coordinates : x, y in weigthand heigth range.\n\
\n\
Commands: \n\
1. help - show game reference\n\
2. dump<filename> -save current universe state\n\
3. exit - end the game\n\
4. tick<n> -count game state after n iteration"
	};

	std::wstring noType = L"There in no ";
	std::wstring noRules = L"You didn't specify the transition rules for the cell. Standard rules will be selected";

};

//Store a game settings
class GameState {
public:
	//virtual ~GameState() = default;

	std::string birthRools;
	std::string survivalRools;
	int heigthOfField;
	int wigthOfField;

	std::string fileName;
	std::string universeName;
	std::vector <Ceil> cellsCoordinates;
};

class GameLoader : public GameState
{
public:
	//Load one of the default universes
	GameLoader()
	{
		Messages msg;
		msg.showWarning(L"There's no input file. One of the default universes will be loaded");
		loadDefaultUniverse();
	}

	//Load player's universe
	GameLoader(std::wstring fileName)
	{
		Messages msg;
		if (!this->parseInputFile(fileName))
		{
			msg.showWarning(L"There's no input file. One of the default universes will be loaded");
		}
	}
		
private:
	void setDefaultSettings()
	{
		this->birthRools	= "2";
		this->survivalRools = "23";
		this->heigthOfField = 30;
		this->wigthOfField	= 30;
	}

	void loadDefaultUniverse()
	{
		WIN32_FIND_DATAW wfd;
		HANDLE const hFind = FindFirstFileW(L"C:\\Users\\User\\source\\repos\\nsu_lessons\\GameOfLife\\DefaultUniverses\\*", &wfd);
		std::vector<std::wstring> allUniverses;

		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				allUniverses.push_back(wfd.cFileName);
			} while (NULL != FindNextFileW(hFind, &wfd));

			//Remove useless directories
			allUniverses.erase(allUniverses.begin());
			allUniverses.erase(allUniverses.begin());

			FindClose(hFind);
		}

		std::srand(std::time(nullptr));
		int currentUniverse = std::rand() % allUniverses.size();

		this->parseInputFile(L"./DefaultUniverses/" + allUniverses[currentUniverse]);
	}

	int parseInputFile(std::wstring inputFile)
	{
		//In case there's no input file set a default settings
		setDefaultSettings();

		std::ifstream fin(inputFile.c_str());
		if (!fin.is_open())
		{
			loadDefaultUniverse();
			return this->failed_to_open;
		}

		std::regex roolsRegex("(#R )(B[0-9]+\/S[0-9]+)");
		std::regex sizeRegex("[#S ]([0-9]+) ([0-9]+)");
		std::regex universeNameRegex("[#N ]([A-Za-z]*)");
		std::regex digits("[0-9]+");
		std::regex letters("[A-Za-z ]+");
		std::smatch s;

		//Temp buffer
		char temp[256];

		//Read a name of universe
		fin.getline(temp, 256);
		if (std::regex_search(temp, universeNameRegex))
		{
			this->universeName = temp;
			this->universeName.erase(this->universeName.find("#N "), 3);
		}

		//Read rools for the game
		fin.getline(temp, 256);
		if (std::regex_search(temp, roolsRegex))
		{
			std::string str(temp);
			auto iter = std::sregex_iterator(str.begin(), str.end(), digits);
			s = *iter;
			this->birthRools = s.str();
			s = *(++iter);
			this->survivalRools = s.str();
		}

		//Read size of field
		fin.getline(temp, 256);
		if (std::regex_search(temp, sizeRegex))
		{
			std::string str(temp);
			auto iter = std::sregex_iterator(str.begin(), str.end(), digits);
			s = *iter;
			this->wigthOfField = std::stol(s.str());
			s = *(++iter);
			this->heigthOfField = std::stol(s.str());
		}

		//Read coordinates of cells
		int x, y;
		while (!fin.eof())
		{
			fin >> x >> y;
			this->cellsCoordinates.push_back(Ceil(x, y, true));
		}
		return file_success;
	}

	static const int failed_to_open = 0;
	static const int file_success = 1;
};

class GameField
{
public:
	GameField(GameState& gs)
	{
		this->currentGameState = &gs;
		std::vector<bool> temp;

		_field.reserve(currentGameState->heigthOfField);
		for(size_t y = 0; y < currentGameState->heigthOfField; y++)
		{
			temp.resize(0);
			for (size_t i = 0; i < currentGameState->wigthOfField; i++)
			{
				temp.push_back(0);
			}
			_field.push_back(temp);
		}

		for (const Ceil &ceil : currentGameState->cellsCoordinates)
		{
			if(ceil.isAlive) this->_field[ceil.x][ceil.y] = AliveCell;
		}
	}

	~GameField() = default;

	int mapXCoordinate(int x)
	{
		if (x >= currentGameState->wigthOfField) return x % currentGameState->wigthOfField;
		if (x < 0) return currentGameState->wigthOfField - 1;
		return x;
	}

	int mapYCoordinate(int y)
	{
		if (y >= currentGameState->heigthOfField) return y % currentGameState->heigthOfField;
		if (y < 0) return currentGameState->heigthOfField - 1;
		return y;
	}

	void updateField()
	{
		std::vector<Ceil> ceils;
		int neighbours, _x_minus, _x, _x_plus, _y, _y_minus, _y_plus;

		for (size_t y = 0; y < currentGameState->heigthOfField; y++)
		{
			for (size_t x = 0; x < currentGameState->wigthOfField; x++)
			{
				bool isAlive = _field[x][y];

				neighbours	= 0;
				_x			= mapXCoordinate(x);
				_x_minus	= mapXCoordinate(x - 1);
				_x_plus		= mapXCoordinate(x + 1);
				_y			= mapYCoordinate(y);
				_y_minus	= mapYCoordinate(y - 1);
				_y_plus		= mapYCoordinate(y + 1);

				if (_field[_x_minus][_y_minus] == AliveCell) neighbours++;
				if (_field[_x][_y_minus] == AliveCell) neighbours++;
				if (_field[_x_plus][_y_minus] == AliveCell) neighbours++;

				if (_field[_x_minus][_y] == AliveCell) neighbours++;
				if (_field[_x_plus][_y] == AliveCell) neighbours++;

				if (_field[_x_minus][_y_plus] == AliveCell) neighbours++;
				if (_field[_x][_y_plus] == AliveCell) neighbours++;
				if (_field[_x_plus][_y_plus] == AliveCell) neighbours++;
		
				if (shouldBeBorn(neighbours) && !isAlive) ceils.push_back(Ceil(x, y));
				if (shouldSurvive(neighbours) && isAlive) ceils.push_back(Ceil(x, y));
				if (!shouldSurvive(neighbours) && isAlive) ceils.push_back(Ceil(x, y, false));
			}
		}

		for (auto &ceil : ceils)
		{
			if (ceil.isAlive) this->_field[ceil.x][ceil.y] = AliveCell;
			else this->_field[ceil.x][ceil.y] = EmptyOrDead;
		}
	}

	void showField(int numOfIteration = 1)
	{

		for (size_t i = 0; i < numOfIteration; i++)
		{
			std::system("cls");
			this->updateField();
			std::cout << "\t\t\t\t\tConway's game of life.\n" << std::endl;
			std::cout << currentGameState->universeName << std::endl;
			std::cout << "B" << currentGameState->birthRools << "/S" << currentGameState->survivalRools << std::endl;

			for (int y = 0; y < currentGameState->heigthOfField; y++)
			{
				for (int x = 0; x < currentGameState->wigthOfField; x++)
				{
					std::cout << ((this->_field[x][y]) ? &ac : " ") << " ";
				}
				std::cout << std::endl;
			}

			Sleep(100);
		}
	}

	void dumpField(std::wstring outputFile)
	{
		std::ofstream fout(outputFile);
		fout << "#N " << currentGameState->universeName << std::endl;
		fout << "#R " << "#B" << currentGameState->birthRools << "/" << "S" << currentGameState->survivalRools << std::endl;
		fout << "#S " << currentGameState->wigthOfField << " " << currentGameState->heigthOfField << std::endl;

		for (size_t i = 0; i < currentGameState->cellsCoordinates.size(); i++)
		{
			if (currentGameState->cellsCoordinates[i].isAlive)
				fout << currentGameState->cellsCoordinates[i].x << " " << currentGameState->cellsCoordinates[i].y << std::endl;
		}
	}

private:
	GameState* currentGameState;
	std::vector<std::vector<bool>> _field;

	bool shouldSurvive(int neighbours)
	{
		size_t found = currentGameState->survivalRools.find(std::to_string(neighbours));
		if (found != std::string::npos) return true;
		else return false;
	}

	bool shouldBeBorn(int neighbours)
	{
		size_t found = currentGameState->birthRools.find(std::to_string(neighbours));
		if (found != std::string::npos) return true;
		else return false;
	}
};

class CommandState {
public:
	CommandState(GameState& gs) { currentGameState = &gs; }
	~CommandState() = default;

	int readCommand()
	{
		std::cin >> _currentCommand;

		//Exit command
		if (_currentCommand == "exit") return this->_exit;

		//Dump game field command
		if (_currentCommand == "dump") return this->_dump;
		
		//Show field after n iteration
		if (_currentCommand == "tick") return this->_tick;
		
		//Show game reference
		if (_currentCommand == "help") return this->_help;

		return -1;

	}

	const static int _exit = 0;
	const static int _dump = 1;
	const static int _tick = 2;
	const static int _help = 3;

private:
	GameState* currentGameState;
	std::string _currentCommand;
};


int main(int argc, char** argv)
{
	int commandArg = 0, iterations = 0, gameMode = OnlineGameMode;
	std::wstring inputFile, outputFile;

	//Read input file name
	//If there's no input file, let input file = 0
	std::string _temp = (argc == 1) ? "" : argv[1];
	inputFile = std::wstring(_temp.begin(), _temp.end());

	//If there are adiitional arguments
	if (argc > 1)
	{
		gameMode = OfflineGameMode;
		//Read arguments: number of iterations and output file name
		for (int i = 1; i < argc; i++)
		{
			if (argv[i] == std::string("-i"))
			{
				iterations = std::stoi(argv[i + 1]);
			}

			if (argv[i] == std::string("-o")) 
			{
				std::string temp = argv[i + 1];
				outputFile = std::wstring(temp.begin(), temp.end());
			}
		}
	}

	Messages msg;
	GameState* gs = new GameLoader(inputFile);
	GameField game(*gs);
	CommandState command(*gs);
	std::wstring commandString;

	//Offline game mode
	if (gameMode == OfflineGameMode)
	{
		game.showField(iterations);
		game.dumpField(outputFile);
		std::wcout << "Saved in " << outputFile << std::endl;
		exit(0);
	}

	//Start screen
	std::cout << "\t\t\t\t\tConway's game of life.\n" << std::endl;
	std::cout << "Enter tick <number of iteration> to start the game!" << std::endl;
	msg.showReference();


	//Online game mode
	while (1)
	{
		commandArg = command.readCommand();
		switch (commandArg)
		{
		case CommandState::_exit:
			delete gs;
			exit(0);
			break;

		case CommandState::_dump:
			std::wcin >> commandString;
			game.dumpField(commandString);
			msg.done();
			break;

		case CommandState::_tick:
			std::cin >> iterations;
			game.showField(iterations);
			break;

		case CommandState::_help:
			msg.showReference();
			break;

		default:
			msg.errUnknownCommand();
			break;
		}
	}

	delete gs;

	return 0;
}