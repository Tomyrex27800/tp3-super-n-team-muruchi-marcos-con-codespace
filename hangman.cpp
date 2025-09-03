#include "hangman.h"
#include <algorithm>
#include <chrono>
#include <cctype>

Hangman::Hangman() : currentPlayerIndex(0), gameState(GameState::WAITING_FOR_PLAYERS), 
                     maxIncorrectGuesses(6), incorrectGuesses(0) {
    // Inicializar el generador de números aleatorios con el tiempo actual
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rng.seed(static_cast<unsigned int>(seed));
}

bool Hangman::addPlayer(const std::string& playerName) {
    if (gameState != GameState::WAITING_FOR_PLAYERS) {
        return false; // No se pueden agregar jugadores una vez iniciado el juego
    }
    
    if (playerName.empty()) {
        return false; // Nombre vacío no válido
    }
    
    // Verificar que no exista un jugador con el mismo nombre
    for (const auto& player : players) {
        if (player.name == playerName) {
            return false; // Jugador ya existe
        }
    }
    
    players.emplace_back(playerName);
    return true;
}

std::vector<std::string> Hangman::getPlayerNames() const {
    std::vector<std::string> names;
    for (const auto& player : players) {
        if (player.isActive) {
            names.push_back(player.name);
        }
    }
    return names;
}

int Hangman::getPlayerCount() const {
    int count = 0;
    for (const auto& player : players) {
        if (player.isActive) {
            count++;
        }
    }
    return count;
}

std::string Hangman::getCurrentPlayerName() const {
    if (players.empty() || gameState != GameState::IN_PROGRESS) {
        return "";
    }
    
    // Buscar el siguiente jugador activo
    for (size_t i = 0; i < players.size(); ++i) {
        size_t index = (currentPlayerIndex + i) % players.size();
        if (players[index].isActive) {
            return players[index].name;
        }
    }
    
    return "";
}

bool Hangman::startGame() {
    int activePlayerCount = getPlayerCount();
    if (activePlayerCount < 2) {
        return false; // Necesita al menos 2 jugadores activos
    }
    
    if (gameState != GameState::WAITING_FOR_PLAYERS) {
        return false; // El juego ya fue iniciado
    }
    
    gameState = GameState::IN_PROGRESS;
    
    // Buscar el primer jugador activo
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].isActive) {
            currentPlayerIndex = i;
            break;
        }
    }
    
    selectRandomWord();
    initializeWordDisplay();
    guessedLetters.clear();
    incorrectGuesses = 0;
    winner = "";
    
    return true;
}

bool Hangman::isGameStarted() const {
    return gameState == GameState::IN_PROGRESS;
}

bool Hangman::isGameFinished() const {
    return gameState == GameState::FINISHED;
}

GameState Hangman::getGameState() const {
    return gameState;
}

void Hangman::selectRandomWord() {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(wordPool.size() - 1));
    int index = dist(rng);
    currentWord = wordPool[index];
}

void Hangman::initializeWordDisplay() {
    wordDisplay = "";
    for (size_t i = 0; i < currentWord.length(); ++i) {
        if (currentWord[i] == ' ') {
            wordDisplay += " ";
        } else {
            wordDisplay += "_";
        }
    }
}

bool Hangman::isWordGuessed() const {
    return wordDisplay.find('_') == std::string::npos;
}

void Hangman::updateWordDisplay(char letter) {
    for (size_t i = 0; i < currentWord.length(); ++i) {
        if (std::toupper(currentWord[i]) == std::toupper(letter)) {
            wordDisplay[i] = currentWord[i];
        }
    }
}

bool Hangman::guessLetter(const std::string& playerName, char letter) {
    if (gameState != GameState::IN_PROGRESS) {
        return false; // El juego no está en progreso
    }
    
    if (!isPlayerTurn(playerName)) {
        return false; // No es el turno del jugador
    }
    
    char upperLetter = std::toupper(letter);
    
    // Verificar si la letra ya fue adivinada
    if (guessedLetters.find(upperLetter) != guessedLetters.end()) {
        return false; // Letra ya fue adivinada
    }
    
    guessedLetters.insert(upperLetter);
    
    // Verificar si la letra está en la palabra
    bool letterFound = false;
    for (char c : currentWord) {
        if (std::toupper(c) == upperLetter) {
            letterFound = true;
            break;
        }
    }
    
    if (letterFound) {
        updateWordDisplay(upperLetter);
        
        // Verificar si la palabra está completa
        if (isWordGuessed()) {
            winner = playerName;
            gameState = GameState::FINISHED;
            return true;
        }
    } else {
        incorrectGuesses++;
        
        // Verificar si se alcanzó el límite de intentos incorrectos
        if (incorrectGuesses >= maxIncorrectGuesses) {
            gameState = GameState::FINISHED;
            // No hay ganador en este caso
            return true;
        }
    }
    
    nextTurn();
    return true;
}

bool Hangman::guessWord(const std::string& playerName, const std::string& word) {
    if (gameState != GameState::IN_PROGRESS) {
        return false; // El juego no está en progreso
    }
    
    if (!isPlayerTurn(playerName)) {
        return false; // No es el turno del jugador
    }
    
    // Convertir ambas palabras a mayúsculas para comparar
    std::string upperWord = word;
    std::string upperCurrentWord = currentWord;
    std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);
    std::transform(upperCurrentWord.begin(), upperCurrentWord.end(), upperCurrentWord.begin(), ::toupper);
    
    if (upperWord == upperCurrentWord) {
        // ¡Palabra correcta! El jugador gana
        winner = playerName;
        gameState = GameState::FINISHED;
        wordDisplay = currentWord; // Mostrar la palabra completa
        return true;
    } else {
        // Palabra incorrecta, cuenta como un intento fallido
        incorrectGuesses++;
        
        // Verificar si se alcanzó el límite de intentos incorrectos
        if (incorrectGuesses >= maxIncorrectGuesses) {
            gameState = GameState::FINISHED;
            return true;
        }
        
        nextTurn();
        return true;
    }
}

bool Hangman::isPlayerTurn(const std::string& playerName) const {
    if (gameState != GameState::IN_PROGRESS || players.empty()) {
        return false;
    }
    
    return players[currentPlayerIndex].name == playerName;
}

std::string Hangman::getWordDisplay() const {
    return wordDisplay;
}

std::set<char> Hangman::getGuessedLetters() const {
    return guessedLetters;
}

std::string Hangman::getWinner() const {
    return winner;
}

int Hangman::getRemainingGuesses() const {
    return maxIncorrectGuesses - incorrectGuesses;
}

int Hangman::getIncorrectGuesses() const {
    return incorrectGuesses;
}

std::string Hangman::getCurrentWord() const {
    return currentWord;
}

void Hangman::nextTurn() {
    if (players.empty()) return;
    
    // Buscar el siguiente jugador activo
    size_t initialIndex = currentPlayerIndex;
    do {
        currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
    } while (!players[currentPlayerIndex].isActive && currentPlayerIndex != initialIndex);
}

int Hangman::getCurrentPlayerIndex() const {
    return currentPlayerIndex;
}

void Hangman::removePlayer(const std::string& playerName) {
    for (auto& player : players) {
        if (player.name == playerName) {
            player.isActive = false;
            break;
        }
    }
}

std::vector<std::pair<std::string, int>> Hangman::getPlayersWithPoints() const {
    std::vector<std::pair<std::string, int>> playersWithPoints;
    for (const auto& player : players) {
        if (player.isActive) {
            playersWithPoints.emplace_back(player.name, player.points);
        }
    }
    return playersWithPoints;
}

void Hangman::resetGame() {
    if (gameState == GameState::FINISHED) {
        // Sumar punto al ganador
        for (auto& player : players) {
            if (player.name == winner && player.isActive) {
                player.points++;
                break;
            }
        }
        
        // Reiniciar juego
        gameState = GameState::IN_PROGRESS;
        currentPlayerIndex = 0;
        selectRandomWord();
        initializeWordDisplay();
        guessedLetters.clear();
        incorrectGuesses = 0;
        winner = "";
        
        // Buscar el primer jugador activo
        for (size_t i = 0; i < players.size(); ++i) {
            if (players[i].isActive) {
                currentPlayerIndex = i;
                break;
            }
        }
    }
}

void Hangman::resetToWaiting() {
    gameState = GameState::WAITING_FOR_PLAYERS;
    currentPlayerIndex = 0;
    currentWord = "";
    wordDisplay = "";
    guessedLetters.clear();
    incorrectGuesses = 0;
    winner = "";
}
